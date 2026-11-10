#pragma once

/**
 * @file song_data.h
 * @brief Note events of a song, loaded from the composition, and envelopes derived from them.
 */

#include <string>
#include <string_view>
#include <vector>

namespace demo {

/**
 * @struct NoteEvent
 * @brief One note of a track.
 */
struct NoteEvent {
    double time{0.0};     ///< Start in song seconds.
    double duration{0.0}; ///< Until note-off, the next note in the same column, or the song end.
    int pitch{60};        ///< MIDI note number (C-4 = 48 in Renoise notation, 60 = C-5).
    float velocity{1.0f}; ///< [0, 1].
    int column{0};        ///< Note column within the track.

    [[nodiscard]] double end() const { return time + duration; }
};

/**
 * @struct SongTrack
 * @brief A named track with its notes sorted by start time.
 */
struct SongTrack {
    std::string name;
    std::vector<NoteEvent> notes;
    double maxDuration{0.0}; ///< Longest note, bounds envelope lookups.

    /** @brief Index of the first note starting at or after @p time. */
    [[nodiscard]] size_t firstNoteAtOrAfter(double time) const;
};

/**
 * @struct SongSection
 * @brief A span of the song, e.g. one entry of a tracker's pattern sequence.
 */
struct SongSection {
    std::string name;
    double start{0.0};
    double end{0.0};
};

/**
 * @class SongData
 * @brief Tempo, structure and note events of a song.
 *
 * Loaded from the composition rather than analysed from audio, so note timing
 * is exact and every instrument is a separate track.
 */
class SongData {
public:
    /**
     * @brief Load a song file. Supported: Renoise (.xrns).
     * @param path File path.
     * @param error Receives a description when loading fails.
     * @return true on success.
     */
    bool load(const std::string &path, std::string *error = nullptr);

    [[nodiscard]] bool empty() const { return m_tracks.empty(); }

    [[nodiscard]] double bpm() const { return m_bpm; }
    [[nodiscard]] int beatsPerBar() const { return m_beatsPerBar; }
    [[nodiscard]] double length() const { return m_length; }

    [[nodiscard]] const std::vector<SongTrack> &tracks() const { return m_tracks; }
    [[nodiscard]] const std::vector<SongSection> &sections() const { return m_sections; }

    /** @brief Track by exact name, nullptr if there is none. */
    [[nodiscard]] const SongTrack *track(std::string_view name) const;

private:
    friend bool loadRenoiseSong(const std::string &path, SongData &song, std::string *error);

    double m_bpm{120.0};
    int m_beatsPerBar{4};
    double m_length{0.0};
    std::vector<SongTrack> m_tracks;
    std::vector<SongSection> m_sections;
};

/**
 * @struct EnvelopeShape
 * @brief How a note becomes a smooth control value over time.
 *
 * A note rises over @c attack seconds, falls over @c decay seconds towards
 * @c sustain while held, and fades over @c release seconds after it ends.
 * Long attack and release give the "breathing" response the visuals need;
 * short ones give sharp hits.
 */
struct EnvelopeShape {
    float attack{0.02f};
    float decay{0.3f};
    float sustain{0.0f};
    float release{0.3f};
    bool useVelocity{true}; ///< Scale by note velocity.
};

/**
 * @brief Envelope value of one note at @p time, in [0, 1].
 */
float noteEnvelope(const NoteEvent &note, double time, const EnvelopeShape &shape);

/**
 * @brief Envelope of a track at @p time: the maximum over its sounding notes.
 *
 * A pure function of time, so it is exact under scrubbing and frame export.
 */
float trackEnvelope(const SongTrack &track, double time, const EnvelopeShape &shape);

/**
 * @class SongEnvelope
 * @brief Control value from one or more tracks, e.g. "bass" from the Kick and Bass tracks.
 */
class SongEnvelope {
public:
    SongEnvelope() = default;

    /**
     * @brief Combine the named tracks of a song. Unknown names are ignored.
     * @param song Song that outlives the envelope.
     * @param trackNames Tracks to combine (maximum of their envelopes).
     * @param shape Envelope shape.
     */
    SongEnvelope(const SongData &song, const std::vector<std::string> &trackNames, EnvelopeShape shape);

    /** @brief Value at a song time, in [0, 1]. */
    [[nodiscard]] float value(double time) const;

    /** @brief Notes of all tracks starting in [from, to), e.g. to spawn effects per note. */
    [[nodiscard]] std::vector<NoteEvent> notesBetween(double from, double to) const;

    [[nodiscard]] EnvelopeShape &shape() { return m_shape; }
    [[nodiscard]] bool empty() const { return m_tracks.empty(); }

private:
    std::vector<const SongTrack *> m_tracks;
    EnvelopeShape m_shape;
};

} // namespace demo
