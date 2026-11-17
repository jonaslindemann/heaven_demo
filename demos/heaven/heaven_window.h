#pragma once

/**
 * @file heaven_window.h
 * @brief Paranoid Frog - Heaven: "The Dreaming Garden" demo.
 */

#include "clouds.h"
#include "embers.h"
#include "flower_field.h"
#include "giants.h"
#include "islands.h"
#include "lake_material.h"
#include "light_trails.h"
#include "motes.h"
#include "swirls.h"

#include <ivf/blend_shader_material.h>
#include <ivfmusic/glfw_music_scene_window.h>
#include <ivf/mip_bloom_effect.h>
#include <ivfmusic/score.h>

#include <ivf/fog_effect.h>
#include <ivf/nodes.h>

#include <cstdint>

namespace heaven {

/**
 * @struct MusicState
 * @brief The musical controls the garden responds to, each in [0, 1].
 */
struct MusicState {
    float bass{0.0f}; ///< Kick + Bass: ground pulses.
    float pads{0.0f}; ///< Pad + Choir: atmosphere, bloom, flower breathing.
    float lead{0.0f}; ///< Lead + Bell: flowers opening, light trails.
    float arp{0.0f};  ///< Arp A + Arp B: sparkling particles.
};

/**
 * @class HeavenWindow
 * @brief The Dreaming Garden: a glowing garden on a dark lake, driven by the song.
 */
class HeavenWindow : public ivfmusic::GLFWMusicSceneWindow {
public:
    explicit HeavenWindow(const ivfmusic::MusicSceneConfig &config);

protected:
    int onSceneSetup() override;
    void onSceneUpdate(double time, double dt) override;
    void onSceneUi() override;
    std::optional<ivf::TimelineCamera> cameraAt(double time) const override;

    /** @brief Musical controls at a song time; a pure function of time. */
    MusicState musicAt(double time) const;

private:
    void updateRings(double time);

    /**
     * @brief A spot on the water ahead of the camera at a note's time, hashed from the
     *        note time so it is the same on every run.
     * @param nearest,farthest Distance range ahead of the camera.
     * @param width Sideways spread.
     * @param salt Varies the placement between effects.
     */
    glm::vec2 spotAhead(double noteTime, float nearest, float farthest, float width, uint32_t salt) const;

    ivfmusic::TrackEnvelope m_bass;
    ivfmusic::TrackEnvelope m_pads;
    ivfmusic::TrackEnvelope m_lead;
    ivfmusic::TrackEnvelope m_arp;
    ivfmusic::TrackEnvelope m_kick;      ///< Note source for the rings on the water and the waves of light.
    ivfmusic::TrackEnvelope m_leadNotes; ///< Note source for the light trails.
    ivfmusic::TrackEnvelope m_bellNotes; ///< Note source for the rings from the last flower (outro).
    ivfmusic::TrackEnvelope m_stab;
    ivfmusic::TrackEnvelope m_clap;
    MusicState m_music;

    FlowerField m_flowers;
    Motes m_motes;
    Swirls m_swirls;
    LightTrails m_trails;
    Islands m_islands;
    Giants m_giants;
    Clouds m_clouds;
    Embers m_embers;
    LakeMaterialPtr m_lake;
    ivf::BlendShaderMaterialPtr m_sky;
    ivf::FogEffectPtr m_fog;
    ivf::MipBloomEffectPtr m_glow;
};

} // namespace heaven
