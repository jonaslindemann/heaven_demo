#pragma once

/**
 * @file renoise_loader.h
 * @brief Reads note events from a Renoise song (.xrns).
 */

#include "song_data.h"

#include <string>

namespace demo {

/**
 * @brief Load tempo, pattern sequence and notes of a Renoise song.
 *
 * Reads Song.xml from the .xrns archive. Each sequence entry becomes a
 * SongSection; every sequencer track with notes becomes a SongTrack. Note-offs
 * end notes, delay columns shift them within a line, and tracks muted in a
 * sequence slot are skipped. Tempo changes during the song are not supported.
 *
 * @param path Path to the .xrns file.
 * @param song Receives the song.
 * @param error Receives a description when loading fails.
 * @return true on success.
 */
bool loadRenoiseSong(const std::string &path, SongData &song, std::string *error);

} // namespace demo
