#version 330 core
// Dark mirror lake: a faint slow swell, rings sent out by the bass, and glowing
// roots beneath the surface that light up as waves travel through the garden

in vec3 vWorldPos;

uniform sampler2D reflectionTexture;
uniform bool hasReflection;
uniform vec2 viewportSize;
uniform vec3 viewPos;
uniform float time;

uniform float uGlow;
uniform float uPads;
uniform vec3 uSky;
uniform vec3 uHorizon;  // mist on the horizon (also the fog color)
uniform float uRoots;   // how far the roots have spread, 0..1 (timeline)
uniform float uMirror;  // Act III: the lake stills into a perfect mirror, 0..1 (timeline)
uniform float uRipples; // strength of the wind ripples (timeline)

// Start times of the last waves of light, sent out from the garden's center on each downbeat
uniform vec3 uWaves;

// Pulse rings: (center x, center z, start time, strength)
uniform vec4 uRing0;
uniform vec4 uRing1;
uniform vec4 uRing2;
uniform vec4 uRing3;
uniform vec4 uRing4;
uniform vec4 uRing5;
uniform vec4 uRing6;
uniform vec4 uRing7;

out vec4 fragColor;

const float RING_SPEED = 1.8;     // world units per second
const float RING_WAVELENGTH = 0.45;
const float RING_LIFE = 3.5;      // seconds

const vec2 GARDEN_CENTER = vec2(0.0, -15.0);
const float ROOT_REACH = 75.0;    // radius of the roots at full spread
const float WAVE_SPEED = 14.0;    // world units per second
const float WAVE_LIFE = 5.0;

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

float fbm(vec2 p)
{
    float sum = 0.0, amplitude = 0.5;
    for (int i = 0; i < 4; ++i) {
        sum += amplitude * noise(p);
        p = p * 2.03 + vec2(17.1, 9.2);
        amplitude *= 0.5;
    }
    return sum;
}

// Glow of a meandering line where a noise field crosses its middle value
float rootLine(float v, float core, float halo)
{
    float d = abs(v);
    float aa = max(fwidth(v), 1e-4);
    return clamp(1.0 - d / (core + aa * 1.5), 0.0, 1.0) + 0.35 * exp(-d / halo);
}

// Roots beneath the surface: thick meandering roots with finer branches, spreading
// out from the center of the garden and lit by waves of light
vec3 roots(vec2 p, float dist)
{
    float r = length(p - GARDEN_CENTER);
    float reach = uRoots * ROOT_REACH;
    // (no early out: fwidth() below needs uniform control flow)
    float spread = smoothstep(reach, reach - 8.0, r);

    vec2 q = p * 0.12;
    vec2 warp = vec2(fbm(q + vec2(3.1, 1.7)), fbm(q + vec2(8.3, 2.8)));
    float main = fbm(q + warp * 1.6) - 0.5;
    float branch = fbm(q * 2.7 + warp * 2.2 + vec2(5.0)) - 0.5;
    float lines = rootLine(main, 0.008, 0.02) + 0.55 * rootLine(branch, 0.005, 0.012) * smoothstep(0.12, 0.0, abs(main));

    // Waves of light rolling outwards
    float wave = 0.0;
    for (int i = 0; i < 3; ++i) {
        float age = time - uWaves[i];
        if (age < 0.0 || age > WAVE_LIFE)
            continue;
        float x = r - WAVE_SPEED * age;
        wave += exp(-x * x / 6.0) * exp(-age * 0.5) + 0.35 * exp(-max(-x, 0.0) / 10.0) * step(x, 0.0) * exp(-age * 1.2);
    }

    vec3 tint = mix(vec3(0.1, 0.9, 0.85), vec3(0.65, 0.35, 1.0), smoothstep(0.35, 0.65, fbm(q * 0.6 + 11.0)));
    float light = (0.12 + 0.2 * uPads + 1.6 * wave) * (0.5 + 0.5 * uGlow);
    // Fine lines alias into noise far away; let them fade
    float fade = 1.0 / (1.0 + dist * 0.03);
    return tint * lines * light * spread * fade;
}

// Slow swell: a few long waves, so the mirror is alive but not broken
vec2 swellSlope(vec2 p, float t);

// Wind ripples: small waves drifting with the wind. Octaves finer than a pixel are
// left out (footprint = world size of a pixel), so they shimmer instead of sparkling.
float rippleHeight(vec2 p, float t, float footprint)
{
    const vec2 wind = vec2(0.8, -0.45);
    float h = 0.0, amplitude = 0.5, frequency = 1.6;
    for (int i = 0; i < 3; ++i) {
        float resolved = smoothstep(0.8, 0.25, footprint * frequency);
        vec2 drift = wind * t * (0.5 + 0.35 * float(i)) + vec2(-t * 0.2, t * 0.3) * float(i);
        h += amplitude * resolved * noise(p * frequency + drift + float(i) * 13.7);
        amplitude *= 0.5;
        frequency *= 2.3;
    }
    return h;
}

vec2 rippleSlope(vec2 p, float t, float footprint)
{
    // Gusts: patches of rough water drifting across calmer water
    float gust = smoothstep(0.3, 0.7, fbm(p * 0.045 + vec2(-0.06, 0.03) * t));
    const float e = 0.05;
    float h = rippleHeight(p, t, footprint);
    vec2 slope = vec2(rippleHeight(p + vec2(e, 0.0), t, footprint) - h,
                      rippleHeight(p + vec2(0.0, e), t, footprint) - h) / e;
    return slope * 0.11 * mix(0.3, 1.0, gust);
}

vec2 swellSlope(vec2 p, float t)
{
    const vec2 dirs[3] = vec2[3](vec2(1.0, 0.35), vec2(-0.5, 1.0), vec2(0.3, -1.0));
    const float lengths[3] = float[3](9.0, 5.5, 3.1);
    vec2 slope = vec2(0.0);
    for (int i = 0; i < 3; ++i) {
        vec2 d = normalize(dirs[i]);
        float k = 6.2831853 / lengths[i];
        slope += d * k * cos(dot(d, p) * k + t * 0.35 * sqrt(k));
    }
    return slope * 0.004;
}

// Height of one ring and the brightness of its crest
vec2 ring(vec4 r, vec2 p)
{
    float age = time - r.z;
    if (r.w <= 0.0 || age < 0.0 || age > RING_LIFE)
        return vec2(0.0);
    float front = RING_SPEED * age;
    float x = length(p - r.xy) - front;      // distance behind the wavefront
    float width = 0.35 + 0.3 * age;           // the wave packet spreads as it travels
    float fade = r.w * exp(-age * 1.1) * smoothstep(0.0, 0.15, age);
    float packet = exp(-(x * x) / (width * width));
    float height = fade * packet * sin(6.2831853 * x / RING_WAVELENGTH);
    float crest = fade * exp(-(x * x) / 0.02);
    return vec2(height, crest);
}

vec2 rings(vec2 p)
{
    return ring(uRing0, p) + ring(uRing1, p) + ring(uRing2, p) + ring(uRing3, p) +
           ring(uRing4, p) + ring(uRing5, p) + ring(uRing6, p) + ring(uRing7, p);
}

void main()
{
    vec2 p = vWorldPos.xz;
    float dist = length(viewPos - vWorldPos);

    // Ring slope by central differences
    const float e = 0.03;
    vec2 c = rings(p);
    float hx = rings(p + vec2(e, 0.0)).x - rings(p - vec2(e, 0.0)).x;
    float hz = rings(p + vec2(0.0, e)).x - rings(p - vec2(0.0, e)).x;
    vec2 ringSlope = vec2(hx, hz) / (2.0 * e) * 0.12;

    // Fine ripples alias into noise in the distance; let them fade into the mirror
    float detail = 1.0 / (1.0 + fwidth(length(p)) * 6.0);
    vec2 slope = (swellSlope(p, time) + ringSlope * detail) * (1.0 - 0.92 * uMirror);
    // Ripples calm down in the Act III mirror, but never stop completely
    float footprint = length(fwidth(p));
    slope += rippleSlope(p, time, footprint) * uRipples * (1.0 - 0.6 * uMirror);

    vec3 n = normalize(vec3(-slope.x, 1.0, -slope.y));
    vec3 v = normalize(viewPos - vWorldPos);
    float cosTheta = max(dot(n, v), 0.0);
    float fresnel = mix(mix(0.55, 0.93, uMirror), 1.0, pow(max(1.0 - cosTheta, 0.0), 4.0)); // (pow of a rounding-negative is NaN)

    vec3 deep = uSky * 0.25 + vec3(0.002, 0.004, 0.008);
    vec3 reflection = uSky;
    if (hasReflection) {
        vec2 uv = gl_FragCoord.xy / viewportSize + n.xz * 0.06;
        reflection = texture(reflectionTexture, clamp(uv, vec2(0.001), vec2(0.999))).rgb;
        // A single NaN in the reflection would spread through the glow's blur; drop it
        // (tested on the bits, since isnan() may be optimized away)
        uvec3 bits = floatBitsToUint(reflection) & uvec3(0x7fffffffu);
        if (any(greaterThanEqual(bits, uvec3(0x7f800000u))))
            reflection = uSky;
    }

    vec3 color = mix(deep, reflection, fresnel);

    // Seen through the water: clearest when looking down, faint at grazing angles
    color += roots(p + n.xz * 0.3, dist) * (1.0 - 0.6 * fresnel) * (1.0 - 0.75 * uMirror);

    // The crest of each ring glows faintly, like disturbed bioluminescent water
    color += vec3(0.25, 0.85, 1.0) * c.y * detail * (0.3 + 0.5 * uGlow) / (1.0 + 0.04 * dist) * (1.0 - 0.7 * uMirror);

    // Fade into the mist before the far plane clips the lake, matching the sky below the
    // horizon (sky.frag), so the lake's edge is never seen, even from high above
    color = mix(color, uHorizon, smoothstep(60.0, 390.0, dist));

    fragColor = vec4(color, 1.0);
}
