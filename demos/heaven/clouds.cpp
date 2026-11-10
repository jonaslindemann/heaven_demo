#include "clouds.h"

#include <ivf/mesh_node.h>

#include <cmath>
#include <cstdint>

using namespace heaven;
using namespace ivf;

namespace {

constexpr float kTwoPi = 6.28318531f;

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

const glm::vec3 kCloudTints[] = {{1.0f, 0.55f, 0.85f}, {0.65f, 0.5f, 1.0f}, {0.45f, 0.85f, 1.0f}, {1.0f, 0.7f, 0.6f}};

} // namespace

bool Clouds::create(const std::string &vertexShader, const std::string &fragmentShader, int count)
{
    m_material = demo::BlendMaterial::createFromFiles(vertexShader, fragmentShader, "heaven_cloud",
                                                      demo::BlendMaterial::Mode::Additive, true);
    if (!m_material)
        return false;

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

    // A wide, high ring of clouds around the garden
    m_node = InstancedMeshNode::create(quad);
    const glm::vec2 center(0.0f, -15.0f);
    for (int i = 0; i < count; ++i)
    {
        const uint32_t s = uint32_t(i) * 7u + 23u;
        const float angle = kTwoPi * hash01(s);
        const float distance = 35.0f + 95.0f * hash01(s + 1u);
        const glm::vec3 p(center.x + distance * std::cos(angle), 16.0f + 26.0f * hash01(s + 2u),
                          center.y + distance * std::sin(angle));
        m_node->addInstance(InstancedMeshNode::makeTransform(p), glm::vec4(kCloudTints[i % 4], 1.0f),
                            glm::vec4(hash01(s + 3u) * 100.0f, 9.0f + 16.0f * hash01(s + 4u), hash01(s + 5u),
                                      0.3f + 0.7f * hash01(s + 6u)));
    }
    m_node->setMaterial(m_material);
    return true;
}
