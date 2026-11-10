#pragma once

/**
 * @file demo_window.h
 * @brief Base window for music demos: song clock, transport controls and show mode.
 */

#include "demo_config.h"
#include "guided_camera.h"
#include "music_clock.h"
#include "param_timeline.h"
#include "song_data.h"

#include <ivfui/glfw_application.h>
#include <ivfui/glfw_scene_window.h>

#include <memory>
#include <optional>

namespace demo {

/**
 * @class DemoWindow
 * @brief Scene window driven by the song: derive from it to make a demo.
 *
 * Owns the MusicClock, advances it every frame and publishes the song time to
 * all shaders as the global uniforms `time` (seconds), `beat` (fractional
 * beats) and `beatPhase` ([0, 1) within the current beat).
 *
 * With a song file (DemoConfig::songFile) the note events of every track are
 * available through song(), and the tempo is taken from it. With a timeline
 * file (DemoConfig::timelineFile) keyframed parameters are available through
 * param(); the file is reloaded when it changes while the demo runs.
 *
 * In development mode a transport bar shows the song position, bar, beat and
 * current marker, and allows play/pause and scrubbing. Keys: P play/pause,
 * Left/Right seek 5 s (Shift: one bar), PgUp/PgDn previous/next marker, Home
 * restart, M mute, F1 toggle the overlay, F2 the song track monitor, F3 the
 * timeline panel, F4 switch between the guided and the free orbit camera.
 * Show mode (--show) hides all development UI.
 *
 * The guided camera follows cameraPathAt(), by default the timeline parameters
 * `cameraPos` and `cameraTarget` (use "spline" keys for smooth motion). The
 * viewer can look around by dragging with the left mouse button and float away
 * from the path with the right button and the wheel; the camera returns to the
 * path when left alone.
 *
 * Demos implement onDemoSetup() to build the scene and onDemoUpdate() to
 * animate it from the song time.
 */
class DemoWindow : public ivfui::GLFWSceneWindow {
public:
    explicit DemoWindow(const DemoConfig &config);

    [[nodiscard]] MusicClock &clock() { return m_clock; }
    [[nodiscard]] const SongData &song() const { return m_song; }
    [[nodiscard]] const ParamTimeline &timeline() const { return m_timeline; }
    [[nodiscard]] const DemoConfig &config() const { return m_config; }

    /** @brief Current musical position in bars: 1.0 is the start of the song. */
    [[nodiscard]] double barPosition() const { return m_clock.bars() + 1.0; }

    /** @brief Musical position in bars of any song time. */
    [[nodiscard]] double barAt(double time) const { return m_clock.beatsAt(time) / m_clock.beatsPerBar() + 1.0; }

    /** @brief Timeline parameter at the current position, @p fallback if it is not defined. */
    [[nodiscard]] float param(std::string_view name, float fallback = 0.0f) const
    {
        return m_timeline.value(name, barPosition(), fallback);
    }
    [[nodiscard]] glm::vec3 param3(std::string_view name, glm::vec3 fallback = glm::vec3(0.0f)) const
    {
        return m_timeline.vec3(name, barPosition(), fallback);
    }
    [[nodiscard]] bool showMode() const { return m_config.showMode; }

    [[nodiscard]] GuidedCamera &guidedCamera() { return m_guidedCamera; }
    [[nodiscard]] bool freeCamera() const { return m_freeCamera; }

    /**
     * @brief The authored camera path at a song time, without viewer input.
     *
     * Must be a pure function of @p time. The default reads the timeline
     * parameters `cameraPos` and `cameraTarget`; returns nothing when there is no
     * path, and the camera is then left to the demo.
     */
    [[nodiscard]] virtual std::optional<CameraPose> cameraPathAt(double time) const;

protected:
    /** @brief Build the scene. Return non-zero to abort. */
    virtual int onDemoSetup() = 0;

    /**
     * @brief Animate the scene. Called every frame after the clock has advanced.
     * @param time Song time in seconds.
     * @param dt Frame time in seconds.
     */
    virtual void onDemoUpdate(double time, double dt) {}

    /** @brief Draw demo-specific development UI (not called in show mode). */
    virtual void onDemoUi() {}

    int onSetup() final;
    void onUpdate() final;
    void onDrawUi() final;
    void onKey(int key, int scancode, int action, int mods) override;
    void onFrameExportStarted() override;
    void onMouseDrag(double x, double y, double dx, double dy, int button) override;
    void onScroll(double xoffset, double yoffset) override;

private:
    void updateCamera(double dt);
    void drawTransport();
    void drawSongMonitor();
    void drawTimelinePanel();
    void seekBy(double seconds);
    void seekToBar(double bar);
    void seekToMarker(int direction);
    void toggleMute();
    int sectionIndexAt(double time) const;

    DemoConfig m_config;
    MusicClock m_clock;
    SongData m_song;
    ParamTimeline m_timeline;
    std::string m_timelineError;
    double m_reloadTimer{0.0};
    bool m_showOverlay{true};
    bool m_holdClock{false}; ///< Keep the clock for one frame, so the first exported frame is at startTime.
    bool m_showSongMonitor{false};
    bool m_showTimelinePanel{false};
    float m_volumeBeforeMute{1.0f};
    GuidedCamera m_guidedCamera;
    bool m_freeCamera{false};
};

/**
 * @brief Parse the command line, open the demo window and run it.
 * @tparam Window A DemoWindow subclass constructible from a DemoConfig.
 * @param config Defaults of the demo, overridden by command-line arguments.
 * @return Process exit code.
 */
template <typename Window> int runDemo(DemoConfig config, int argc, char **argv)
{
#ifdef DEMO_ASSET_SOURCE_DIR
    // Defined by add_demo() for the demo's own sources
    if (config.assetSourceDir.empty())
        config.assetSourceDir = DEMO_ASSET_SOURCE_DIR;
#endif

    switch (config.parseArgs(argc, argv))
    {
    case ArgsResult::Exit:
        return 0;
    case ArgsResult::Error:
        return 1;
    case ArgsResult::Run:
        break;
    }

    auto app = ivfui::GLFWApplication::create();
    app->hint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    app->hint(GLFW_CONTEXT_VERSION_MINOR, 3);
    app->hint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    app->hint(GLFW_SAMPLES, 4);

    auto window = std::make_shared<Window>(config);
    app->addWindow(window);
    return app->loop();
}

} // namespace demo
