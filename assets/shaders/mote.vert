#version 330 core
// Glowing mote: a camera-facing quad drifting on slow swirling paths

layout (location = 0) in vec3 aPos;   // quad corner in [-1, 1]
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTex;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in mat4 aInstanceModel; // home position
layout (location = 8) in vec4 aInstanceColor;
layout (location = 9) in vec4 aInstanceData;  // (seed, size, sparkle group, phase)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float time;
uniform float uPads;
uniform float uArp;
uniform float uReveal;
uniform float uFloat;   // seconds of weightlessness so far (Act III)
uniform float uDissolve; // Act IV: the motes go out from the edge inwards, like the flowers
uniform vec3 uLastFlower;

out vec2 vCorner;
out vec3 vColor;
out float vBrightness;

void main()
{
    float seed = aInstanceData.x;
    float size = aInstanceData.y;
    float group = aInstanceData.z;
    float phase = aInstanceData.w;

    // Layered slow swirls: never still, never fast
    vec3 drift = vec3(
        0.9 * sin(time * 0.11 + seed) + 0.35 * sin(time * 0.37 + seed * 1.7),
        0.35 * sin(time * 0.16 + seed * 2.3) + 0.15 * sin(time * 0.53 + phase),
        0.9 * cos(time * 0.09 + seed * 0.6) + 0.35 * cos(time * 0.29 + seed * 2.9));
    vec3 center = (model * aInstanceModel * vec4(0.0, 0.0, 0.0, 1.0)).xyz + drift;

    // Weightless: the motes rise, wrapping around a tall column so the air stays full
    const float COLUMN = 30.0;
    float rise = uFloat * mix(0.2, 0.8, fract(seed * 0.618));
    // (wraps within [-1, COLUMN - 1), so before Act III nothing moves)
    center.y = mod(center.y + rise + 1.0, COLUMN) - 1.0;
    float edgeFade = smoothstep(-1.0, 0.0, center.y) * smoothstep(COLUMN - 1.0, COLUMN - 4.0, center.y);

    // Twinkle, a gentle swell with the pads, and sparkles on arp notes for a third of the motes
    float twinkle = 0.55 + 0.45 * sin(time * (0.8 + group) + phase);
    float sparkle = group < 0.33 ? uArp * 3.0 : 0.0;
    float start = 0.75 * (1.0 - clamp(length(center.xz - uLastFlower.xz) / 40.0, 0.0, 1.0)) + 0.2 * fract(seed * 0.37);
    float remain = 1.0 - smoothstep(start, start + 0.05, uDissolve);
    vBrightness = (0.5 + 0.6 * uPads + sparkle) * twinkle * (0.6 + 0.4 * uReveal) * edgeFade * remain;

    // Expand the quad towards the camera
    vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 up = vec3(view[0][1], view[1][1], view[2][1]);
    float radius = size * (1.0 + 0.6 * uPads + 0.8 * sparkle);
    vec3 world = center + (right * aPos.x + up * aPos.y) * radius;

    vCorner = aPos.xy;
    vColor = aInstanceColor.rgb;
    gl_Position = projection * view * vec4(world, 1.0);
}
