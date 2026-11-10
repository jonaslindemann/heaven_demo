#pragma once

/**
 * @file param_timeline.h
 * @brief Named parameters keyed by bar position, for continuous scene changes over a song.
 */

#include <glm/glm.hpp>

#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace demo {

/**
 * @brief Interpolation into a key from the previous one.
 */
enum class Ease {
    Linear,
    Step,   ///< Hold the previous value, jump at the key.
    Smooth, ///< Smoothstep.
    Sine,   ///< Sine in-out.
    In,     ///< Quadratic ease-in.
    Out,    ///< Quadratic ease-out.
    Cubic,  ///< Cubic in-out.
    Spline  ///< Smooth curve through the neighbouring keys (Catmull-Rom); keeps the speed continuous.
};

/** @brief Apply an easing curve to t in [0, 1]. Spline needs the neighbouring keys and acts as linear here. */
float applyEase(Ease ease, float t);

/**
 * @class ParamTimeline
 * @brief Keyframed parameters and named markers over the song, positioned in bars.
 *
 * Positions are musical bar numbers as counted by musicians: 1.0 is the start of
 * the song, 121.0 the start of bar 121, 121.5 halfway through it. Each key holds
 * a value (1 to 4 components) and the easing used to arrive at it from the
 * previous key. Before the first key a parameter holds the first value, after
 * the last key the last value.
 *
 * Runs of "spline" keys form one smooth curve, so motion such as a camera path
 * flows through its keys without stopping. The curve starts and ends at rest,
 * and also comes to rest at a key next to a segment of another easing.
 *
 * Usually loaded from a JSON file that can be reloaded while the demo runs:
 * @code{.json}
 * {
 *   "markers": [ { "bar": 1, "name": "Intro" }, { "bar": 17, "name": "Verse 1" } ],
 *   "params": {
 *     "fog":   [ [1, 0.12], [17, 0.06, "sine"] ],
 *     "sky":   [ [1, [0.01, 0.01, 0.03]], [121, [0.3, 0.1, 0.4], "smooth"] ]
 *   }
 * }
 * @endcode
 */
class ParamTimeline {
public:
    struct Key {
        double bar{1.0};
        glm::vec4 value{0.0f};
        Ease ease{Ease::Linear};
        int components{1}; ///< Number of components given for the value (1-4).
    };

    struct Marker {
        double bar{1.0};
        std::string name;
    };

    /**
     * @brief Load from a JSON file, replacing the current contents.
     *
     * On error the current contents are kept, so a typo while editing does not
     * reset a running demo.
     */
    bool loadFile(const std::string &path, std::string *error = nullptr);

    /**
     * @brief Reload the file loaded with loadFile() if it changed on disk.
     * @return true if it was reloaded successfully.
     */
    bool reloadIfChanged(std::string *error = nullptr);

    [[nodiscard]] const std::string &path() const { return m_path; }

    void clear();

    /** @brief Add or replace the key of @p name at @p bar. */
    void setKey(const std::string &name, double bar, glm::vec4 value, Ease ease = Ease::Linear, int components = 4);
    void setKey(const std::string &name, double bar, float value, Ease ease = Ease::Linear);

    /** @brief Largest number of components used by the keys of @p name, 0 if it does not exist. */
    [[nodiscard]] int components(std::string_view name) const;

    void addMarker(double bar, std::string name);

    [[nodiscard]] bool has(std::string_view name) const;

    /** @brief Interpolated value at a bar position; @p fallback if the parameter is missing. */
    [[nodiscard]] glm::vec4 vec4(std::string_view name, double bar, glm::vec4 fallback = glm::vec4(0.0f)) const;
    [[nodiscard]] glm::vec3 vec3(std::string_view name, double bar, glm::vec3 fallback = glm::vec3(0.0f)) const;
    [[nodiscard]] float value(std::string_view name, double bar, float fallback = 0.0f) const;

    /**
     * @brief Integral of a parameter's first component from @p fromBar to @p toBar, in value x bars.
     *
     * Turns a rate into an amount that is still a pure function of the position,
     * e.g. how far something has drifted while a "weightless" value was up.
     * Uses the trapezoid rule with steps of at most @p step bars.
     */
    [[nodiscard]] double integral(std::string_view name, double fromBar, double toBar, float fallback = 0.0f,
                                  double step = 1.0 / 32.0) const;

    [[nodiscard]] const std::vector<Marker> &markers() const { return m_markers; }

    /** @brief Index of the marker in effect at @p bar, -1 before the first. */
    [[nodiscard]] int markerIndexAt(double bar) const;

    [[nodiscard]] const std::map<std::string, std::vector<Key>, std::less<>> &params() const { return m_params; }

private:
    std::map<std::string, std::vector<Key>, std::less<>> m_params;
    std::vector<Marker> m_markers;

    std::string m_path;
    std::filesystem::file_time_type m_fileTime{};
};

} // namespace demo
