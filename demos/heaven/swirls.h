#pragma once

/**
 * @file swirls.h
 * @brief Streams of sparkles flowing along curved paths around the garden.
 */

#include <blend_material.h>

#include <ivf/instanced_mesh_node.h>

#include <string>

namespace heaven {

/**
 * @class Swirls
 * @brief Sparkles flowing in looping streams through the garden; they flare up with the arps.
 *
 * The stream curves and the motion along them are computed in swirl.vert, so
 * positions are a pure function of song time. The `uSwirl` uniform (timeline)
 * brings the streams in one after another.
 *
 * Per-instance data: (stream index, position along the stream in [0, 1), seed, size).
 */
class Swirls {
public:
    static constexpr int Streams = 8;

    bool create(const std::string &vertexShader, const std::string &fragmentShader, int perStream);

    [[nodiscard]] ivf::InstancedMeshNodePtr node() const { return m_node; }

private:
    ivf::InstancedMeshNodePtr m_node;
    demo::BlendMaterialPtr m_material;
};

} // namespace heaven
