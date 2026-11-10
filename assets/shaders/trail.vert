#version 330 core
// Light trail of one lead note: the head sweeps a rising arc, the tail follows,
// and the whole ribbon fades after the note has ended

layout (location = 0) in vec3 aPos;   // (u along the trail: 0 tail, 1 head; side in [-1, 1])
layout (location = 4) in mat4 aInstanceModel; // origin on the water
layout (location = 8) in vec4 aInstanceColor;
layout (location = 9) in vec4 aInstanceData;  // (note start, note duration, seed, pitch in [0, 1])

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

uniform float time;
uniform float uTrails;  // overall strength (timeline)
uniform float uGlow;

out float vSide;
out float vAlong;
out vec3 vColor;
out float vIntensity;

const float TRAVEL = 2.2;   // seconds for the head to complete the arc
const float FADE = 1.8;     // fade after the note ends
const float LENGTH = 0.45;  // tail length, as a fraction of the arc

float hash(float n)
{
    return fract(sin(n * 12.9898 + 78.233) * 43758.5453);
}

// The arc: a widening, rising spiral around the origin; higher notes climb higher
vec3 arc(float s, float seed, float pitch)
{
    float turn = mix(1.4, 2.8, hash(seed)) * (hash(seed + 1.0) < 0.5 ? -1.0 : 1.0);
    float angle = 6.2831853 * hash(seed + 2.0) + turn * s;
    float radius = mix(1.2, 3.2, hash(seed + 3.0)) * (0.35 + 0.65 * s);
    float height = mix(1.2, 4.5, pitch) * (1.0 - (1.0 - s) * (1.0 - s));
    return vec3(cos(angle) * radius, 0.1 + height, sin(angle) * radius);
}

void main()
{
    float start = aInstanceData.x;
    float duration = aInstanceData.y;
    float seed = aInstanceData.z;
    float pitch = aInstanceData.w;
    float age = time - start;

    vSide = aPos.y;
    vAlong = aPos.x;
    vColor = aInstanceColor.rgb;

    if (age < 0.0 || age > duration + FADE + TRAVEL) {
        vIntensity = 0.0;
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0); // outside the view: the ribbon is not drawn
        return;
    }

    // The head moves fast at first and settles; the tail follows
    float head = 1.0 - pow(1.0 - clamp(age / TRAVEL, 0.0, 1.0), 2.0);
    float tail = max(head - LENGTH, 0.0);
    float s = mix(tail, head, aPos.x);

    mat4 m = model * aInstanceModel;
    vec3 p = (m * vec4(arc(s, seed, pitch), 1.0)).xyz;
    vec3 ahead = (m * vec4(arc(min(s + 0.01, 1.0), seed, pitch), 1.0)).xyz;
    vec3 behind = (m * vec4(arc(max(s - 0.01, 0.0), seed, pitch), 1.0)).xyz;
    vec3 tangent = ahead - behind;
    tangent = dot(tangent, tangent) > 1e-10 ? normalize(tangent) : vec3(0.0, 1.0, 0.0);

    // Face the camera: widen across the trail, perpendicular to the view direction
    vec3 toEye = normalize(viewPos - p);
    vec3 side = cross(tangent, toEye);
    side = dot(side, side) > 1e-8 ? normalize(side) : vec3(1.0, 0.0, 0.0);
    float width = 0.09 * (0.25 + 0.75 * aPos.x);
    p += side * aPos.y * width;

    float fadeOut = 1.0 - smoothstep(duration, duration + FADE, age);
    vIntensity = uTrails * fadeOut * (0.2 + 0.8 * aPos.x * aPos.x) * (0.6 + 0.6 * uGlow);
    gl_Position = projection * view * vec4(p, 1.0);
}
