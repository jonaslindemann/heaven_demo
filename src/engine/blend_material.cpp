#include "blend_material.h"

#include <ivf/logger.h>
#include <ivf/shader_manager.h>
#include <ivf/shader_watcher.h>

#include <glad/glad.h>

using namespace demo;

BlendMaterial::BlendMaterial(ivf::ProgramPtr program, Mode mode) : ShaderMaterial(program), m_mode(mode) {}

std::shared_ptr<BlendMaterial> BlendMaterial::createFromFiles(const std::string &vertPath, const std::string &fragPath,
                                                              const std::string &name, Mode mode, bool watch)
{
    auto shaderMgr = ivf::ShaderManager::instance();
    auto program = shaderMgr->loadProgramFromFiles(vertPath, fragPath, name, false);
    if (shaderMgr->compileLinkErrors())
    {
        ivf::logError("Shader program '" + name + "' failed to compile or link.", "BlendMaterial");
        return nullptr;
    }
    if (watch)
        ivf::ShaderWatcher::instance()->watch(program, vertPath, fragPath);
    return std::make_shared<BlendMaterial>(program, mode);
}

void BlendMaterial::apply()
{
    ShaderMaterial::apply();

    m_blendWasEnabled = glIsEnabled(GL_BLEND) == GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &m_depthMaskWas);
    glGetIntegerv(GL_BLEND_SRC_RGB, &m_srcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &m_dstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &m_srcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &m_dstAlpha);

    if (m_mode == Mode::Background)
        glDisable(GL_BLEND);
    else
        glEnable(GL_BLEND);
    if (m_mode == Mode::Additive)
        glBlendFunc(GL_ONE, GL_ONE);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
}

void BlendMaterial::unapply()
{
    glDepthMask(m_depthMaskWas);
    glBlendFuncSeparate(m_srcRgb, m_dstRgb, m_srcAlpha, m_dstAlpha);
    if (m_blendWasEnabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    ShaderMaterial::unapply();
}
