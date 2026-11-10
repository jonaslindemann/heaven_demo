#pragma once

/**
 * @file islands.h
 * @brief Floating islands that rise out of the lake in the choruses.
 */

#include "flower_field.h"

#include <blend_material.h>

#include <ivf/instanced_mesh_node.h>
#include <ivf/shader_material.h>

#include <glm/glm.hpp>

#include <functional>
#include <string>
#include <vector>

namespace heaven {

/**
 * @class Islands
 * @brief Rocky islands with glowing veins, flowers on top and luminous vines hanging below.
 *
 * update() places every island from the song time and the `islands` timeline
 * value and publishes it as the global uniform `uIsland[i]` = (position, presence),
 * which the rock (island.vert), the vines (vine.vert) and the island flowers
 * (flower.vert) all read, so they move together. Islands rise out of the lake
 * one after another as the value grows and sink back as it falls.
 */
class Islands {
public:
    static constexpr int Count = 12; ///< Matches the uIsland[] array size in the shaders.

    /**
     * @param keepClear Returns true for lake positions (x, z) where an island of the
     *                  given radius would be in the camera's way.
     */
    bool create(const std::string &rockVertex, const std::string &rockFragment, const std::string &vineVertex,
                const std::string &vineFragment, const std::function<bool(glm::vec2, float)> &keepClear = {});

    /** @brief Place the islands; @p amount is the `islands` timeline value in [0, 1]. */
    void update(double time, float amount);

    /** @brief Spots for the flowers on the island tops, see FlowerField::addIslandFlowers(). */
    [[nodiscard]] const std::vector<IslandFlowerSpot> &flowerSpots() const { return m_flowerSpots; }

    [[nodiscard]] ivf::InstancedMeshNodePtr rockNode() const { return m_rocks; }
    [[nodiscard]] ivf::InstancedMeshNodePtr vineNode() const { return m_vines; }

private:
    struct Island {
        glm::vec2 position; ///< Lake position (x, z).
        float height;       ///< Floating height when fully risen.
        float radius;
        float order;        ///< When it rises, in [0, 1].
        float phase;
    };

    std::vector<Island> m_islands;
    std::vector<IslandFlowerSpot> m_flowerSpots;
    ivf::InstancedMeshNodePtr m_rocks;
    ivf::InstancedMeshNodePtr m_vines;
    ivf::ShaderMaterialPtr m_rockMaterial;
    demo::BlendMaterialPtr m_vineMaterial;
};

} // namespace heaven
