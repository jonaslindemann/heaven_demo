#include "light_trails.h"

#include <ivf/mesh_node.h>

#include <algorithm>
#include <cmath>

using namespace heaven;
using namespace ivf;

namespace {

// Warm gold, rose, pale cyan and white-violet: brighter than the flowers, so the melody stands out
const glm::vec3 kTrailColors[] = {
    {1.0f, 0.8f, 0.45f}, {1.0f, 0.5f, 0.7f}, {0.5f, 0.95f, 1.0f}, {0.85f, 0.75f, 1.0f}};

} // namespace

MeshNodePtr heaven::buildRibbonMesh(int segments)
{
    auto ribbon = MeshNode::create();
    ribbon->newMesh((segments + 1) * 2, segments * 2);
    auto mesh = ribbon->lastMesh();
    mesh->setGenerateNormals(false);
    mesh->begin(GL_TRIANGLES);
    for (int i = 0; i <= segments; ++i)
        for (float side : {-1.0f, 1.0f})
        {
            mesh->vertex3f(float(i) / segments, side, 0.0f);
            mesh->color4f(1.0f, 1.0f, 1.0f, 1.0f);
            mesh->normal3f(0.0f, 1.0f, 0.0f);
        }
    for (int i = 0; i < segments; ++i)
    {
        const int a = i * 2;
        mesh->index3i(a, a + 2, a + 1);
        mesh->index3i(a + 1, a + 2, a + 3);
    }
    mesh->end();
    return ribbon;
}

bool LightTrails::create(const std::string &vertexShader, const std::string &fragmentShader)
{
    m_material = demo::BlendMaterial::createFromFiles(vertexShader, fragmentShader, "heaven_trail",
                                                      demo::BlendMaterial::Mode::Additive, true);
    if (!m_material)
        return false;

    auto ribbon = buildRibbonMesh(64);

    m_node = InstancedMeshNode::create(ribbon);
    m_node->setInstanceCount(MaxTrails);
    for (int i = 0; i < MaxTrails; ++i)
        m_node->setInstanceData(i, glm::vec4(-1000.0f, 0.0f, 0.0f, 0.0f)); // inactive
    m_node->setMaterial(m_material);
    return true;
}

void LightTrails::update(double time, const demo::SongEnvelope &notes,
                         const std::function<glm::vec3(const demo::NoteEvent &)> &origin)
{
    // Notes whose trail can still be visible (durations are at most a few seconds)
    const auto recent = notes.notesBetween(time - Lifetime - 5.0, time + 1e-6);
    int slot = 0;
    for (auto it = recent.rbegin(); it != recent.rend() && slot < MaxTrails; ++it)
    {
        const auto &note = *it;
        if (time - note.end() > Lifetime)
            continue;
        const float seed = float(std::fmod(note.time * 7.31, 1000.0));
        const float pitch = std::clamp((note.pitch - 60) / 24.0f, 0.0f, 1.0f);
        const glm::vec3 color = kTrailColors[int(std::llround(note.time * 1000.0)) % 4] * note.velocity;
        m_node->setInstanceTransform(slot, InstancedMeshNode::makeTransform(origin(note)));
        m_node->setInstanceColor(slot, glm::vec4(color, 1.0f));
        m_node->setInstanceData(slot, glm::vec4(float(note.time), float(note.duration), seed, pitch));
        ++slot;
    }
    for (; slot < MaxTrails; ++slot)
        m_node->setInstanceData(slot, glm::vec4(-1000.0f, 0.0f, 0.0f, 0.0f));
}
