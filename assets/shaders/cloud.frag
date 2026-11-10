#version 330 core
// Soft glowing cloud: billowing noise inside a soft-edged blob; drawn with additive blending

in vec2 vCorner;
in vec3 vColor;
in float vSeed;
in float vPresence;

uniform float time;
uniform float uPads;
uniform float uClap;
uniform float uGlow;

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

float fbm(vec2 p)
{
    float sum = 0.0, amplitude = 0.5;
    for (int i = 0; i < 5; ++i) {
        sum += amplitude * noise(p);
        p = p * 2.02 + vec2(5.3, 1.7);
        amplitude *= 0.5;
    }
    return sum;
}

void main()
{
    if (vPresence <= 0.001)
        discard;
    float r2 = dot(vCorner, vCorner);
    if (r2 > 1.0)
        discard;

    vec2 p = vCorner * vec2(2.4, 1.1) + vec2(vSeed, vSeed * 0.37) + vec2(time * 0.01, 0.0);
    float billow = fbm(p + 0.6 * vec2(fbm(p * 0.7 + 3.0), fbm(p * 0.7 + 7.0)));
    float density = smoothstep(0.35, 0.8, billow * (1.0 - r2) * 1.6);

    // Brighter, warmer cores; breathing with the pads, flashing on claps
    vec3 color = mix(vColor * 1.2, vec3(1.0, 0.92, 0.97), density * density * 0.5);
    float light = (0.14 + 0.2 * uPads + 0.35 * uClap) * (0.7 + 0.3 * uGlow);
    fragColor = vec4(color * density * light * vPresence, 1.0);
}
