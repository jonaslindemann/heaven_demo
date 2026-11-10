#include "flower_field.h"

#include <ivf/mesh_node.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <vector>
#include <cstdint>

using namespace heaven;
using namespace ivf;

namespace {

constexpr float kTwoPi = 6.28318531f;

// Small deterministic generator, so placements are identical on every machine
class Random {
public:
    explicit Random(uint32_t seed) : m_state(seed) {}
    float next() // [0, 1)
    {
        m_state = m_state * 1664525u + 1013904223u;
        return float(m_state >> 8) / float(1u << 24);
    }
    float range(float a, float b) { return a + (b - a) * next(); }

private:
    uint32_t m_state;
};

// Vertex attribute encoding, read by flower.vert:
//   aPos   = petal (u, v, 0) | stem (u, around, 0) | core (unit sphere point)
//   aColor = (petal index, petal count of the ring, ring, part), part: 0 petal, 1 stem, 2 core
//   aNormal = (petal length, petal width, petal roundness), aTex = (openness, curl): the species style
enum Part { Petal = 0, Stem = 1, Core = 2 };

// A flower species: its mesh and how its petals are shaped
struct Species {
    int outerPetals;
    int innerPetals;
    float length;    // petal length, 1 = lily
    float width;     // petal width, 1 = lily
    float roundness; // < 1 slender and pointed, > 1 broad with blunt tips
    float openness;  // how far the flower can open, low values stay cup shaped
    float curl;      // how strongly the petals curl back towards the tips
    float nodMin;    // head tilt range, 1 = upside down
    float nodMax;
};

MeshNodePtr buildFlowerMesh(const Species &species)
{
    const int outerPetals = species.outerPetals, innerPetals = species.innerPetals;
    constexpr int petalU = 12, petalV = 6;   // segments along / across a petal
    constexpr int stemU = 16, stemAround = 8;
    constexpr int coreLat = 8, coreLon = 10;

    const int petalVerts = (petalU + 1) * (petalV + 1);
    const int petalTris = petalU * petalV * 2;
    const int petals = outerPetals + innerPetals;
    const int stemVerts = (stemU + 1) * (stemAround + 1);
    const int coreVerts = (coreLat + 1) * (coreLon + 1);

    const int vertexCount = petals * petalVerts + stemVerts + coreVerts;
    const int triangleCount = petals * petalTris + stemU * stemAround * 2 + coreLat * coreLon * 2;

    auto node = MeshNode::create();
    node->newMesh(vertexCount, triangleCount);
    auto mesh = node->lastMesh();
    mesh->setGenerateNormals(false); // shaped on the GPU
    mesh->begin(GL_TRIANGLES);

    int base = 0;
    auto grid = [&](int nu, int nv, auto vertex) {
        for (int i = 0; i <= nu; ++i)
            for (int j = 0; j <= nv; ++j)
            {
                mesh->tex2f(species.openness, species.curl);
                vertex(float(i) / nu, float(j) / nv);
            }
        for (int i = 0; i < nu; ++i)
            for (int j = 0; j < nv; ++j)
            {
                const int a = base + i * (nv + 1) + j, b = a + nv + 1;
                mesh->index3i(a, b, a + 1);
                mesh->index3i(a + 1, b, b + 1);
            }
        base += (nu + 1) * (nv + 1);
    };

    for (int ring = 0; ring < 2; ++ring)
    {
        const int count = ring == 0 ? outerPetals : innerPetals;
        for (int p = 0; p < count; ++p)
            grid(petalU, petalV, [&](float u, float v) {
                mesh->vertex3f(u, v * 2.0f - 1.0f, 0.0f);
                mesh->color4f(float(p), float(count), float(ring), float(Petal));
                mesh->normal3f(species.length, species.width, species.roundness);
            });
    }

    grid(stemU, stemAround, [&](float u, float around) {
        mesh->vertex3f(u, around, 0.0f);
        mesh->color4f(0.0f, 1.0f, 0.0f, float(Stem));
        mesh->normal3f(species.length, species.width, species.roundness);
    });

    grid(coreLat, coreLon, [&](float lat, float lon) {
        const float theta = lat * 3.14159265f, phi = lon * kTwoPi;
        mesh->vertex3f(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));
        mesh->color4f(0.0f, 1.0f, 0.0f, float(Core));
        mesh->normal3f(species.length, species.width, species.roundness);
    });

    mesh->end();
    return node;
}

// outer, inner, length, width, roundness, openness, curl, nod range
const Species kSpecies[] = {
    {6, 0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.08f},    // lily
    {5, 0, 1.3f, 0.55f, 0.75f, 1.0f, 1.6f, 0.0f, 0.1f},   // star lily: long pointed petals
    {9, 6, 0.85f, 1.3f, 1.5f, 0.75f, 0.45f, 0.0f, 0.0f},  // lotus: layered, cupped
    {16, 10, 0.75f, 0.35f, 1.0f, 1.0f, 0.7f, 0.0f, 0.2f}, // aster: many thin rays
    {3, 3, 0.8f, 1.5f, 1.6f, 0.28f, 0.25f, 0.0f, 0.0f},   // tulip: a closed glowing cup
    {5, 0, 0.75f, 1.3f, 1.3f, 0.35f, 1.6f, 0.5f, 0.62f},  // bell: hangs its head like a lantern
};

// Neon dream palette: cyan, magenta, violet, aqua, rose
const glm::vec3 kPalette[] = {
    {0.25f, 0.85f, 1.0f}, {1.0f, 0.3f, 0.75f}, {0.6f, 0.4f, 1.0f}, {0.3f, 1.0f, 0.75f}, {1.0f, 0.55f, 0.6f}};

} // namespace

bool FlowerField::create(const std::string &vertexShader, const std::string &fragmentShader,
                         const std::function<bool(glm::vec2)> &keepClear)
{
    m_material = ShaderMaterial::createFromFiles(vertexShader, fragmentShader, "heaven_flower", true);
    if (!m_material)
        return false;

    const auto &species = kSpecies;
    constexpr int speciesCount = int(std::size(kSpecies));
    // How common each species is along the path
    const float weights[speciesCount] = {1.0f, 0.8f, 1.0f, 0.8f, 0.7f, 0.6f};

    std::vector<InstancedMeshNodePtr> nodes;
    for (const auto &s : species)
        nodes.push_back(InstancedMeshNode::create(buildFlowerMesh(s)));

    Random random(20261109u);
    auto pickSpecies = [&]() {
        float total = 0.0f;
        for (float w : weights)
            total += w;
        float x = random.next() * total;
        for (int i = 0; i < speciesCount; ++i)
            if ((x -= weights[i]) < 0.0f)
                return i;
        return speciesCount - 1;
    };
    auto add = [&](int kind, const glm::mat4 &transform, glm::vec4 color, glm::vec4 data) {
        nodes[kind]->addInstance(transform, color, data);
        m_placements.push_back({transform, color, data});
    };
    // Room around the last flower, so it stands alone at the end
    auto nearLastFlower = [](glm::vec2 p) { return glm::distance(p, glm::vec2(kLastFlower.x, kLastFlower.z)) < 1.8f; };
    auto place = [&](int kind, glm::vec3 pos, float scale, float height) {
        const glm::vec3 color = kPalette[int(random.next() * 5.0f) % 5];
        const float nod = random.range(species[kind].nodMin, species[kind].nodMax);
        const glm::vec4 data(random.next() * 1000.0f, random.next() * kTwoPi, random.next(), height);
        const glm::mat4 transform =
            InstancedMeshNode::makeTransform(pos, {0.0f, random.range(0.0f, 360.0f), 0.0f}, glm::vec3(scale));
        if (!nearLastFlower({pos.x, pos.z}))
            add(kind, transform, glm::vec4(color, nod), data);
    };

    // The last flower: the first to emerge in Act I, rooted through Act III, alone at the end.
    // Order -1 marks it for flower.vert.
    add(2, InstancedMeshNode::makeTransform(kLastFlower, {0.0f, 20.0f, 0.0f}, glm::vec3(1.25f)),
        glm::vec4(1.0f, 0.5f, 0.85f, 0.0f), glm::vec4(512.0f, 1.0f, -1.0f, 1.15f));

    // Flowers along the camera's path, leaving the middle open
    for (int i = 0; i < 34; ++i)
    {
        const float side = (i % 2 == 0) ? -1.0f : 1.0f;
        const glm::vec3 pos(side * random.range(1.6f, 9.0f), 0.0f, random.range(-26.0f, 5.0f));
        const int kind = pickSpecies();
        place(kind, pos, random.range(0.55f, 1.35f), random.range(0.6f, 2.3f) * (kind == 5 ? 1.3f : 1.0f));
    }

    // A few enormous flowers in the distance
    for (int i = 0; i < 5; ++i)
    {
        const glm::vec3 pos(random.range(-14.0f, 14.0f), 0.0f, random.range(-44.0f, -28.0f));
        place(i % 2 == 0 ? 2 : 3, pos, random.range(2.2f, 3.0f), random.range(1.2f, 1.6f));
    }

    // Act II: the garden spreads outwards from its center, in loose clusters
    const glm::vec2 center(0.0f, -15.0f);
    int grown = 0;
    for (int attempt = 0; attempt < 4000 && grown < 260; ++attempt)
    {
        // Denser near the center, thinning out towards the edge of the world
        const float radius = 10.0f + 55.0f * std::pow(random.next(), 0.8f);
        const float angle = random.next() * kTwoPi;
        const glm::vec2 p = center + radius * glm::vec2(std::cos(angle), std::sin(angle));
        const int kind = pickSpecies();
        const float scale = random.range(0.6f, 1.5f) * (1.0f + 0.5f * (radius - 10.0f) / 55.0f);
        const float height = random.range(0.6f, 2.6f) * (kind == 5 ? 1.3f : 1.0f);
        if (keepClear && keepClear(p))
            continue;
        const float order = std::clamp((radius - 10.0f) / 55.0f + random.range(-0.08f, 0.08f), 0.0f, 1.0f);
        const glm::vec3 color = kPalette[int(random.next() * 5.0f) % 5];
        const float nod = random.range(species[kind].nodMin, species[kind].nodMax);
        const glm::vec4 data(random.next() * 1000.0f, random.next() * kTwoPi, 2.0f + order, height);
        const glm::mat4 transform =
            InstancedMeshNode::makeTransform({p.x, 0.0f, p.y}, {0.0f, random.range(0.0f, 360.0f), 0.0f}, glm::vec3(scale));
        // (checked last, so the other placements are the same as without the last flower)
        if (nearLastFlower(p))
            continue;
        add(kind, transform, glm::vec4(color, nod), data);
        ++grown;
    }

    for (const auto &node : nodes)
    {
        node->setMaterial(m_material);
        m_nodes.push_back(node);
    }
    return true;
}

void FlowerField::addIslandFlowers(const std::vector<IslandFlowerSpot> &spots)
{
    // Lilies, star lilies, asters and bells: light, open shapes that read from below
    const int kinds[] = {0, 1, 3, 5};
    Random random(77031u);
    for (const auto &spot : spots)
    {
        const int kind = kinds[int(random.next() * 4.0f) % 4];
        const glm::vec3 color = kPalette[int(random.next() * 5.0f) % 5];
        const float nod = random.range(kSpecies[kind].nodMin, kSpecies[kind].nodMax);
        const glm::vec4 data(random.next() * 1000.0f, random.next() * kTwoPi, 4.0f + float(spot.island), spot.height);
        m_nodes[kind]->addInstance(InstancedMeshNode::makeTransform(spot.local, {0.0f, random.range(0.0f, 360.0f), 0.0f},
                                                                    glm::vec3(spot.scale)),
                                   glm::vec4(color, nod), data);
    }
}
