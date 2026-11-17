#pragma once

/**
 * @file clouds.h
 * @brief Glowing clouds drifting high above the garden in the choruses.
 */

#include <ivf/blend_shader_material.h>

#include <ivf/instanced_mesh_node.h>

#include <string>

namespace heaven {

/**
 * @class Clouds
 * @brief Large soft camera-facing clouds, shaped by noise in cloud.frag.
 *
 * They fade in with the `uIslands` uniform, breathe with the pads and flash on claps.
 * Per-instance data: (seed, size, emergence order, drift speed).
 */
class Clouds {
public:
    bool create(const std::string &vertexShader, const std::string &fragmentShader, int count);

    [[nodiscard]] ivf::InstancedMeshNodePtr node() const { return m_node; }

private:
    ivf::InstancedMeshNodePtr m_node;
    ivf::BlendShaderMaterialPtr m_material;
};

} // namespace heaven
