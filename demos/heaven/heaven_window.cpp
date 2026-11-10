#include "heaven_window.h"

#include <imgui.h>
#include <ivf/shader_material.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

using namespace heaven;
using namespace ivf;

HeavenWindow::HeavenWindow(const demo::DemoConfig &config) : DemoWindow(config) {}

int HeavenWindow::onDemoSetup()
{
    // Track groups of the Renoise song and how each responds:
    // sharp hits for rhythm, slow breathing for pads
    m_bass = demo::SongEnvelope(song(), {"Kick", "Bass"}, {0.01f, 0.25f, 0.2f, 0.2f, true});
    m_pads = demo::SongEnvelope(song(), {"Pad", "Choir"}, {0.6f, 1.0f, 0.8f, 1.5f, true});
    m_lead = demo::SongEnvelope(song(), {"Lead", "Bell"}, {0.05f, 0.6f, 0.4f, 0.6f, true});
    m_arp = demo::SongEnvelope(song(), {"Arp A (ackord)", "Arp B (ackord)"}, {0.005f, 0.15f, 0.0f, 0.1f, true});
    m_kick = demo::SongEnvelope(song(), {"Kick"}, {});
    m_leadNotes = demo::SongEnvelope(song(), {"Lead"}, {});
    m_bellNotes = demo::SongEnvelope(song(), {"Bell"}, {});
    // Chorus-only hits: they make the islands' veins and the giant stalks pulse, the clouds flash
    m_stab = demo::SongEnvelope(song(), {"Stab"}, {0.005f, 0.18f, 0.0f, 0.1f, true});
    m_clap = demo::SongEnvelope(song(), {"Clap"}, {0.005f, 0.25f, 0.0f, 0.15f, true});

    // Night sky behind everything; added first so all later geometry covers it
    auto sky = Sphere::create(300.0, 64, 32);
    m_sky = demo::BlendMaterial::createFromFiles(config().resolve("assets/shaders/sky.vert"),
                                                 config().resolve("assets/shaders/sky.frag"), "heaven_sky",
                                                 demo::BlendMaterial::Mode::Background, true);
    if (!m_sky)
        return -1;
    sky->setMaterial(m_sky);
    add(sky);

    // The lake: a still mirror that the bass sends rings across
    // Reaches beyond the far plane, so its edge is never seen
    auto lake = Plane::create(1000.0, 1000.0, 4, 4);
    add(lake);
    auto reflection = PlanarReflection::create({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    reflection->addHiddenNode(lake);
    reflection->setResolutionScale(0.75f);
    addReflection(reflection);
    m_lake = LakeMaterial::createFromFiles(config().resolve("assets/shaders/lake.vert"),
                                           config().resolve("assets/shaders/lake.frag"), reflection, true);
    if (!m_lake)
        return -1;
    lake->setMaterial(m_lake);

    // Keep the camera's path through the garden free of flowers
    std::vector<glm::vec2> path;
    for (double bar = 1.0; bar < 121.0; bar += 0.25)
        if (const auto pose = DemoWindow::cameraPathAt(clock().timeAtBar(bar - 1.0)))
            path.emplace_back(pose->position.x, pose->position.z);
    auto nearPath = [&path](glm::vec2 p) {
        for (const auto &q : path)
            if (glm::distance(p, q) < 2.6f)
                return true;
        return false;
    };
    if (!m_flowers.create(config().resolve("assets/shaders/flower.vert"),
                          config().resolve("assets/shaders/flower.frag"), nearPath))
        return -1;

    // Chorus world: floating islands, giant stalks and clouds. Islands keep clear of
    // where the camera flies high (Act III), by more than their own size.
    // (Act IV is left out: the islands have sunk by the time the camera comes down.)
    auto highPathNear = [this](glm::vec2 p, float radius) {
        for (double bar = 1.0; bar < 137.0; bar += 0.25)
            if (const auto pose = DemoWindow::cameraPathAt(clock().timeAtBar(bar - 1.0)))
                if (pose->position.y > 4.0f &&
                    glm::distance(p, glm::vec2(pose->position.x, pose->position.z)) < radius + 6.0f)
                    return true;
        return false;
    };
    if (!m_islands.create(config().resolve("assets/shaders/island.vert"), config().resolve("assets/shaders/island.frag"),
                          config().resolve("assets/shaders/vine.vert"), config().resolve("assets/shaders/trail.frag"),
                          highPathNear))
        return -1;
    m_flowers.addIslandFlowers(m_islands.flowerSpots());
    if (!m_giants.create(config().resolve("assets/shaders/giant.vert"), config().resolve("assets/shaders/giant.frag"), 9))
        return -1;
    if (!m_clouds.create(config().resolve("assets/shaders/cloud.vert"), config().resolve("assets/shaders/cloud.frag"),
                         48))
        return -1;
    for (const auto &node : m_flowers.nodes())
        add(node);
    add(m_islands.rockNode());
    add(m_giants.node());

    // Blended, so added after the opaque geometry
    add(m_clouds.node());
    add(m_islands.vineNode());
    // Act IV: sparks rising from the dissolving flowers
    if (!m_embers.create(config().resolve("assets/shaders/ember.vert"), config().resolve("assets/shaders/mote.frag"),
                         m_flowers.placements(), 20))
        return -1;
    add(m_embers.node());
    if (!m_motes.create(config().resolve("assets/shaders/mote.vert"), config().resolve("assets/shaders/mote.frag"),
                        2000))
        return -1;
    add(m_motes.node());
    if (!m_swirls.create(config().resolve("assets/shaders/swirl.vert"), config().resolve("assets/shaders/mote.frag"),
                         400))
        return -1;
    add(m_swirls.node());
    if (!m_trails.create(config().resolve("assets/shaders/trail.vert"), config().resolve("assets/shaders/trail.frag")))
        return -1;
    add(m_trails.node());

    // Fog in the sky color: low mist over the water, fading the distance into the sky
    m_fog = FogEffect::create();
    m_fog->setHeightFalloff(0.25f);
    m_fog->setBaseHeight(0.0f);
    m_fog->setSkyAmount(0.0f); // the sky dome already fades into the fog color
    m_fog->setGroundBelowHorizon(true); // below the horizon the background is the distant lake
    m_fog->setHorizonBlend(0.08f);      // and the sky sinks into its mist
    m_fog->load();
    addEffect(m_fog);

    m_glow = demo::GlowEffect::create();
    m_glow->setThreshold(0.4f);
    m_glow->setSpread(0.8f);
    m_glow->load();
    addEffect(m_glow);
    setRenderToTexture(true);

    cameraManipulator()->setFarZ(400.0);
    return 0;
}

MusicState HeavenWindow::musicAt(double time) const
{
    return MusicState{m_bass.value(time), m_pads.value(time), m_lead.value(time), m_arp.value(time)};
}

std::optional<demo::CameraPose> HeavenWindow::cameraPathAt(double time) const
{
    auto pose = DemoWindow::cameraPathAt(time);
    if (!pose)
        return pose;

    // A slow, dreamlike float on top of the authored path
    const float t = float(time);
    pose->position += glm::vec3(0.18f * std::sin(t * 0.31f), 0.07f * std::sin(t * 0.23f + 1.0f),
                                0.12f * std::sin(t * 0.17f + 2.0f));
    pose->target += glm::vec3(0.25f * std::sin(t * 0.13f + 0.5f), 0.1f * std::sin(t * 0.19f), 0.0f);
    return pose;
}

glm::vec2 HeavenWindow::spotAhead(double noteTime, float nearest, float farthest, float width, uint32_t salt) const
{
    uint32_t h = (uint32_t(std::llround(noteTime * 1000.0)) ^ salt) * 2654435761u;
    auto next = [&h]() {
        h ^= h >> 15;
        h *= 2246822519u;
        h ^= h >> 13;
        return float(h & 0xffffu) / 65535.0f;
    };
    const auto pose = cameraPathAt(noteTime);
    if (!pose)
        return glm::vec2(0.0f);
    glm::vec2 forward(pose->target.x - pose->position.x, pose->target.z - pose->position.z);
    forward = glm::length(forward) > 1e-3f ? glm::normalize(forward) : glm::vec2(0.0f, -1.0f);
    const glm::vec2 right(-forward.y, forward.x);
    const glm::vec2 eye(pose->position.x, pose->position.z);
    return eye + forward * (nearest + (farthest - nearest) * next()) + right * ((next() - 0.5f) * width);
}

void HeavenWindow::updateRings(double time)
{
    // Each kick drops a ring onto the water somewhere in front of the camera;
    // in the outro the bells ring out from the last flower
    constexpr double ringLife = 3.5;
    struct Ring {
        glm::vec2 center;
        double time;
        float strength;
    };
    std::vector<Ring> rings;
    for (const auto &note : m_kick.notesBetween(time - ringLife, time + 1e-6))
        rings.push_back({spotAhead(note.time, 5.0f, 15.0f, 9.0f, 0u), note.time, 0.6f + 0.4f * note.velocity});
    const double outro = clock().timeAtBar(140.0);
    for (const auto &note : m_bellNotes.notesBetween(std::max(time - ringLife, outro), time + 1e-6))
        rings.push_back({glm::vec2(kLastFlower.x, kLastFlower.z), note.time, 0.5f + 0.5f * note.velocity});
    std::sort(rings.begin(), rings.end(), [](const Ring &a, const Ring &b) { return a.time < b.time; });
    const int first = std::max(0, int(rings.size()) - LakeMaterial::MaxRings);
    int slot = 0;
    for (size_t i = first; i < rings.size(); ++i, ++slot)
        m_lake->setRing(slot, rings[i].center, float(rings[i].time), rings[i].strength);
    for (; slot < LakeMaterial::MaxRings; ++slot)
        m_lake->setRing(slot, glm::vec2(0.0f), 0.0f, 0.0f);

    // Kicks on a downbeat also send a wave of light through the roots, from the garden's center
    constexpr double waveLife = 5.0;
    const double barLength = clock().timeAtBar(1.0) - clock().timeAtBar(0.0);
    glm::vec3 waves(-1000.0f);
    int wave = 0;
    const auto recent = m_kick.notesBetween(time - waveLife, time + 1e-6);
    for (auto it = recent.rbegin(); it != recent.rend() && wave < 3; ++it)
    {
        const double inBar = std::fmod(it->time - clock().timeAtBar(0.0), barLength);
        if (inBar < 0.01 || inBar > barLength - 0.01)
            waves[wave++] = float(it->time);
    }
    ShaderMaterial::setGlobalUniform("uWaves", waves);
}

void HeavenWindow::onDemoUpdate(double time, double dt)
{
    m_music = musicAt(time);

    const float reveal = param("reveal", 1.0f);
    const float glow = param("glow", 1.0f);
    const glm::vec3 sky = param3("skyColor", {0.02f, 0.02f, 0.05f});

    // Shared by all garden shaders
    ShaderMaterial::setGlobalUniform("uReveal", reveal);
    ShaderMaterial::setGlobalUniform("uGlow", glow);
    ShaderMaterial::setGlobalUniform("uBass", m_music.bass);
    ShaderMaterial::setGlobalUniform("uPads", m_music.pads);
    ShaderMaterial::setGlobalUniform("uLead", m_music.lead);
    ShaderMaterial::setGlobalUniform("uArp", m_music.arp);
    // Brighter mist at the horizon, breathing with the pads; the fog has the same color
    const glm::vec3 horizon = sky * (2.4f + 0.8f * m_music.pads);
    ShaderMaterial::setGlobalUniform("uSky", sky);
    ShaderMaterial::setGlobalUniform("uHorizon", horizon);

    setClearColor(glm::vec4(horizon, 1.0f));
    m_fog->setColor(horizon);
    m_fog->setDensity(param("fogDensity", 0.08f));
    ShaderMaterial::setGlobalUniform("uGrowth", param("growth", 0.0f));
    ShaderMaterial::setGlobalUniform("uRoots", param("roots", 0.0f));
    ShaderMaterial::setGlobalUniform("uSwirl", param("swirl", 0.0f));
    ShaderMaterial::setGlobalUniform("uTrails", param("trails", 1.0f));
    // Act III: weightlessness. uFloat is how many seconds the garden has been weightless,
    // the integral of (1 - gravity), so drifting stays a pure function of song time.
    const double bar = barPosition();
    const double secondsPerBar = clock().timeAtBar(1.0) - clock().timeAtBar(0.0);
    const double weightless = (bar - 1.0) - timeline().integral("gravity", 1.0, bar, 1.0f);
    ShaderMaterial::setGlobalUniform("uGravity", param("gravity", 1.0f));
    ShaderMaterial::setGlobalUniform("uFloat", float(std::max(weightless, 0.0) * secondsPerBar));
    ShaderMaterial::setGlobalUniform("uMirror", param("mirror", 0.0f));
    ShaderMaterial::setGlobalUniform("uRipples", param("ripples", 1.0f));
    ShaderMaterial::setGlobalUniform("uAurora", param("aurora", 0.0f));
    // Act IV: the world dissolves into sparks, closing in on the last flower
    ShaderMaterial::setGlobalUniform("uDissolve", param("dissolve", 0.0f));
    ShaderMaterial::setGlobalUniform("uLastFlower", kLastFlower);
    ShaderMaterial::setGlobalUniform("uLastLight", param("lastLight", 0.0f));

    const float islands = param("islands", 0.0f);
    ShaderMaterial::setGlobalUniform("uIslands", islands);
    ShaderMaterial::setGlobalUniform("uStab", m_stab.value(time));
    ShaderMaterial::setGlobalUniform("uClap", m_clap.value(time));
    m_islands.update(time, islands);

    updateRings(time);
    // Lead notes rise from the water ahead of the camera as trails of light
    m_trails.update(time, m_leadNotes, [this](const demo::NoteEvent &note) {
        const glm::vec2 spot = spotAhead(note.time, 6.0f, 14.0f, 10.0f, 0x5bd1e995u);
        return glm::vec3(spot.x, 0.0f, spot.y);
    });
    m_glow->setIntensity(0.8f + 0.8f * glow + 0.5f * m_music.pads);
}

void HeavenWindow::onDemoUi()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos({viewport->WorkPos.x + 10.0f, viewport->WorkPos.y + 10.0f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({260.0f, 0.0f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Music");
    ImGui::ProgressBar(m_music.bass, {-1.0f, 0.0f}, "bass");
    ImGui::ProgressBar(m_music.pads, {-1.0f, 0.0f}, "pads");
    ImGui::ProgressBar(m_music.lead, {-1.0f, 0.0f}, "lead");
    ImGui::ProgressBar(m_music.arp, {-1.0f, 0.0f}, "arp");
    ImGui::TextUnformatted(freeCamera() ? "F4: free camera" : "Guided camera (F4: free)");
    ImGui::End();
}
