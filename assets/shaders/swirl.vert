#version 330 core
// Sparkle flowing along one of the looping streams around the garden

layout (location = 0) in vec3 aPos;   // quad corner in [-1, 1]
layout (location = 8) in vec4 aInstanceColor;
layout (location = 9) in vec4 aInstanceData;  // (stream, position along the stream, seed, size)

uniform mat4 view;
uniform mat4 projection;

uniform float time;
uniform float uSwirl;  // how many streams are flowing, 0..1 (timeline)
uniform float uArp;
uniform float uPads;
uniform float uGravity; // 1 = normal, 0 = weightless (Act III)

out vec2 vCorner;
out vec3 vColor;
out float vBrightness;

const float PI = 3.14159265;
const int STREAMS = 8;
const vec3 GARDEN_CENTER = vec3(0.0, 0.0, -15.0);

float hash(float n)
{
    return fract(sin(n * 12.9898 + 78.233) * 43758.5453);
}

// A closed, wavy loop around the garden; each stream has its own size, height and tilt
vec3 streamPoint(float k, float theta)
{
    float radius = mix(6.0, 24.0, hash(k + 0.5));
    float squash = mix(0.5, 1.0, hash(k + 1.5));
    float height = mix(0.8, 5.0, hash(k + 2.5));
    vec3 p = vec3(cos(theta) * radius,
                  height + 1.1 * sin(theta * 2.0 + k * 1.7) + 0.45 * sin(theta * 5.0 + k),
                  sin(theta) * radius * squash);
    p.xz += vec2(sin(theta * 3.0 + k), cos(theta * 2.0 + k * 0.7)) * 2.0;
    float rotation = k * 0.85;
    p.xz = mat2(cos(rotation), sin(rotation), -sin(rotation), cos(rotation)) * p.xz;
    return GARDEN_CENTER + p;
}

void main()
{
    float k = aInstanceData.x;
    float along = aInstanceData.y;
    float seed = aInstanceData.z;
    float size = aInstanceData.w;

    // Streams flow in alternating directions at their own pace
    float speed = mix(0.035, 0.08, hash(k + 3.5)) * (mod(k, 2.0) < 1.0 ? 1.0 : -1.0);
    float theta = 2.0 * PI * along + time * speed;
    vec3 center = streamPoint(k, theta);

    // Weightless: the streams loft into the sky, the high ones highest
    center.y += (1.0 - uGravity) * (4.0 + 3.0 * center.y);

    // A loose ribbon of sparkles around the stream's center line
    center += 0.45 * vec3(sin(seed + time * 0.7), 0.6 * sin(seed * 1.3 + time * 0.5), cos(seed * 0.7 + time * 0.6));

    // Streams come in one after another
    float present = smoothstep(k / float(STREAMS), (k + 1.0) / float(STREAMS), uSwirl);

    // Bright packets travel along the stream; arp notes make them flare
    float packet = pow(max(0.5 + 0.5 * sin(2.0 * PI * along * 7.0 - time * 1.3 + k), 0.0), 8.0);
    float twinkle = 0.6 + 0.4 * sin(time * (2.0 + 3.0 * hash(seed)) + seed);
    vBrightness = present * twinkle * (0.25 + 0.35 * uPads + packet * (0.6 + 3.5 * uArp));

    vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 up = vec3(view[0][1], view[1][1], view[2][1]);
    float radius = size * (1.0 + packet * (0.5 + 1.2 * uArp)) * present;
    vec3 world = center + (right * aPos.x + up * aPos.y) * radius;

    vCorner = aPos.xy;
    vColor = aInstanceColor.rgb;
    gl_Position = projection * view * vec4(world, 1.0);
}
