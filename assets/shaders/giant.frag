#version 330 core
// Dark organic stalk with bands of light travelling up it, flaring with the stabs,
// and a glowing growing tip

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vTint;
in float vAlong;
in float vGrow;
in float vSeed;

uniform vec3 viewPos;
uniform vec3 uSky;
uniform float time;
uniform float uGlow;
uniform float uStab;

out vec4 fragColor;

void main()
{
    vec3 n = dot(vNormal, vNormal) > 1e-12 ? normalize(vNormal) : vec3(0.0, 1.0, 0.0);
    if (!gl_FrontFacing)
        n = -n;
    vec3 toEye = normalize(viewPos - vWorldPos);
    float rim = pow(max(1.0 - abs(dot(n, toEye)), 0.0), 2.5);

    vec3 color = vec3(0.015, 0.02, 0.03) + uSky * (0.5 + 2.0 * rim) + vTint * rim * 0.35;

    // Bands of light travelling upwards
    float band = pow(max(0.5 + 0.5 * sin(vAlong * 70.0 - time * 1.4 + vSeed), 0.0), 12.0);
    color += vTint * band * (0.25 + 2.2 * uStab);

    // Faint lengthwise ridges, like a fern stem
    color += vTint * 0.08 * smoothstep(0.7, 1.0, sin(atan(n.z, n.x) * 6.0));

    // The growing tip glows
    color += mix(vTint, vec3(1.0), 0.4) * exp(-(vGrow - vAlong) * 60.0) * 2.0 * step(0.001, vGrow);

    fragColor = vec4(color * (0.6 + 0.6 * uGlow), 1.0);
}
