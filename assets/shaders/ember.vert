#version 330 core
// Ember: a spark leaving a dissolving flower and rising into the dark.
// Each instance belongs to one flower and has the flower's transform and data,
// so it can find the flower's head (where Act III has carried it) and burn away
// with it; see dissolveStart() in flower.vert.

layout (location = 0) in vec3 aPos;   // quad corner in [-1, 1]
layout (location = 4) in mat4 aInstanceModel; // the flower's transform
layout (location = 8) in vec4 aInstanceColor; // (flower tint, ember index)
layout (location = 9) in vec4 aInstanceData;  // the flower's (seed, phase, emergence order, stem height)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float time;
uniform float uGrowth;
uniform float uFloat;
uniform float uDissolve;
uniform vec3 uLastFlower;
uniform float uPads;

out vec2 vCorner;
out vec3 vColor;
out float vBrightness;

const float PI = 3.14159265;
// The timeline raises dissolve linearly over 10 bars (24 s): seconds per unit of dissolve
const float DISSOLVE_SECONDS = 24.0;

float hash(float n)
{
    return fract(sin(n * 12.9898 + 4.1414) * 43758.5453);
}

// Same as in flower.vert
float dissolveStart(vec3 base, float seed)
{
    float distance = length(base.xz - uLastFlower.xz);
    return 0.72 * (1.0 - clamp(distance / 85.0, 0.0, 1.0)) + 0.06 * hash(seed + 11.0);
}

void main()
{
    float seed = aInstanceData.x;
    float phase = aInstanceData.y;
    float index = aInstanceColor.a;
    float key = seed + index * 1.37;

    mat4 m = model * aInstanceModel;
    vec3 base = (m * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    float scale = length(mat3(m)[0]);

    // Leaves the flower at some moment while it burns, lives a few seconds
    float start = dissolveStart(base, seed);
    float age = (uDissolve - (start + 0.2 * hash(key + 20.0))) * DISSOLVE_SECONDS;
    float life = mix(3.5, 7.0, hash(key + 21.0));
    if (age <= 0.0 || age >= life) {
        vCorner = vec2(0.0);
        vColor = vec3(0.0);
        vBrightness = 0.0;
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0); // beyond the far plane: clipped
        return;
    }

    // The flower's head, lifted as in flower.vert's weightless drift
    vec3 head = base + vec3(0.0, aInstanceData.w * (1.0 + 0.3 * uGrowth) * scale, 0.0);
    if (uFloat > 0.0) {
        float speed = mix(0.12, 0.55, hash(seed + 6.0));
        float height = uFloat * speed;
        float wander = 2.0 * PI * hash(seed + 7.0);
        head += vec3(cos(wander), 0.0, sin(wander)) * 0.15 * height + vec3(0.0, height, 0.0);
        head += 0.25 * vec3(sin(time * 0.4 + phase), 0.0, cos(time * 0.33 + phase)) * min(uFloat * 0.2, 1.0);
    }

    // From somewhere on the petals, rising and spiralling outwards, a little faster as it goes
    float a = 2.0 * PI * hash(key + 22.0);
    vec3 outward = vec3(cos(a), 0.0, sin(a));
    vec3 origin = head + (outward * hash(key + 23.0) * 0.6 + vec3(0.0, 0.25 * (hash(key + 24.0) - 0.3), 0.0)) * scale;
    float rise = mix(0.3, 0.9, hash(key + 25.0));
    float swirl = time * mix(0.6, 1.4, hash(key + 26.0)) + a;
    vec3 center = origin + outward * 0.35 * age + vec3(0.0, rise * age + 0.04 * age * age, 0.0) +
                  0.3 * sqrt(age) * vec3(sin(swirl), 0.0, cos(swirl));

    float t = age / life;
    vBrightness = smoothstep(0.0, 0.25, age) * pow(1.0 - t, 1.5) * (1.8 + 0.8 * uPads);
    vColor = mix(mix(aInstanceColor.rgb, vec3(1.0, 0.85, 0.6), 0.5), aInstanceColor.rgb, smoothstep(0.0, 0.4, t));

    vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 up = vec3(view[0][1], view[1][1], view[2][1]);
    float radius = mix(0.07, 0.15, hash(key + 27.0)) * sqrt(scale) * (1.0 - 0.5 * t);
    vec3 world = center + (right * aPos.x + up * aPos.y) * radius;

    vCorner = aPos.xy;
    gl_Position = projection * view * vec4(world, 1.0);
}
