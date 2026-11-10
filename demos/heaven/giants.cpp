#include "giants.h"

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

const glm::vec3 kGiantTints[] = {{0.3f, 1.0f, 0.8f}, {0.7f, 0.45f, 1.0f}, {1.0f, 0.45f, 0.75f}, {0.4f, 0.75f, 1.0f}};

} // namespace

bool Giants::create(const std::string &vertexShader, const std::string &fragmentShader, int count)
{
    m_material = ShaderMaterial::createFromFiles(vertexShader, fragmentShader, "heaven_giant", true);
    if (!m_material)
        return false;

    // Tube parameters: aPos = (s along the stalk, angle around it in [0, 1], 0)
    constexpr int along = 160, around = 12;
    auto tube = MeshNode::create();
    tube->newMesh((along + 1) * (around + 1), along * around * 2);
    auto mesh = tube->lastMesh();
    mesh->setGenerateNormals(false);
    mesh->begin(GL_TRIANGLES);
    for (int i = 0; i <= along; ++i)
        for (int j = 0; j <= around; ++j)
        {
            mesh->vertex3f(float(i) / along, float(j) / around, 0.0f);
            mesh->color4f(1.0f, 1.0f, 1.0f, 1.0f);
            mesh->normal3f(0.0f, 1.0f, 0.0f);
        }
    for (int i = 0; i < along; ++i)
        for (int j = 0; j < around; ++j)
        {
            const int a = i * (around + 1) + j, b = a + around + 1;
            mesh->index3i(a, b, a + 1);
            mesh->index3i(a + 1, b, b + 1);
        }
    mesh->end();

    // On a wide ring around the garden, curling towards it
    m_node = InstancedMeshNode::create(tube);
    const glm::vec2 center(0.0f, -15.0f);
    for (int i = 0; i < count; ++i)
    {
        const uint32_t s = uint32_t(i) * 6u + 11u;
        const float angle = kTwoPi * (float(i) + 0.6f * hash01(s)) / float(count);
        const float distance = 58.0f + 30.0f * hash01(s + 1u);
        const glm::vec2 p = center + distance * glm::vec2(std::cos(angle), std::sin(angle));
        // Turn the curl plane so the fiddlehead leans in over the garden
        const float yaw = -glm::degrees(angle) + 180.0f + 40.0f * (hash01(s + 2u) - 0.5f);
        m_node->addInstance(InstancedMeshNode::makeTransform({p.x, -1.0f, p.y}, {0.0f, yaw, 0.0f}, glm::vec3(1.0f)),
                            glm::vec4(kGiantTints[i % 4], 1.0f),
                            glm::vec4(hash01(s + 3u) * 100.0f, 28.0f + 30.0f * hash01(s + 4u),
                                      5.0f + 5.0f * hash01(s + 5u), hash01(s + 6u)));
    }
    m_node->setMaterial(m_material);
    return true;
}
