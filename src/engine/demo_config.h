#pragma once

/**
 * @file demo_config.h
 * @brief Start-up settings of a demo, overridable from the command line.
 */

#include <string>

namespace demo {

/** @brief Outcome of DemoConfig::parseArgs(). */
enum class ArgsResult {
    Run,  ///< Start the demo.
    Exit, ///< Exit successfully, e.g. after --help.
    Error ///< Invalid arguments.
};

/**
 * @struct DemoConfig
 * @brief Settings a demo starts with.
 *
 * A demo fills in its defaults (title, song, tempo) and then calls parseArgs()
 * so they can be overridden from the command line.
 */
struct DemoConfig {
    std::string title{"Demo"};
    int width{1280};
    int height{720};
    bool fullscreen{false};

    /// Show mode: no development UI, plays the song from startTime.
    bool showMode{false};

    std::string audioFile;     ///< Rendered song, relative to the working directory.
    std::string songFile;      ///< Composition with note events (.xrns), optional.
    std::string timelineFile;  ///< Parameter timeline (.json), optional; reloaded when changed.

    /// Project asset directory. Paths starting with "assets/" are read from here
    /// when the file exists, so edits in the project are picked up without a rebuild.
    std::string assetSourceDir;
    double bpm{0.0};           ///< Tempo for beats and bars; 0 takes it from the song file (or 120).
    double firstBeat{0.0};     ///< Song time of the first downbeat in seconds.
    int beatsPerBar{4};

    double startTime{0.0};     ///< Song time to start at in seconds.
    bool autoplay{true};       ///< Start playing immediately.
    float volume{1.0f};

    /// Frame export: when set, renders a PNG sequence into this directory at
    /// exportFps in fixed time steps (no development UI, silent), then exits.
    std::string exportDir;
    int exportFps{60};
    double endTime{0.0};       ///< Song time to stop exporting at in seconds; 0 is the end of the song.

    /**
     * @brief Override settings from command-line arguments.
     *
     * Supported: --fullscreen, --show, --size WxH, --start <seconds>,
     * --end <seconds>, --export <dir>, --fps <fps>, --paused, --mute,
     * --bpm <bpm>, --help.
     */
    ArgsResult parseArgs(int argc, char **argv);

    /**
     * @brief Resolve an asset path, preferring the project copy in assetSourceDir.
     * @param path Path such as "assets/song.wav", relative to the executable directory.
     */
    [[nodiscard]] std::string resolve(const std::string &path) const;

    /** @brief Print the supported command-line arguments. */
    static void printUsage(const char *program);
};

} // namespace demo
