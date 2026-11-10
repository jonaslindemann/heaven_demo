#version 330 core
// Night sky: a luminous mist on the horizon, a deep zenith and faint stars.
// uSky is the sky color, uHorizon the brighter mist on the horizon. The fog uses
// uHorizon too, so the distant lake and the sky fade into the same mist.

in vec3 vDir;

uniform float time;
uniform float uGlow;
uniform vec3 uSky;
uniform vec3 uHorizon;
uniform float uAurora;  // Act III: curtains of light in the sky, 0..1 (timeline)
uniform float uPads;

out vec4 fragColor;

float hash1(float n)
{
    return fract(sin(n * 127.1) * 43758.5453);
}

float noise1(float x)
{
    float i = floor(x);
    float f = fract(x);
    return mix(hash1(i), hash1(i + 1.0), f * f * (3.0 - 2.0 * f));
}

// Curtains of light hanging in the sky, folding slowly, with fine vertical rays
vec3 aurora(vec3 dir, float h)
{
    float azimuth = atan(dir.z, dir.x);
    vec3 color = vec3(0.0);
    for (int layer = 0; layer < 3; ++layer) {
        float l = float(layer);
        float x = azimuth * (3.0 + l) + l * 17.0;
        float fold = noise1(x + time * (0.05 + 0.02 * l)) + 0.5 * noise1(x * 2.3 - time * 0.07);
        float base = 0.12 + 0.12 * l + 0.08 * sin(azimuth * 2.0 + time * 0.03 + l);
        float above = h - base;
        // Sharp lower edge, long soft fade upwards
        float curtain = smoothstep(-0.02, 0.02, above) * exp(-max(above, 0.0) / (0.12 + 0.06 * l));
        // Fine, irregular rays: two frequencies that drift against each other
        float rays = 0.8 + 0.12 * sin(azimuth * 97.0 + fold * 13.0 + l * 3.0) + 0.08 * sin(azimuth * 211.0 - fold * 7.0);
        float strength = smoothstep(0.75, 1.2, fold) * curtain * rays;
        vec3 tint = mix(vec3(0.2, 1.0, 0.7), vec3(0.75, 0.35, 1.0), clamp(smoothstep(0.0, 0.25, above) + 0.3 * l, 0.0, 1.0));
        color += tint * strength;
    }
    return color;
}

float hash(vec3 p)
{
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.x + p.y) * p.z);
}

void main()
{
    vec3 dir = normalize(vDir);
    float h = dir.y;

    vec3 zenith = uSky * 0.3;
    vec3 color = mix(uSky, zenith, pow(clamp(h, 0.0, 1.0), 0.45));

    // Glowing band of mist just above the water (its brightness breathes with the pads)
    color += (uHorizon - uSky) * exp(-abs(h) * 10.0);

    // Below the horizon, past the end of the lake (the far plane): the distant water,
    // lost in mist. lake.frag fades into the same color before it is clipped.
    if (h < 0.0)
        color = uHorizon;

    if (uAurora > 0.0)
        color += aurora(dir, h) * uAurora * (0.5 + 0.5 * uPads) * 0.35;

    // Stars: one candidate per cell of a grid on the dome
    vec3 cell = floor(dir * 160.0);
    float pick = hash(cell);
    if (pick > 0.985) {
        vec3 center = (cell + 0.5 + 0.35 * (vec3(hash(cell + 1.7), hash(cell + 4.1), hash(cell + 8.3)) - 0.5)) / 160.0;
        float d = length(dir - normalize(center)) * 160.0;
        float twinkle = 0.65 + 0.35 * sin(time * (1.0 + 3.0 * hash(cell + 2.3)) + pick * 60.0);
        float brightness = (pick - 0.985) / 0.015;
        color += vec3(0.75, 0.85, 1.0) * exp(-d * d * 18.0) * brightness * twinkle * 0.9 * smoothstep(0.03, 0.25, h);
    }

    fragColor = vec4(color, 1.0);
}
