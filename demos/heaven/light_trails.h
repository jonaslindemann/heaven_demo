#pragma once

/**
 * @file light_trails.h
 * @brief Ribbons of light drawn through the air by the lead melody.
 */

#include <blend_material.h>
#include <song_data.h>

#include <ivf/instanced_mesh_node.h>
#include <ivf/mesh_node.h>

#include <glm/glm.hpp>

#include <functional>
#include <string>

namespace heaven {

/**
 * @brief A strip of (segments + 1) x 2 vertices: aPos = (u along in [0, 1], side in [-1, 1], 0).
 *
 * Shaders bend it onto a curve and widen it towards the camera (trail.vert, vine.vert).
 */
ivf::MeshNodePtr buildRibbonMesh(int segments);

/**
 * @class LightTrails
 * @brief One trail per note: it rises from the water and sweeps a curving arc.
 *
 * A fixed pool of ribbons is reassigned every frame to the most recent notes,
 * so the trails are a pure function of song time. The curve, its timing and
 * fading are computed in trail.vert from per-instance data:
 * (note start, note duration, seed, pitch in [0, 1]).
 */
class LightTrails {
public:
    static constexpr int MaxTrails = 12;
    static constexpr double Lifetime = 2.2 + 1.8; ///< Sweep time plus fade after the note, see trail.vert.

    bool create(const std::string &vertexShader, const std::string &fragmentShader);

    /**
     * @brief Assign the ribbons to the notes sounding around @p time.
     * @param notes Source of notes (e.g. the lead envelope).
     * @param origin Where the trail of a note starts on the water, from the note.
     */
    void update(double time, const demo::SongEnvelope &notes,
                const std::function<glm::vec3(const demo::NoteEvent &)> &origin);

    [[nodiscard]] ivf::InstancedMeshNodePtr node() const { return m_node; }

private:
    ivf::InstancedMeshNodePtr m_node;
    demo::BlendMaterialPtr m_material;
};

} // namespace heaven
