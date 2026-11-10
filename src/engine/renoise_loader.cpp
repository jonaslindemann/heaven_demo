#include "renoise_loader.h"

#include <ivf/logger.h>

#include <miniz.h>
#include <pugixml.hpp>

#include <algorithm>
#include <cstring>
#include <set>
#include <string_view>

using namespace demo;

namespace {

bool readZipEntry(const std::string &path, const char *entry, std::string &content, std::string *error)
{
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_file(&zip, path.c_str(), 0))
    {
        if (error)
            *error = "Cannot open " + path;
        return false;
    }

    size_t size = 0;
    void *data = mz_zip_reader_extract_file_to_heap(&zip, entry, &size, 0);
    if (data)
    {
        content.assign(static_cast<const char *>(data), size);
        mz_free(data);
    }
    else if (error)
    {
        *error = std::string("No ") + entry + " in " + path;
    }
    mz_zip_reader_end(&zip);
    return data != nullptr;
}

int hexValue(std::string_view text)
{
    if (text.empty())
        return -1;
    int value = 0;
    for (char c : text)
    {
        int digit;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'A' && c <= 'F')
            digit = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
            digit = c - 'a' + 10;
        else
            return -1;
        value = value * 16 + digit;
    }
    return value;
}

// "C-4", "C#4" -> MIDI-style number with C-0 = 0 (Renoise numbering), -1 if not a note
int parsePitch(std::string_view note)
{
    if (note.size() != 3 || note[2] < '0' || note[2] > '9')
        return -1;
    static constexpr std::string_view names = "C-C#D-D#E-F-F#G-G#A-A#B-";
    for (int semitone = 0; semitone < 12; ++semitone)
        if (names.substr(semitone * 2, 2) == note.substr(0, 2))
            return (note[2] - '0') * 12 + semitone;
    return -1;
}

} // namespace

bool demo::loadRenoiseSong(const std::string &path, SongData &song, std::string *error)
{
    std::string xml;
    if (!readZipEntry(path, "Song.xml", xml, error))
        return false;

    pugi::xml_document doc;
    if (auto result = doc.load_buffer(xml.data(), xml.size()); !result)
    {
        if (error)
            *error = std::string("Invalid Song.xml: ") + result.description();
        return false;
    }

    const pugi::xml_node root = doc.child("RenoiseSong");
    const pugi::xml_node global = root.child("GlobalSongData");
    const double bpm = global.child("BeatsPerMin").text().as_double(120.0);
    const int linesPerBeat = global.child("LinesPerBeat").text().as_int(4);
    const double secondsPerLine = 60.0 / (bpm * linesPerBeat);

    song = SongData();
    song.m_bpm = bpm;
    song.m_beatsPerBar = global.child("SignatureNumerator").text().as_int(4);

    // Track list; only sequencer tracks hold notes, but all occupy a slot in each pattern
    std::vector<bool> isNoteTrack;
    std::vector<SongTrack> tracks;
    for (pugi::xml_node track : root.child("Tracks").children())
    {
        isNoteTrack.push_back(std::strcmp(track.name(), "SequencerTrack") == 0);
        tracks.push_back(SongTrack{track.child_value("Name"), {}, 0.0});
    }

    std::vector<pugi::xml_node> patterns;
    for (pugi::xml_node pattern : root.child("PatternPool").child("Patterns").children("Pattern"))
        patterns.push_back(pattern);

    // Per track and note column: index of the note that is still sounding, or -1
    std::vector<std::vector<long>> sounding(tracks.size());
    std::set<std::string> tempoWarnings;

    long songLine = 0;
    for (pugi::xml_node entry : root.child("PatternSequence").child("SequenceEntries").children("SequenceEntry"))
    {
        const int patternIndex = entry.child("Pattern").text().as_int(-1);
        if (patternIndex < 0 || patternIndex >= int(patterns.size()))
        {
            if (error)
                *error = "Sequence refers to missing pattern " + std::to_string(patternIndex);
            return false;
        }
        const pugi::xml_node pattern = patterns[patternIndex];
        const int numLines = pattern.child("NumberOfLines").text().as_int(64);

        std::set<int> muted;
        for (pugi::xml_node m : entry.child("MutedTracks").children("MutedTrack"))
            muted.insert(m.text().as_int());

        song.m_sections.push_back(
            SongSection{entry.child_value("SectionName"), songLine * secondsPerLine, (songLine + numLines) * secondsPerLine});

        size_t trackIndex = 0;
        for (pugi::xml_node patternTrack : pattern.child("Tracks").children())
        {
            for (pugi::xml_node line : patternTrack.child("Lines").children("Line"))
            {
                const int lineIndex = line.attribute("index").as_int();
                if (lineIndex >= numLines)
                    continue;

                for (pugi::xml_node fx : line.child("EffectColumns").children("EffectColumn"))
                {
                    const std::string number = fx.child_value("Number");
                    const int value = hexValue(fx.child_value("Value"));
                    if ((number == "ZT" && value != int(bpm)) || (number == "ZL" && value != linesPerBeat))
                        tempoWarnings.insert(number + fx.child_value("Value"));
                }

                if (trackIndex >= tracks.size() || !isNoteTrack[trackIndex] || muted.count(int(trackIndex)))
                    continue;

                SongTrack &track = tracks[trackIndex];
                auto &open = sounding[trackIndex];

                int column = 0;
                for (pugi::xml_node noteColumn : line.child("NoteColumns").children("NoteColumn"))
                {
                    const std::string note = noteColumn.child_value("Note");
                    if (!note.empty())
                    {
                        if (open.size() <= size_t(column))
                            open.resize(column + 1, -1);

                        const int delay = hexValue(noteColumn.child_value("Delay"));
                        const double time =
                            (songLine + lineIndex + (delay > 0 ? delay / 256.0 : 0.0)) * secondsPerLine;

                        // A note-off or a new note ends the note sounding in this column
                        if (open[column] >= 0)
                        {
                            NoteEvent &previous = track.notes[open[column]];
                            previous.duration = std::max(time - previous.time, 0.0);
                            open[column] = -1;
                        }

                        const int pitch = parsePitch(note);
                        if (pitch >= 0)
                        {
                            // The volume column may also hold effects; only 00-80 is a volume
                            const int volume = hexValue(noteColumn.child_value("Volume"));
                            const float velocity = (volume >= 0 && volume <= 0x80) ? volume / 128.0f : 1.0f;
                            open[column] = long(track.notes.size());
                            track.notes.push_back(NoteEvent{time, 0.0, pitch, velocity, column});
                        }
                    }
                    ++column;
                }
            }
            ++trackIndex;
        }
        songLine += numLines;
    }

    song.m_length = songLine * secondsPerLine;

    for (size_t t = 0; t < tracks.size(); ++t)
    {
        SongTrack &track = tracks[t];
        // Notes still sounding at the end last until the song ends
        for (long index : sounding[t])
            if (index >= 0)
                track.notes[index].duration = song.m_length - track.notes[index].time;

        std::stable_sort(track.notes.begin(), track.notes.end(),
                         [](const NoteEvent &a, const NoteEvent &b) { return a.time < b.time; });
        for (const NoteEvent &note : track.notes)
            track.maxDuration = std::max(track.maxDuration, note.duration);

        if (isNoteTrack[t])
            song.m_tracks.push_back(std::move(track));
    }

    for (const auto &warning : tempoWarnings)
        ivf::logWarning("Tempo change " + warning + " in " + path + " is not supported; using " +
                            std::to_string(int(bpm)) + " BPM",
                        "Renoise");

    return true;
}
