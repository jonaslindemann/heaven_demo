#include "islands.h"

#include "light_trails.h"

#include <ivf/nodes.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

using namespace heaven;
using namespace ivf;

namespace {

constexpr float kTwoPi = 6.28318531f;
constexpr int kVinesPerIsland = 7;

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

const glm::vec3 kIslandTints[] = {{0.3f, 1.0f, 0.85f}, {0.75f, 0.5f, 1.0f}, {1.0f, 0.5f, 0.8f}, {0.4f, 0.8f, 1.0f}};

float smoothstep(float a, float b, float x)
{
    const float t = std::clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

} // namespace

bool Islands::create(const std::string &rockVertex, const std::string &rockFragment, const std::string &vineVertex,
                     const std::string &vineFragment, const std::function<bool(glm::vec2, float)> &keepClear)
{
    m_rockMaterial = ShaderMaterial::createFromFiles(rockVertex, rockFragment, "heaven_island", true);
    m_vineMaterial = demo::BlendMaterial::createFromFiles(vineVertex, vineFragment, "heaven_vine",
                                                          demo::BlendMaterial::Mode::Additive, true);
    if (!m_rockMaterial || !m_vineMaterial)
        return false;

    // Around the garden, out of the camera's way, at varied heights and sizes
    const glm::vec2 center(0.0f, -15.0f);
    for (uint32_t attempt = 0; attempt < 400 && int(m_islands.size()) < Count; ++attempt)
    {
        const uint32_t s = attempt * 8u + 3u;
        Island island;
        const float angle = kTwoPi * hash01(s);
        const float distance = 14.0f + 32.0f * hash01(s + 1u);
        island.position = center + distance * glm::vec2(std::cos(angle), std::sin(angle));
        island.radius = 2.2f + 3.8f * hash01(s + 2u);
        island.height = 7.0f + 14.0f * hash01(s + 3u) + 0.3f * distance;
        island.order = hash01(s + 4u);
        island.phase = kTwoPi * hash01(s + 5u);
        if (keepClear && keepClear(island.position, island.radius))
            continue;
        bool overlaps = false;
        for (const auto &other : m_islands)
            overlaps = overlaps || glm::distance(other.position, island.position) < other.radius + island.radius + 3.0f;
        if (!overlaps)
            m_islands.push_back(island);
    }

    // Rocks: a sphere that island.vert shapes into a flat-topped, tapering island
    m_rocks = InstancedMeshNode::create(Sphere::create(1.0, 48, 32));
    m_vines = InstancedMeshNode::create(buildRibbonMesh(48));
    for (int i = 0; i < int(m_islands.size()); ++i)
    {
        const auto &island = m_islands[i];
        const glm::vec3 tint = kIslandTints[i % 4];
        m_rocks->addInstance(glm::mat4(1.0f), glm::vec4(tint, 1.0f),
                             glm::vec4(float(i), hash01(uint32_t(i) + 900u) * 100.0f, island.radius, 0.0f));

        for (int v = 0; v < kVinesPerIsland; ++v)
        {
            const uint32_t s = uint32_t(i * 31 + v) + 5000u;
            // Data: (island, seed, length, where under the island: ring radius in [0, 1])
            m_vines->addInstance(glm::mat4(1.0f), glm::vec4(tint, island.radius),
                                 glm::vec4(float(i), hash01(s) * 100.0f, island.radius * (1.2f + 2.2f * hash01(s + 1u)),
                                           0.15f + 0.7f * hash01(s + 2u)));
        }

        // A few flowers on top, inside the flat part
        const int flowers = 3 + int(island.radius);
        for (int f = 0; f < flowers; ++f)
        {
            const uint32_t s = uint32_t(i * 57 + f) + 9000u;
            const float rho = island.radius * 0.6f * std::sqrt(hash01(s));
            const float a = kTwoPi * hash01(s + 1u);
            IslandFlowerSpot spot;
            spot.island = i;
            spot.local = glm::vec3(rho * std::cos(a), island.radius * 0.18f, rho * std::sin(a));
            spot.scale = 0.5f + 0.5f * hash01(s + 2u);
            spot.height = 0.4f + 1.0f * hash01(s + 3u);
            m_flowerSpots.push_back(spot);
        }
    }
    m_rocks->setMaterial(m_rockMaterial);
    m_vines->setMaterial(m_vineMaterial);
    update(0.0, 0.0f);
    return true;
}

void Islands::update(double time, float amount)
{
    const float t = float(time);
    for (int i = 0; i < Count; ++i)
    {
        glm::vec4 value(0.0f, -1000.0f, 0.0f, 0.0f); // unused slots stay far below the lake
        if (i < int(m_islands.size()))
        {
            const auto &island = m_islands[i];
            // Rise out of the lake one after another, each in a short window of the value,
            // so none is left half-submerged while the value holds; sink back in reverse
            const float start = island.order * 0.85f;
            const float presence = smoothstep(start, start + 0.12f, amount);
            const float submerged = -(island.radius * 2.6f + 1.0f);
            const float y = submerged + (island.height - submerged) * presence +
                            0.35f * std::sin(t * 0.27f + island.phase);
            const glm::vec2 drift = 0.6f * glm::vec2(std::sin(t * 0.05f + island.phase), std::cos(t * 0.04f + island.phase));
            value = glm::vec4(island.position.x + drift.x, y, island.position.y + drift.y, presence);
        }
        ShaderMaterial::setGlobalUniform("uIsland[" + std::to_string(i) + "]", value);
    }
}
