#include "demo_window.h"

#include <imgui.h>
#include <ivf/logger.h>
#include <ivf/shader_material.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

using namespace demo;

namespace {

std::string formatTime(double seconds)
{
    seconds = std::max(seconds, 0.0);
    const int minutes = int(seconds / 60.0);
    char text[32];
    std::snprintf(text, sizeof(text), "%02d:%06.3f", minutes, seconds - minutes * 60.0);
    return text;
}

GLFWmonitor *monitorFor(const DemoConfig &config)
{
    return config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
}

} // namespace

DemoWindow::DemoWindow(const DemoConfig &config)
    : GLFWSceneWindow(config.width, config.height, config.title, monitorFor(config)), m_config(config)
{}

int DemoWindow::onSetup()
{
    // Keep development panels inside the demo window instead of letting ImGui
    // open separate OS windows (which also keeps panel positions window-relative)
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

    m_clock.load(m_config.resolve(m_config.audioFile));

    double bpm = m_config.bpm;
    int beatsPerBar = m_config.beatsPerBar;
    if (!m_config.songFile.empty())
    {
        std::string error;
        if (m_song.load(m_config.resolve(m_config.songFile), &error))
        {
            ivf::logInfo("Loaded " + m_config.songFile + ": " + std::to_string(m_song.tracks().size()) +
                             " tracks, " + std::to_string(m_song.bpm()) + " BPM",
                         "DemoWindow");
            if (bpm <= 0.0)
            {
                bpm = m_song.bpm();
                beatsPerBar = m_song.beatsPerBar();
            }
            if (m_clock.hasAudio() && std::abs(m_song.length() - m_clock.length()) > 0.5)
                ivf::logWarning("Song file is " + std::to_string(m_song.length()) + " s but the audio is " +
                                    std::to_string(m_clock.length()) + " s",
                                "DemoWindow");
        }
        else
        {
            ivf::logError(error, "DemoWindow");
        }
    }
    m_clock.setTempo(bpm > 0.0 ? bpm : 120.0, m_config.firstBeat, beatsPerBar);
    m_clock.setVolume(m_config.volume);

    if (!m_config.timelineFile.empty())
    {
        const std::string path = m_config.resolve(m_config.timelineFile);
        if (m_timeline.loadFile(path, &m_timelineError))
            ivf::logInfo("Loaded timeline " + path, "DemoWindow");
        else
            ivf::logError(m_timelineError, "DemoWindow");
    }

    if (m_config.showMode || !m_config.exportDir.empty())
    {
        m_showOverlay = false;
        hideMainMenu();
    }

    if (int result = onDemoSetup(); result != 0)
        return result;

    // The guided camera owns the view; F4 hands it to the orbit manipulator
    cameraManipulator()->setManipulationBlocked(true);

    m_clock.seek(m_config.startTime);

    if (!m_config.exportDir.empty())
    {
        // Silent, in fixed steps of one video frame, from startTime to endTime
        const double end = m_config.endTime > 0.0 ? m_config.endTime : m_clock.length();
        if (end <= m_config.startTime)
        {
            ivf::logError("Nothing to export: set --end after --start", "DemoWindow");
            return 1;
        }
        m_clock.setFreeRunning(true);
        m_clock.play();

        ivfui::GLFWWindow::FrameExportSettings settings;
        settings.directory = m_config.exportDir;
        settings.fps = m_config.exportFps;
        settings.frameCount = int(std::ceil((end - m_config.startTime) * m_config.exportFps));
        settings.closeWhenDone = true;
        startFrameExport(settings);
        return 0;
    }

    if (m_config.autoplay)
        m_clock.play();
    return 0;
}

void DemoWindow::onFrameExportStarted()
{
    m_holdClock = true;
}

void DemoWindow::onUpdate()
{
    const double dt = frameTime();
    if (m_holdClock)
        m_holdClock = false;
    else
        m_clock.update(dt);

    // Pick up edits of the timeline file while developing
    if (!m_config.showMode && (m_reloadTimer += dt) >= 0.5)
    {
        m_reloadTimer = 0.0;
        std::string error;
        if (m_timeline.reloadIfChanged(&error))
        {
            m_timelineError.clear();
            ivf::logInfo("Reloaded timeline " + m_timeline.path(), "DemoWindow");
        }
        else if (!error.empty())
        {
            m_timelineError = error;
            ivf::logError(error, "DemoWindow");
        }
    }

    ivf::ShaderMaterial::setGlobalUniform("time", float(m_clock.time()));
    ivf::ShaderMaterial::setGlobalUniform("beat", float(m_clock.beats()));
    ivf::ShaderMaterial::setGlobalUniform("beatPhase", float(m_clock.beatPhase()));

    onDemoUpdate(m_clock.time(), dt);
    updateCamera(dt);
}

std::optional<CameraPose> DemoWindow::cameraPathAt(double time) const
{
    if (!m_timeline.has("cameraPos"))
        return std::nullopt;
    const double bar = barAt(time);
    CameraPose pose;
    pose.position = m_timeline.vec3("cameraPos", bar);
    pose.target = m_timeline.vec3("cameraTarget", bar, pose.position + glm::vec3(0.0f, 0.0f, -1.0f));
    return pose;
}

void DemoWindow::updateCamera(double dt)
{
    if (m_freeCamera)
        return;
    const auto path = cameraPathAt(m_clock.time());
    if (!path)
        return;
    m_guidedCamera.update(dt);
    const CameraPose pose = m_guidedCamera.apply(*path);
    cameraManipulator()->setCameraPosition(pose.position);
    cameraManipulator()->setCameraTarget(pose.target);
}

void DemoWindow::onMouseDrag(double x, double y, double dx, double dy, int button)
{
    GLFWSceneWindow::onMouseDrag(x, y, dx, dy, button);
    if (m_freeCamera || ImGui::GetIO().WantCaptureMouse)
        return;
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        m_guidedCamera.look(float(dx), float(dy));
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        m_guidedCamera.move(glm::vec3(float(dx), float(-dy), 0.0f) * m_guidedCamera.settings().moveSpeed);
}

void DemoWindow::onScroll(double xoffset, double yoffset)
{
    GLFWSceneWindow::onScroll(xoffset, yoffset);
    if (!m_freeCamera && !ImGui::GetIO().WantCaptureMouse)
        m_guidedCamera.move(glm::vec3(0.0f, 0.0f, float(yoffset) * 0.4f));
}

void DemoWindow::onDrawUi()
{
    if (m_config.showMode || !m_showOverlay)
        return;

    drawTransport();
    if (m_showSongMonitor)
        drawSongMonitor();
    if (m_showTimelinePanel)
        drawTimelinePanel();
    onDemoUi();
}

void DemoWindow::drawTimelinePanel()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos({viewport->WorkPos.x + viewport->WorkSize.x - 660.0f, viewport->WorkPos.y + 10.0f},
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({320.0f, 420.0f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Timeline", &m_showTimelinePanel);

    if (m_timeline.path().empty())
    {
        ImGui::TextUnformatted("No timeline file loaded");
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("%s", m_timeline.path().c_str());
    if (!m_timelineError.empty())
        ImGui::TextColored({1.0f, 0.4f, 0.3f, 1.0f}, "%s", m_timelineError.c_str());

    const double bar = barPosition();
    if (ImGui::CollapsingHeader("Markers", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const int current = m_timeline.markerIndexAt(bar);
        for (int i = 0; i < int(m_timeline.markers().size()); ++i)
        {
            const auto &marker = m_timeline.markers()[i];
            char label[160];
            std::snprintf(label, sizeof(label), "%5.1f  %s##marker%d", marker.bar, marker.name.c_str(), i);
            if (ImGui::Selectable(label, i == current))
                seekToBar(marker.bar);
        }
    }

    if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (const auto &[name, keys] : m_timeline.params())
        {
            const glm::vec4 v = m_timeline.vec4(name, bar);
            switch (m_timeline.components(name))
            {
            case 1:
                ImGui::Text("%-16s %.3f", name.c_str(), v.x);
                break;
            case 2:
                ImGui::Text("%-16s %.3f %.3f", name.c_str(), v.x, v.y);
                break;
            case 3:
                ImGui::Text("%-16s %.3f %.3f %.3f", name.c_str(), v.x, v.y, v.z);
                break;
            default:
                ImGui::Text("%-16s %.3f %.3f %.3f %.3f", name.c_str(), v.x, v.y, v.z, v.w);
                break;
            }
        }
    }
    ImGui::End();
}

void DemoWindow::seekToBar(double bar)
{
    m_clock.seek(m_clock.timeAtBar(bar - 1.0));
}

void DemoWindow::seekToMarker(int direction)
{
    const auto &markers = m_timeline.markers();
    if (markers.empty())
        return;

    // Jumping back from just after a marker goes to the one before it, so it can be stepped through
    const double bar = barPosition();
    int index = m_timeline.markerIndexAt(bar);
    if (direction > 0)
        index = std::min(index + 1, int(markers.size()) - 1);
    else if (index >= 0 && bar - markers[index].bar < 0.25)
        index = std::max(index - 1, 0);
    else
        index = std::max(index, 0);
    seekToBar(markers[index].bar);
}

int DemoWindow::sectionIndexAt(double time) const
{
    const auto &sections = m_song.sections();
    for (size_t i = 0; i < sections.size(); ++i)
        if (time < sections[i].end)
            return int(i);
    return sections.empty() ? -1 : int(sections.size()) - 1;
}

void DemoWindow::drawSongMonitor()
{
    ImGui::SetNextWindowPos({ImGui::GetMainViewport()->WorkPos.x + ImGui::GetMainViewport()->WorkSize.x - 330.0f,
                             ImGui::GetMainViewport()->WorkPos.y + 10.0f},
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({320.0f, 0.0f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Song", &m_showSongMonitor);

    if (m_song.empty())
    {
        ImGui::TextUnformatted("No song file loaded");
        ImGui::End();
        return;
    }

    const double time = m_clock.time();
    if (int section = sectionIndexAt(time); section >= 0)
        ImGui::Text("Sequence slot %d: %s", section, m_song.sections()[section].name.c_str());

    // Short, sharp shape so individual notes are visible
    const EnvelopeShape shape{0.005f, 0.25f, 0.3f, 0.15f, true};
    for (const auto &track : m_song.tracks())
    {
        const float value = trackEnvelope(track, time, shape);
        ImGui::ProgressBar(value, {140.0f, 0.0f}, "");
        ImGui::SameLine();
        ImGui::TextUnformatted(track.name.c_str());
    }
    ImGui::End();
}

void DemoWindow::drawTransport()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const float margin = 10.0f;
    ImGui::SetNextWindowPos({viewport->WorkPos.x + margin, viewport->WorkPos.y + viewport->WorkSize.y - margin},
                            ImGuiCond_Always, {0.0f, 1.0f});
    ImGui::SetNextWindowSize({viewport->WorkSize.x - 2.0f * margin, 0.0f}, ImGuiCond_Always);
    ImGui::Begin("Transport", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button(m_clock.isPlaying() ? "Pause" : "Play", {60.0f, 0.0f}))
        m_clock.togglePlay();
    ImGui::SameLine();
    if (ImGui::Button("|<"))
        m_clock.seek(0.0);

    ImGui::SameLine();
    const double bars = m_clock.bars();
    const int bar = int(std::floor(bars));
    const int beat = int(std::floor((bars - bar) * m_clock.beatsPerBar()));
    ImGui::Text("%s / %s   bar %3d.%d   %.1f bpm", formatTime(m_clock.time()).c_str(),
                formatTime(m_clock.length()).c_str(), bar + 1, beat + 1, m_clock.bpm());

    if (int marker = m_timeline.markerIndexAt(barPosition()); marker >= 0)
    {
        ImGui::SameLine();
        ImGui::TextColored({0.6f, 0.85f, 1.0f, 1.0f}, "  %s", m_timeline.markers()[marker].name.c_str());
    }
    if (!m_timelineError.empty())
    {
        ImGui::SameLine();
        ImGui::TextColored({1.0f, 0.4f, 0.3f, 1.0f}, "  timeline error (F3)");
    }

    ImGui::SameLine();
    bool muted = m_clock.volume() == 0.0f;
    if (ImGui::Checkbox("Mute", &muted))
        toggleMute();

    ImGui::SameLine();
    ImGui::Text("%5.1f fps%s", ImGui::GetIO().Framerate, m_clock.hasAudio() ? "" : "   (no audio)");

    // Scrubbing seeks continuously while dragging
    float position = float(m_clock.time());
    const float length = float(std::max(m_clock.length(), 1.0));
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::SliderFloat("##position", &position, 0.0f, length, ""))
        m_clock.seek(position);

    ImGui::End();
}

void DemoWindow::seekBy(double seconds)
{
    m_clock.seek(m_clock.time() + seconds);
}

void DemoWindow::toggleMute()
{
    if (m_clock.volume() > 0.0f)
    {
        m_volumeBeforeMute = m_clock.volume();
        m_clock.setVolume(0.0f);
    }
    else
    {
        m_clock.setVolume(m_volumeBeforeMute > 0.0f ? m_volumeBeforeMute : 1.0f);
    }
}

void DemoWindow::onKey(int key, int scancode, int action, int mods)
{
    GLFWSceneWindow::onKey(key, scancode, action, mods);
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;
    if (!m_config.exportDir.empty())
        return; // transport and camera keys would change the exported frames

    const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    const double barLength = m_clock.timeAtBar(1.0) - m_clock.timeAtBar(0.0);

    switch (key)
    {
    case GLFW_KEY_P:
        if (action == GLFW_PRESS)
            m_clock.togglePlay();
        break;
    case GLFW_KEY_LEFT:
        seekBy(shift ? -barLength : -5.0);
        break;
    case GLFW_KEY_RIGHT:
        seekBy(shift ? barLength : 5.0);
        break;
    case GLFW_KEY_HOME:
        m_clock.seek(0.0);
        break;
    case GLFW_KEY_M:
        if (action == GLFW_PRESS)
            toggleMute();
        break;
    case GLFW_KEY_F1:
        if (action == GLFW_PRESS && !m_config.showMode)
            m_showOverlay = !m_showOverlay;
        break;
    case GLFW_KEY_F2:
        if (action == GLFW_PRESS && !m_config.showMode)
            m_showSongMonitor = !m_showSongMonitor;
        break;
    case GLFW_KEY_F3:
        if (action == GLFW_PRESS && !m_config.showMode)
            m_showTimelinePanel = !m_showTimelinePanel;
        break;
    case GLFW_KEY_F4:
        if (action == GLFW_PRESS && !m_config.showMode)
        {
            m_freeCamera = !m_freeCamera;
            m_guidedCamera.reset();
            cameraManipulator()->setManipulationBlocked(!m_freeCamera);
        }
        break;
    case GLFW_KEY_PAGE_UP:
        seekToMarker(-1);
        break;
    case GLFW_KEY_PAGE_DOWN:
        seekToMarker(1);
        break;
    default:
        break;
    }
}
