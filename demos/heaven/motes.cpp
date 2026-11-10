#include "motes.h"

#include <ivf/mesh_node.h>

#include <cstdint>

using namespace heaven;
using namespace ivf;

namespace {

// Hash-based random in [0, 1), identical on every machine
float hash01(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return float(x >> 8) / float(1u << 24);
}

const glm::vec3 kMoteColors[] = {
    {0.6f, 0.9f, 1.0f}, {1.0f, 0.8f, 0.55f}, {0.85f, 0.6f, 1.0f}, {0.6f, 1.0f, 0.85f}};

} // namespace

bool Motes::create(const std::string &vertexShader, const std::string &fragmentShader, int count)
{
    m_material = demo::BlendMaterial::createFromFiles(vertexShader, fragmentShader, "heaven_mote",
                                                      demo::BlendMaterial::Mode::Additive, true);
    if (!m_material)
        return false;

    // One quad; its corners are expanded towards the camera in mote.vert
    auto quad = MeshNode::create();
    quad->newMesh(4, 2);
    auto mesh = quad->lastMesh();
    mesh->setGenerateNormals(false);
    mesh->begin(GL_TRIANGLES);
    for (glm::vec2 corner : {glm::vec2(-1, -1), glm::vec2(1, -1), glm::vec2(1, 1), glm::vec2(-1, 1)})
    {
        mesh->vertex3f(corner.x, corner.y, 0.0f);
        mesh->color4f(1.0f, 1.0f, 1.0f, 1.0f);
        mesh->normal3f(0.0f, 0.0f, 1.0f);
    }
    mesh->index3i(0, 1, 2);
    mesh->index3i(0, 2, 3);
    mesh->end();

    m_node = InstancedMeshNode::create(quad);
    m_node->setInstanceCount(size_t(count));
    for (int i = 0; i < count; ++i)
    {
        const uint32_t s = uint32_t(i) * 4u + 1u;
        // Home position in the volume around the camera's path, with more motes low over the water
        const float height = 0.15f + 6.0f * hash01(s) * hash01(s + 101u);
        const glm::vec3 home(-16.0f + 32.0f * hash01(s + 1u), height, -45.0f + 62.0f * hash01(s + 2u));
        m_node->setInstanceTransform(i, InstancedMeshNode::makeTransform(home));
        m_node->setInstanceColor(i, glm::vec4(kMoteColors[i % 4], 1.0f));
        // Data: seed, size, sparkle group (fraction of motes that follow the arp), phase
        m_node->setInstanceData(i, glm::vec4(hash01(s + 3u) * 1000.0f, 0.02f + 0.05f * hash01(s + 4u),
                                             hash01(s + 5u), hash01(s + 6u) * 6.2831853f));
    }
    m_node->setMaterial(m_material);
    return true;
}
