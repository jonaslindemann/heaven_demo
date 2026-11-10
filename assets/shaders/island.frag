#version 330 core
// Dark rock with glowing moss on top, veins of light that pulse with the stabs,
// and a bioluminescent glow at the tip of the underside

in vec3 vWorldPos;
in vec3 vLocal;
in vec3 vCenter;
in vec3 vTint;
in float vSeed;

uniform vec3 viewPos;
uniform vec3 uSky;
uniform float uGlow;
uniform float uPads;
uniform float uStab;
uniform float uDissolve;  // Act IV: burns away on the same front as the flowers (see flower.vert)
uniform vec3 uLastFlower;

out vec4 fragColor;

float hash(vec3 p)
{
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(i + vec3(0, 0, 0)), hash(i + vec3(1, 0, 0)), f.x),
                   mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x),
                   mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

void main()
{
    float burn = 0.0;
    float start = 0.72 * (1.0 - clamp(length(vCenter.xz - uLastFlower.xz) / 85.0, 0.0, 1.0));
    float dissolve = smoothstep(start, start + 0.22, uDissolve);
    if (dissolve > 0.0) {
        float pattern = 0.65 * noise(vLocal * 1.6 + vSeed) + 0.35 * noise(vLocal * 4.3 + vSeed);
        float threshold = dissolve * 1.15 - 0.05;
        if (pattern < threshold)
            discard;
        burn = 1.0 - smoothstep(0.0, 0.05, pattern - threshold);
    }

    // Faceted normal from screen-space derivatives, facing away from the island's center
    vec3 n = cross(dFdx(vWorldPos), dFdy(vWorldPos));
    n = dot(n, n) > 1e-12 ? normalize(n) : vec3(0.0, 1.0, 0.0);
    if (dot(n, vWorldPos - vCenter) < 0.0)
        n = -n;
    vec3 toEye = normalize(viewPos - vWorldPos);
    float rim = pow(max(1.0 - abs(dot(n, toEye)), 0.0), 3.0);

    vec3 rock = vec3(0.02, 0.025, 0.04) * (0.5 + 0.5 * max(n.y, 0.0)) + uSky * (0.6 + 2.5 * rim);

    // Glowing moss on the flat top
    float top = smoothstep(0.55, 0.9, n.y) * step(0.0, vLocal.y);
    vec3 color = rock + vTint * top * (0.25 + 0.35 * uPads);

    // Veins of light, pulsing with the stabs
    float field = noise(vLocal * 4.0 + vSeed) + 0.5 * noise(vLocal * 9.0 + vSeed * 1.7);
    float vein = abs(field - 0.75);
    float line = 1.0 - smoothstep(0.0, 0.018 + fwidth(field), vein);
    color += vTint * line * (0.2 + 2.2 * uStab);

    // The tip of the underside glows like a drop of light
    float tip = smoothstep(-1.5, -2.8, vLocal.y);
    color += mix(vTint, vec3(1.0, 0.6, 0.85), 0.5) * tip * tip * 2.0;

    color += mix(vTint, vec3(1.0, 0.85, 0.6), 0.5) * burn * 3.0;
    fragColor = vec4(color * (0.6 + 0.6 * uGlow), 1.0);
}
