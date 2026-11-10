#include "lake_material.h"

#include <ivf/logger.h>
#include <ivf/shader_manager.h>
#include <ivf/shader_watcher.h>

#include <glad/glad.h>

using namespace heaven;

LakeMaterial::LakeMaterial(ivf::ProgramPtr program, ivf::PlanarReflectionPtr reflection)
    : ShaderMaterial(program), m_reflection(reflection)
{
    for (int i = 0; i < MaxRings; ++i)
        setRing(i, glm::vec2(0.0f), 0.0f, 0.0f);
}

std::shared_ptr<LakeMaterial> LakeMaterial::createFromFiles(const std::string &vertPath, const std::string &fragPath,
                                                            ivf::PlanarReflectionPtr reflection, bool watch)
{
    auto shaderMgr = ivf::ShaderManager::instance();
    auto program = shaderMgr->loadProgramFromFiles(vertPath, fragPath, "heaven_lake", false);
    if (shaderMgr->compileLinkErrors())
    {
        ivf::logError("Lake shaders failed to compile or link.", "LakeMaterial");
        return nullptr;
    }
    if (watch)
        ivf::ShaderWatcher::instance()->watch(program, vertPath, fragPath);
    return std::make_shared<LakeMaterial>(program, reflection);
}

void LakeMaterial::setRing(int index, glm::vec2 center, float startTime, float strength)
{
    if (index >= 0 && index < MaxRings)
        setUniform("uRing" + std::to_string(index), glm::vec4(center, startTime, strength));
}

void LakeMaterial::apply()
{
    // The reflection texture is recreated on resize, so bind it fresh every frame
    GLuint texture = m_reflection ? m_reflection->texture() : 0;
    if (texture != 0)
        setTexture("reflectionTexture", texture, 0);
    else
        removeTexture("reflectionTexture");
    setUniform("hasReflection", texture != 0);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    setUniform("viewportSize", glm::vec2(float(viewport[2]), float(viewport[3])));

    ShaderMaterial::apply();
}
