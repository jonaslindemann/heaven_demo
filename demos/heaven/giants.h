#pragma once

/**
 * @file giants.h
 * @brief Enormous fern-like stalks that unfurl out of the lake in the distance.
 */

#include <ivf/instanced_mesh_node.h>
#include <ivf/shader_material.h>

#include <string>

namespace heaven {

/**
 * @class Giants
 * @brief Huge glowing stalks that rise and uncurl like fiddlehead ferns.
 *
 * The tube is built in giant.vert from mesh parameters (position along the
 * stalk, angle around it); the visible length follows the `uIslands` uniform,
 * so the stalks grow and unfurl in the choruses.
 *
 * Per-instance data: (seed, height, curl radius, emergence order); the
 * transform places and turns each stalk.
 */
class Giants {
public:
    bool create(const std::string &vertexShader, const std::string &fragmentShader, int count);

    [[nodiscard]] ivf::InstancedMeshNodePtr node() const { return m_node; }

private:
    ivf::InstancedMeshNodePtr m_node;
    ivf::ShaderMaterialPtr m_material;
};

} // namespace heaven
