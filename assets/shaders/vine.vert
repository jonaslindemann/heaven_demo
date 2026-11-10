#version 330 core
// Luminous vine hanging from the underside of a floating island, swaying slowly,
// with beads of light running down it (shaded by trail.frag)

layout (location = 0) in vec3 aPos;   // (u along the vine: 0 at the island, side in [-1, 1])
layout (location = 8) in vec4 aInstanceColor; // (tint, island radius)
layout (location = 9) in vec4 aInstanceData;  // (island index, seed, length, ring position under the island)

uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

uniform float time;
uniform float uGlow;
uniform float uStab;
uniform vec4 uIsland[12];

out float vSide;
out float vAlong;
out vec3 vColor;
out float vIntensity;

float hash(float n)
{
    return fract(sin(n * 12.9898 + 78.233) * 43758.5453);
}

vec3 vinePoint(float s, vec3 anchor, float length, float seed)
{
    float sway = 0.5 * sin(time * 0.6 + seed) + 0.25 * sin(time * 1.3 + seed * 2.0);
    float swayZ = 0.4 * sin(time * 0.5 + seed * 1.7);
    return anchor + vec3(sway * s * s, -length * s, swayZ * s * s) * vec3(1.0, 1.0, 1.0);
}

void main()
{
    int index = int(aInstanceData.x + 0.5);
    float seed = aInstanceData.y;
    float length = aInstanceData.z;
    float ring = aInstanceData.w;
    float radius = aInstanceColor.a;
    vec4 island = uIsland[index];

    // Anchor on the underside, which tapers down towards the middle
    float angle = 6.2831853 * hash(seed);
    vec3 anchor = island.xyz + radius * vec3(cos(angle) * ring, -1.4 * sqrt(max(1.0 - ring * ring, 0.0)), sin(angle) * ring);

    float s = aPos.x;
    vec3 p = vinePoint(s, anchor, length, seed);
    vec3 tangent = vinePoint(s + 0.01, anchor, length, seed) - vinePoint(max(s - 0.01, 0.0), anchor, length, seed);
    tangent = dot(tangent, tangent) > 1e-10 ? normalize(tangent) : vec3(0.0, -1.0, 0.0);
    vec3 side = cross(tangent, normalize(viewPos - p));
    side = dot(side, side) > 1e-8 ? normalize(side) : vec3(1.0, 0.0, 0.0);
    p += side * aPos.y * 0.05 * (1.0 - 0.7 * s);

    // Beads of light running down, quickening with the stabs
    float bead = pow(max(0.5 + 0.5 * sin(s * 18.0 - time * 2.5 + seed), 0.0), 8.0);
    vSide = aPos.y;
    vAlong = 0.0;
    vColor = aInstanceColor.rgb;
    vIntensity = smoothstep(0.6, 1.0, island.w) * (1.0 - s) * (0.35 + bead * (1.0 + 2.0 * uStab)) * (0.6 + 0.5 * uGlow);
    gl_Position = projection * view * vec4(p, 1.0);
}
