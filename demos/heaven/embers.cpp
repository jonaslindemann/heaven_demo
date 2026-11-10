#include "embers.h"

#include <ivf/mesh_node.h>

using namespace heaven;
using namespace ivf;

bool Embers::create(const std::string &vertexShader, const std::string &fragmentShader,
                    const std::vector<FlowerPlacement> &flowers, int perFlower)
{
    m_material = demo::BlendMaterial::createFromFiles(vertexShader, fragmentShader, "heaven_ember",
                                                      demo::BlendMaterial::Mode::Additive, true);
    if (!m_material)
        return false;

    // One quad; its corners are expanded towards the camera in ember.vert
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
    for (const auto &flower : flowers)
    {
        if (flower.data.z < -0.5f)
            continue;
        // Color: the flower's tint, with the ember's index in place of the nod
        for (int i = 0; i < perFlower; ++i)
            m_node->addInstance(flower.transform, glm::vec4(glm::vec3(flower.color), float(i)), flower.data);
    }
    m_node->setMaterial(m_material);
    return true;
}
