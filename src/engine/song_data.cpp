#include "song_data.h"

#include "renoise_loader.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

using namespace demo;

size_t SongTrack::firstNoteAtOrAfter(double time) const
{
    auto it = std::lower_bound(notes.begin(), notes.end(), time,
                               [](const NoteEvent &note, double t) { return note.time < t; });
    return size_t(it - notes.begin());
}

bool SongData::load(const std::string &path, std::string *error)
{
    std::string extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });

    if (extension == ".xrns")
        return loadRenoiseSong(path, *this, error);

    if (error)
        *error = "Unsupported song format: " + path;
    return false;
}

const SongTrack *SongData::track(std::string_view name) const
{
    for (const auto &track : m_tracks)
        if (track.name == name)
            return &track;
    return nullptr;
}

float demo::noteEnvelope(const NoteEvent &note, double time, const EnvelopeShape &shape)
{
    const double t = time - note.time;
    if (t < 0.0)
        return 0.0f;

    // Level while the note is held: attack, then decay towards sustain
    auto heldLevel = [&](double u) {
        if (u < shape.attack)
            return float(u / std::max(shape.attack, 1e-4f));
        const double d = (u - shape.attack) / std::max(shape.decay, 1e-4f);
        return float(shape.sustain + (1.0 - shape.sustain) * std::exp(-3.0 * d));
    };

    float level;
    if (t < note.duration)
    {
        level = heldLevel(t);
    }
    else
    {
        const double sinceRelease = t - note.duration;
        if (sinceRelease >= shape.release)
            return 0.0f;
        level = heldLevel(note.duration) * float(1.0 - sinceRelease / std::max(shape.release, 1e-4f));
    }

    return shape.useVelocity ? level * note.velocity : level;
}

float demo::trackEnvelope(const SongTrack &track, double time, const EnvelopeShape &shape)
{
    // Only notes that started within the longest note plus release can still sound
    const double window = track.maxDuration + shape.release;
    size_t i = track.firstNoteAtOrAfter(std::nextafter(time, 1e300));

    float value = 0.0f;
    while (i > 0)
    {
        const NoteEvent &note = track.notes[--i];
        if (note.time < time - window)
            break;
        value = std::max(value, noteEnvelope(note, time, shape));
    }
    return value;
}

SongEnvelope::SongEnvelope(const SongData &song, const std::vector<std::string> &trackNames, EnvelopeShape shape)
    : m_shape(shape)
{
    for (const auto &name : trackNames)
        if (const SongTrack *track = song.track(name))
            m_tracks.push_back(track);
}

float SongEnvelope::value(double time) const
{
    float value = 0.0f;
    for (const SongTrack *track : m_tracks)
        value = std::max(value, trackEnvelope(*track, time, m_shape));
    return value;
}

std::vector<NoteEvent> SongEnvelope::notesBetween(double from, double to) const
{
    std::vector<NoteEvent> result;
    for (const SongTrack *track : m_tracks)
        for (size_t i = track->firstNoteAtOrAfter(from); i < track->notes.size() && track->notes[i].time < to; ++i)
            result.push_back(track->notes[i]);
    std::sort(result.begin(), result.end(), [](const NoteEvent &a, const NoteEvent &b) { return a.time < b.time; });
    return result;
}
