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

#include <blend_material.h>
#include <demo_window.h>
#include <glow_effect.h>
#include <song_data.h>

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
class HeavenWindow : public demo::DemoWindow {
public:
    explicit HeavenWindow(const demo::DemoConfig &config);

protected:
    int onDemoSetup() override;
    void onDemoUpdate(double time, double dt) override;
    void onDemoUi() override;
    std::optional<demo::CameraPose> cameraPathAt(double time) const override;

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

    demo::SongEnvelope m_bass;
    demo::SongEnvelope m_pads;
    demo::SongEnvelope m_lead;
    demo::SongEnvelope m_arp;
    demo::SongEnvelope m_kick;      ///< Note source for the rings on the water and the waves of light.
    demo::SongEnvelope m_leadNotes; ///< Note source for the light trails.
    demo::SongEnvelope m_bellNotes; ///< Note source for the rings from the last flower (outro).
    demo::SongEnvelope m_stab;
    demo::SongEnvelope m_clap;
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
    demo::BlendMaterialPtr m_sky;
    ivf::FogEffectPtr m_fog;
    demo::GlowEffectPtr m_glow;
};

} // namespace heaven
