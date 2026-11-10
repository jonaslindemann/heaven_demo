#include "swirls.h"

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

const glm::vec3 kStreamColors[] = {{0.5f, 0.95f, 1.0f}, {1.0f, 0.6f, 0.9f}, {0.75f, 0.6f, 1.0f}, {0.6f, 1.0f, 0.8f},
                                   {1.0f, 0.85f, 0.6f}, {0.55f, 0.8f, 1.0f}, {1.0f, 0.55f, 0.65f}, {0.8f, 1.0f, 0.95f}};

} // namespace

bool Swirls::create(const std::string &vertexShader, const std::string &fragmentShader, int perStream)
{
    m_material = demo::BlendMaterial::createFromFiles(vertexShader, fragmentShader, "heaven_swirl",
                                                      demo::BlendMaterial::Mode::Additive, true);
    if (!m_material)
        return false;

    // One quad; its corners are expanded towards the camera in swirl.vert
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
    const int count = Streams * perStream;
    m_node->setInstanceCount(size_t(count));
    for (int i = 0; i < count; ++i)
    {
        const int stream = i % Streams;
        const uint32_t s = uint32_t(i) * 3u + 7u;
        m_node->setInstanceColor(i, glm::vec4(kStreamColors[stream], 1.0f));
        m_node->setInstanceData(i, glm::vec4(float(stream), hash01(s), hash01(s + 1u) * 1000.0f,
                                             0.025f + 0.04f * hash01(s + 2u)));
    }
    m_node->setMaterial(m_material);
    return true;
}
