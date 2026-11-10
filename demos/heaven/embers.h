#pragma once

/**
 * @file embers.h
 * @brief Sparks rising from the flowers as they dissolve (Act IV).
 */

#include "flower_field.h"

#include <blend_material.h>

#include <ivf/instanced_mesh_node.h>

#include <string>
#include <vector>

namespace heaven {

/**
 * @class Embers
 * @brief Camera-facing sparks that leave each flower as it burns away.
 *
 * Every flower gets a few instances carrying its own transform and data, so
 * ember.vert can find where the flower is and when it dissolves (the same
 * front as in flower.vert). Drawn additively with mote.frag.
 */
class Embers {
public:
    /**
     * @brief Build the embers.
     * @param flowers The flowers that dissolve; the last flower (order -1) is skipped.
     * @param perFlower Embers per flower.
     * @return false if the shaders failed to compile.
     */
    bool create(const std::string &vertexShader, const std::string &fragmentShader,
                const std::vector<FlowerPlacement> &flowers, int perFlower);

    [[nodiscard]] ivf::InstancedMeshNodePtr node() const { return m_node; }

private:
    ivf::InstancedMeshNodePtr m_node;
    demo::BlendMaterialPtr m_material;
};

} // namespace heaven
