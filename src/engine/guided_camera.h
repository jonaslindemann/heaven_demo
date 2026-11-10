#pragma once

/**
 * @file guided_camera.h
 * @brief Camera that follows an authored path, with limited freedom for the viewer.
 */

#include <glm/glm.hpp>

namespace demo {

/**
 * @struct CameraPose
 * @brief Where the camera is and what it looks at.
 */
struct CameraPose {
    glm::vec3 position{0.0f, 1.0f, 5.0f};
    glm::vec3 target{0.0f};
};

/**
 * @class GuidedCamera
 * @brief Adds viewer input on top of an authored camera path.
 *
 * The path itself is a pure function of song time (see DemoWindow::cameraPathAt()),
 * so renders and exports are reproducible. While watching, the viewer can look
 * around and float away from the path within limits; after a moment without
 * input the camera drifts back onto the path.
 */
class GuidedCamera {
public:
    struct Settings {
        float maxYaw{0.6f};        ///< Look left/right limit, radians.
        float maxPitch{0.35f};     ///< Look up/down limit, radians.
        float maxOffset{3.0f};     ///< How far the viewer can float from the path.
        float minHeight{0.15f};    ///< Lowest camera height, keeps it above the ground or water.
        float lookSpeed{0.004f};   ///< Radians per pixel of mouse movement.
        float moveSpeed{0.01f};    ///< World units per pixel of mouse movement.
        float returnDelay{1.5f};   ///< Seconds without input before returning to the path.
        float returnRate{0.8f};    ///< Return speed (1/s); about 3/rate seconds to settle.
    };

    [[nodiscard]] Settings &settings() { return m_settings; }

    /** @brief Turn the view; mouse movement in pixels. */
    void look(float dx, float dy);

    /**
     * @brief Float away from the path, relative to the camera.
     * @param delta (right, up, forward) in world units.
     */
    void move(glm::vec3 delta);

    /** @brief Advance the return to the path. */
    void update(double dt);

    /** @brief Back onto the path immediately. */
    void reset();

    /** @brief When disabled, apply() returns the path unchanged (e.g. while exporting frames). */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    [[nodiscard]] bool enabled() const { return m_enabled; }

    /** @brief The path pose with the viewer's offset and look direction applied. */
    [[nodiscard]] CameraPose apply(const CameraPose &path) const;

private:
    Settings m_settings;
    bool m_enabled{true};
    float m_yaw{0.0f};
    float m_pitch{0.0f};
    glm::vec3 m_offset{0.0f}; ///< (right, up, forward)
    double m_idle{0.0};
};

} // namespace demo
