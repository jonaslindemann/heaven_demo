#pragma once

/**
 * @file flower_field.h
 * @brief Procedural glowing flowers, shaped and animated on the GPU.
 */

#include <ivf/instanced_mesh_node.h>
#include <ivf/shader_material.h>

#include <glm/glm.hpp>

#include <functional>
#include <string>
#include <vector>

namespace heaven {

/**
 * @struct IslandFlowerSpot
 * @brief Where a flower grows on a floating island, relative to the island's center.
 */
struct IslandFlowerSpot {
    int island{0};
    glm::vec3 local{0.0f};
    float scale{1.0f};
    float height{1.0f};
};

/**
 * @struct FlowerPlacement
 * @brief One flower of the garden as it is instanced: transform, (tint, nod) and data.
 */
struct FlowerPlacement {
    glm::mat4 transform{1.0f};
    glm::vec4 color{1.0f};
    glm::vec4 data{0.0f};
};

/** @brief Where the last flower grows: the first to emerge and the only one left at the end. */
inline constexpr glm::vec3 kLastFlower{2.0f, 0.0f, -1.5f};

/**
 * @class FlowerField
 * @brief Instanced procedural flowers rising from the lake.
 *
 * The flower meshes store parameters instead of positions: (u, v) on a petal,
 * the petal index and ring, or stem and core coordinates. The vertex shader
 * (flower.vert) builds the shape from the petal formula
 * P(u,v) = C(u) + w(u) v N(u) + h(u,v) B(u) and animates opening, breathing,
 * swaying and emergence from uniforms and per-instance data, so all flowers of
 * a species are drawn in one call and every frame is a pure function of time.
 *
 * Per-instance data: x = random seed, y = animation phase, z = emergence order,
 * w = stem height. Orders in [0, 1] emerge with the `uReveal` uniform (the
 * Act I garden); orders in [2, 3] with `uGrowth` (the garden spreading outwards
 * in Act II), nearest the center first. Orders from 4 belong to floating island
 * (order - 4): they move with it and show with it, see `uIsland[]` in flower.vert.
 * Order -1 is the last flower (kLastFlower): it never floats away or dissolves.
 */
class FlowerField {
public:
    /**
     * @brief Build meshes, material and placements.
     * @param vertexShader Path of flower.vert.
     * @param fragmentShader Path of flower.frag.
     * @param keepClear Returns true for lake positions (x, z) where no flower may
     *                  grow, e.g. along the camera path.
     * @return false if the shaders failed to compile.
     */
    bool create(const std::string &vertexShader, const std::string &fragmentShader,
                const std::function<bool(glm::vec2)> &keepClear = {});

    /** @brief Add flowers growing on floating islands; call after create(). */
    void addIslandFlowers(const std::vector<IslandFlowerSpot> &spots);

    /** @brief Scene nodes to add (one instanced node per species). */
    [[nodiscard]] const std::vector<ivf::InstancedMeshNodePtr> &nodes() const { return m_nodes; }

    [[nodiscard]] ivf::ShaderMaterialPtr material() const { return m_material; }

    /** @brief The flowers of the lake (not those on islands), as instanced. */
    [[nodiscard]] const std::vector<FlowerPlacement> &placements() const { return m_placements; }

private:
    std::vector<FlowerPlacement> m_placements;
    std::vector<ivf::InstancedMeshNodePtr> m_nodes;
    ivf::ShaderMaterialPtr m_material;
};

} // namespace heaven
