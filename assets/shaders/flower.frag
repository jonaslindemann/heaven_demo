#version 330 core
// Translucent, emissive flower shading

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vTint;
in vec3 vTipTint;
in vec3 vParam;   // (u, v, part): part 0 petal, 1 stem, 2 core
in float vVisible;
in float vDissolve;
in float vSeed;
in float vLast;

uniform vec3 viewPos;
uniform float uGlow;
uniform float uBass;
uniform float uLead;
uniform float uPads;
uniform float uLastLight;

out vec4 fragColor;

float hash(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x),
               u.y);
}

void main()
{
    // Dissolving: the surface burns away in patches, with glowing edges
    float burn = 0.0;
    if (vDissolve > 0.0) {
        vec2 p = vParam.xy * vec2(5.0, 2.5) + vec2(vParam.z * 7.3 + fract(vSeed) * 50.0, vSeed * 0.013);
        float pattern = 0.65 * noise(p) + 0.35 * noise(p * 2.7 + 3.1);
        float threshold = vDissolve * 1.15 - 0.05;
        if (pattern < threshold)
            discard;
        burn = (1.0 - smoothstep(0.0, 0.06, pattern - threshold)) * smoothstep(0.0, 0.05, vDissolve);
    }

    // Normals of neighbouring folded petals can interpolate to zero
    vec3 n = dot(vNormal, vNormal) > 1e-12 ? normalize(vNormal) : vec3(0.0, 1.0, 0.0);
    if (!gl_FrontFacing)
        n = -n;
    vec3 toEye = normalize(viewPos - vWorldPos);
    float facing = abs(dot(n, toEye));
    float rim = pow(max(1.0 - facing, 0.0), 2.0); // facing can exceed 1 by rounding; pow() of a negative is NaN

    float part = vParam.z;
    vec3 color;

    if (part < 0.5) {
        float u = vParam.x;
        float v = vParam.y;
        // Light gathers towards the edges and tips, faint veins run along the petal
        float veins = 0.8 + 0.2 * cos(v * 14.0);
        float edge = smoothstep(0.55, 1.0, abs(v));
        vec3 tint = mix(vTint, vTipTint, smoothstep(0.35, 1.0, u));
        vec3 body = tint * mix(0.15, 0.9, u) * veins;
        color = body * (0.35 + 1.4 * rim + 0.6 * edge);
        color += tint * pow(u, 4.0) * 3.0; // glowing tips, brighter than white for the bloom
        color *= 0.45 + uGlow * (1.3 + 0.5 * uPads);
    } else if (part < 1.5) {
        color = vTint * 0.05 + vec3(0.02, 0.06, 0.05) * (0.3 + rim);
    } else {
        // The heart of the flower pulses with the lead and the bass
        vec3 heart = mix(vTint, vec3(1.0, 0.95, 0.85), 0.6);
        color = heart * (2.0 + 4.0 * uLead + 2.0 * uBass) * (0.4 + uGlow);
    }

    // Close to the camera a flower would fill the frame with white; soften it
    float nearDim = mix(0.5, 1.0, smoothstep(1.5, 7.0, length(viewPos - vWorldPos)));
    color += mix(vTint, vec3(1.0, 0.85, 0.6), 0.5) * burn * 3.0;
    color *= 1.0 + 0.8 * uLastLight * vLast;
    fragColor = vec4(color * vVisible * nearDim, 1.0);
}
