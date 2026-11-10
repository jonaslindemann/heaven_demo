#pragma once

/**
 * @file lake_material.h
 * @brief The dark mirror lake: planar reflection, calm swell and expanding pulse rings.
 */

#include <ivf/planar_reflection.h>
#include <ivf/shader_material.h>

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace heaven {

/**
 * @class LakeMaterial
 * @brief Reflecting lake whose surface is shaped in assets/shaders/lake.frag.
 *
 * The surface is a near-perfect mirror with a slow, faint swell. Bass hits do
 * not shake the whole lake; each one sends a ring outwards from a point on the
 * water (see setRing()), like a drop falling into still water.
 */
class LakeMaterial : public ivf::ShaderMaterial {
public:
    static constexpr int MaxRings = 8;

    LakeMaterial(ivf::ProgramPtr program, ivf::PlanarReflectionPtr reflection);

    /**
     * @brief Load the lake shaders.
     * @param watch Reload the files when they change.
     * @return The material, or nullptr if the shaders failed to compile.
     */
    static std::shared_ptr<LakeMaterial> createFromFiles(const std::string &vertPath, const std::string &fragPath,
                                                         ivf::PlanarReflectionPtr reflection, bool watch = false);

    /**
     * @brief Set one pulse ring.
     * @param index Slot in [0, MaxRings).
     * @param center Ring origin on the water (x, z).
     * @param startTime Song time the ring was emitted.
     * @param strength Amplitude, 0 disables the slot.
     */
    void setRing(int index, glm::vec2 center, float startTime, float strength);

    void apply() override;

private:
    ivf::PlanarReflectionPtr m_reflection;
};

using LakeMaterialPtr = std::shared_ptr<LakeMaterial>;

} // namespace heaven
