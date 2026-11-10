#pragma once

/**
 * @file motes.h
 * @brief Thousands of glowing motes drifting through the garden.
 */

#include <blend_material.h>

#include <ivf/instanced_mesh_node.h>

#include <string>

namespace heaven {

/**
 * @class Motes
 * @brief Small glowing points floating in the air, drawn additively.
 *
 * Each mote is an instance of a camera-facing quad. Its position is a pure
 * function of song time (a seeded home position plus layered slow swirls), so
 * the motes never stop drifting and scrubbing stays exact. Arp notes make a
 * seeded subset sparkle; pads make all of them swell gently.
 */
class Motes {
public:
    /**
     * @brief Build the motes.
     * @param count Number of motes.
     * @return false if the shaders failed to compile.
     */
    bool create(const std::string &vertexShader, const std::string &fragmentShader, int count);

    [[nodiscard]] ivf::InstancedMeshNodePtr node() const { return m_node; }

private:
    ivf::InstancedMeshNodePtr m_node;
    demo::BlendMaterialPtr m_material;
};

} // namespace heaven
