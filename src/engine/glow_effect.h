#pragma once

/**
 * @file glow_effect.h
 * @brief Wide bloom from the image's mip chain, with soft tone mapping.
 */

#include <ivf/effect.h>

namespace demo {

/**
 * @class GlowEffect
 * @brief Bloom that spreads bright light into a soft, wide halo, then tone maps.
 *
 * Bright parts of the image (above the threshold, with a soft knee) are taken
 * from several mip levels of the input (blurs of increasing size), each smoothed
 * with a small tent filter, and added back. The result is tone mapped with a
 * soft shoulder so emissive values above 1 roll off into white instead of
 * clipping. Place it last in the effect chain.
 */
class GlowEffect : public ivf::Effect {
private:
    float m_threshold{0.6f}; ///< Brightness where glow starts.
    float m_intensity{0.8f}; ///< Amount of glow added.
    float m_spread{0.75f};   ///< Weight falloff per blur level; higher gives wider halos [0, 1].
    float m_exposure{1.0f};  ///< Scale before tone mapping.

public:
    GlowEffect();

    static std::shared_ptr<GlowEffect> create();

    void setThreshold(float threshold) { m_threshold = threshold; }
    void setIntensity(float intensity) { m_intensity = intensity; }
    void setSpread(float spread) { m_spread = spread; }
    void setExposure(float exposure) { m_exposure = exposure; }

    [[nodiscard]] float threshold() const { return m_threshold; }
    [[nodiscard]] float intensity() const { return m_intensity; }
    [[nodiscard]] float spread() const { return m_spread; }
    [[nodiscard]] float exposure() const { return m_exposure; }

protected:
    void doLoad() override;
    void doUpdateParams() override;
    void setupProperties() override;
};

using GlowEffectPtr = std::shared_ptr<GlowEffect>;

} // namespace demo
