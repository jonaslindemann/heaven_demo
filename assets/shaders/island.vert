#version 330 core
// Floating island: a sphere shaped into a flat top and a long tapering underside

layout (location = 0) in vec3 aPos;   // unit sphere
layout (location = 8) in vec4 aInstanceColor;
layout (location = 9) in vec4 aInstanceData;  // (island index, seed, radius, 0)

uniform mat4 view;
uniform mat4 projection;

uniform vec4 uIsland[12]; // (position, presence), see islands.cpp

out vec3 vWorldPos;
out vec3 vLocal;   // shaped position in island units (radius 1)
out vec3 vCenter;
out vec3 vTint;
out float vSeed;

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

float fbm(vec3 p)
{
    return 0.55 * noise(p) + 0.3 * noise(p * 2.1 + 3.7) + 0.15 * noise(p * 4.3 + 7.1);
}

void main()
{
    int index = int(aInstanceData.x + 0.5);
    float seed = aInstanceData.y;
    float radius = aInstanceData.z;
    vec4 island = uIsland[index];

    // Flat, slightly bumpy top; craggy underside hanging down like an inverted mountain
    float n = fbm(aPos * 1.8 + seed);
    vec3 q = aPos;
    q.xz *= 1.0 + 0.35 * (n - 0.5);
    if (aPos.y > 0.0)
        q.y = aPos.y * 0.25 + 0.06 * (n - 0.5) * aPos.y;
    else
        q.y = aPos.y * (1.6 + 1.2 * n) - 0.25 * pow(max(-aPos.y, 0.0), 3.0);

    vLocal = q;
    vCenter = island.xyz;
    vWorldPos = island.xyz + q * radius;
    vTint = aInstanceColor.rgb;
    vSeed = seed;
    gl_Position = projection * view * vec4(vWorldPos, 1.0);
}
