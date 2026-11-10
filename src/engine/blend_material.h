#pragma once

/**
 * @file blend_material.h
 * @brief Shader material with additive or alpha blending, for glowing particles and veils.
 */

#include <ivf/shader_material.h>

#include <memory>
#include <string>

namespace demo {

/**
 * @class BlendMaterial
 * @brief ShaderMaterial that draws with blending and without depth writes.
 *
 * Additive blending makes overlapping light add up (glowing motes, light
 * trails); alpha blending gives translucent veils. Depth testing stays on, so
 * blended geometry is hidden behind opaque geometry; add blended nodes after the
 * opaque ones. Background mode draws opaque without depth writes, for a sky dome
 * added first: everything drawn later covers it, and the depth buffer still reads
 * as empty sky for post effects such as fog. The previous blend and depth-write
 * state is restored after drawing.
 */
class BlendMaterial : public ivf::ShaderMaterial {
public:
    enum class Mode { Additive, Alpha, Background };

    explicit BlendMaterial(ivf::ProgramPtr program = nullptr, Mode mode = Mode::Additive);

    /**
     * @brief Load a program from shader files.
     * @param watch Reload the files when they change.
     * @return The material, or nullptr if the shaders failed to compile.
     */
    static std::shared_ptr<BlendMaterial> createFromFiles(const std::string &vertPath, const std::string &fragPath,
                                                          const std::string &name, Mode mode, bool watch = false);

    void setMode(Mode mode) { m_mode = mode; }
    [[nodiscard]] Mode mode() const { return m_mode; }

    void apply() override;
    void unapply() override;

private:
    Mode m_mode;
    bool m_blendWasEnabled{false};
    unsigned char m_depthMaskWas{1};
    int m_srcRgb{0}, m_dstRgb{0}, m_srcAlpha{0}, m_dstAlpha{0};
};

using BlendMaterialPtr = std::shared_ptr<BlendMaterial>;

} // namespace demo
