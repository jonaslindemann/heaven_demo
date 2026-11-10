#version 330 core
// Procedural flower: builds petals, stem and core from parameters.
// Petal surface: P(u,v) = C(u) + w(u) v N(u) + h(u,v) B(u)

layout (location = 0) in vec3 aPos;   // petal (u, v, 0) | stem (u, around, 0) | core (unit sphere)
layout (location = 1) in vec4 aColor; // (petal index, petals in ring, ring, part)
layout (location = 2) in vec2 aTex;    // species style: (openness, curl)
layout (location = 3) in vec3 aNormal; // species style: (petal length, petal width, petal roundness)
layout (location = 4) in mat4 aInstanceModel;
layout (location = 8) in vec4 aInstanceColor; // (tint, nod: how far the head hangs over)
layout (location = 9) in vec4 aInstanceData; // (seed, phase, emergence order, stem height)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float time;     // song time
uniform float uReveal;  // how much of the Act I garden has emerged (timeline)
uniform float uGrowth;  // how far the garden has spread outwards in Act II (timeline)
uniform vec4 uIsland[12]; // floating islands: (position, presence), see islands.cpp
uniform float uFloat;     // seconds of weightlessness so far (Act III), the integral of 1 - gravity
uniform float uDissolve;  // Act IV: how far the world has dissolved, 0..1 (timeline)
uniform vec3 uLastFlower; // where the last flower grows; the dissolving closes in on it
uniform float uLastLight; // Act IV: the last flower shines brighter as the rest dissolves
uniform float uGlow;
uniform float uBass;
uniform float uPads;
uniform float uLead;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vTint;
out vec3 vTipTint;
out vec3 vParam;    // (u, v, part)
out float vVisible;
out float vDissolve; // how far this flower has burned away, 0..1
out float vSeed;
out float vLast;     // 1 for the last flower

const float PI = 3.14159265;
const vec3 UP = vec3(0.0, 1.0, 0.0);

float gSeed, gPhase, gHeight, gOpen, gHeadScale, gVisible;
float gLength, gWidth, gRound, gCurl;
vec3 gNodAxis;
float gNodAngle;

float hash(float n)
{
    return fract(sin(n * 12.9898 + 4.1414) * 43758.5453);
}

// When a flower starts to dissolve: the farthest first, closing in on the last flower.
// (ember.vert uses the same function, so the embers leave the flowers as they burn.)
float dissolveStart(vec3 base, float seed)
{
    float distance = length(base.xz - uLastFlower.xz);
    return 0.72 * (1.0 - clamp(distance / 85.0, 0.0, 1.0)) + 0.06 * hash(seed + 11.0);
}

// Rotate v around a unit axis (Rodrigues)
vec3 rotate(vec3 v, vec3 axis, float angle)
{
    float c = cos(angle), s = sin(angle);
    return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1.0 - c);
}

// Gently swaying stem from the water (y slightly below 0) to the flower head
vec3 stemPoint(float u)
{
    float sway = 0.12 * sin(time * 0.55 + gPhase) + 0.05 * sin(time * 1.3 + gPhase * 2.0);
    float lean = 0.18 * sin(gSeed);
    vec3 bend = vec3(sway + lean, 0.0, 0.6 * sway - lean * 0.5) * (u * u);
    return vec3(0.0, -0.3 + (gHeight + 0.3) * u, 0.0) + bend * gHeight;
}

// Point on a petal: u along the petal, v across it in [-1, 1]
vec3 petalPoint(float u, float v, float phi, float ring, float jitter)
{
    float length = mix(1.0, 0.62, ring) * gLength * (1.0 + 0.16 * (jitter - 0.5));
    float width = mix(0.42, 0.3, ring) * gWidth;
    float open = clamp(gOpen + 0.12 * (jitter - 0.5) * gOpen, 0.0, 1.0);

    // Spine C(u): tilts away from vertical as the flower opens, curling outward towards the tip
    float a = mix(0.1, 1.15, open) - 0.35 * ring * open;
    float b = max(mix(0.25, 1.1, open) * (1.0 - 0.4 * ring) * gCurl, 0.02);
    float r = length * (cos(a) - cos(a + b * u)) / b;
    float y = length * (sin(a + b * u) - sin(a)) / b;

    vec3 radial = vec3(cos(phi), 0.0, sin(phi));
    vec3 side = vec3(-sin(phi), 0.0, cos(phi));

    // Width profile w(u): zero at base and tip. Round petals are widest towards the tip
    // with blunt ends, slender ones widest near the base, tapering to a point.
    // (sin(PI) is slightly negative in float, and pow() of a negative number is NaN)
    float uw = pow(clamp(u, 0.0, 1.0), gRound);
    float blunt = mix(1.1, 0.45, clamp(gRound - 0.7, 0.0, 1.0));
    float w = width * pow(max(sin(PI * uw), 0.0), blunt) * (1.0 - 0.2 * u);

    // Cupping h(u,v) along the spine normal B(u); stronger while closed
    float tilt = a + b * u;
    vec3 normal = radial * cos(tilt) - UP * sin(tilt);
    float cup = 0.35 * width * (1.0 - 0.5 * open) * v * v * sin(PI * u);

    return radial * r + UP * y + side * (w * v) - normal * cup;
}

vec3 flowerPoint(vec3 p, vec4 info)
{
    float part = info.w;
    vec3 head = stemPoint(1.0);

    if (part < 0.5) {
        float ring = info.z;
        float jitter = hash(gSeed + info.x * 7.0 + ring * 31.0);
        float phi = 2.0 * PI * (info.x + 0.5 * ring + 0.25 * (jitter - 0.5)) / info.y + gSeed;
        vec3 petal = petalPoint(p.x, p.y, phi, ring, jitter);
        return head + rotate(petal, gNodAxis, gNodAngle) * gHeadScale;
    }
    if (part < 1.5) {
        float around = p.y * 2.0 * PI;
        float radius = 0.035 * (1.0 - 0.5 * p.x) * mix(0.6, 1.0, gHeadScale);
        return stemPoint(p.x) + vec3(cos(around), 0.0, sin(around)) * radius;
    }
    vec3 core = UP * 0.05 + p * 0.11 * mix(0.8, 1.3, clamp(gWidth - 0.4, 0.0, 1.0));
    return head + rotate(core, gNodAxis, gNodAngle) * gHeadScale;
}

void main()
{
    gSeed = aInstanceData.x;
    gPhase = aInstanceData.y;
    float order = aInstanceData.z;

    // Species style from the mesh, varied per flower
    gLength = aNormal.x * mix(0.85, 1.15, hash(gSeed + 1.0));
    gWidth = aNormal.y * mix(0.8, 1.2, hash(gSeed + 2.0));
    gRound = aNormal.z;
    gCurl = aTex.y * mix(0.75, 1.3, hash(gSeed + 3.0));
    float nodDirection = 2.0 * PI * hash(gSeed + 4.0);
    gNodAxis = vec3(-sin(nodDirection), 0.0, cos(nodDirection));
    gNodAngle = aInstanceColor.a * 2.6 + 0.08 * sin(time * 0.4 + gPhase);

    // Emergence: flowers appear one after another as the garden is revealed,
    // then the garden spreads outwards (orders from 2 follow the growth)
    // and flowers on floating islands (orders from 4) come and go with their island
    vec3 islandOffset = vec3(0.0);
    bool last = order < -0.5;
    if (last)
        gVisible = smoothstep(0.0, 0.25, uReveal);
    else if (order < 1.5)
        gVisible = smoothstep(order * 0.75, order * 0.75 + 0.25, uReveal);
    else if (order < 3.5)
        gVisible = smoothstep((order - 2.0) * 0.85, (order - 2.0) * 0.85 + 0.15, uGrowth);
    else {
        vec4 island = uIsland[clamp(int(order - 4.0 + 0.5), 0, 11)];
        gVisible = smoothstep(0.6, 1.0, island.w);
        islandOffset = island.xyz;
    }
    // Growing taller as the garden comes alive
    gHeight = aInstanceData.w * mix(0.35, 1.0, gVisible) * (1.0 + 0.3 * uGrowth);
    gHeadScale = mix(0.25, 1.0, gVisible);

    // Opening: revealed flowers half open, the lead opens them further, pads make them breathe
    float breath = 0.5 + 0.5 * sin(time * 1.1 + gPhase);
    float leadResponse = 0.6 + 0.4 * sin(gPhase + time * 0.3);
    gOpen = clamp(0.55 * gVisible + 0.35 * uLead * leadResponse + 0.12 * uPads * breath, 0.0, 1.0) * aTex.x;
    // Alone at the end, the last flower opens fully
    if (last)
        gOpen = max(gOpen, mix(gOpen, 0.95, uLastLight * gVisible));

    // Normal from finite differences of the procedural surface
    vec3 local = flowerPoint(aPos, aColor);
    // Hidden flowers wait below the surface and rise out of the water as they emerge
    vec3 rise = vec3(0.0, -(1.0 - gVisible) * (aInstanceData.w + 1.5), 0.0);
    vec3 normal = vec3(0.0, 1.0, 0.0);
    if (aColor.w < 0.5) {
        float du = aPos.x < 0.99 ? 0.01 : -0.01;
        float dv = aPos.y < 0.99 ? 0.02 : -0.02;
        vec3 pu = flowerPoint(aPos + vec3(du, 0.0, 0.0), aColor) - local;
        vec3 pv = flowerPoint(aPos + vec3(0.0, dv, 0.0), aColor) - local;
        // At the petal base the width is zero and the cross product vanishes
        vec3 c = cross(pu, pv) * sign(du * dv);
        normal = dot(c, c) > 1e-14 ? normalize(c) : vec3(0.0, 1.0, 0.0);
    } else if (aColor.w < 1.5) {
        float around = aPos.y * 2.0 * PI;
        normal = vec3(cos(around), 0.0, sin(around));
    } else {
        normal = aPos;
    }

    // Weightless: the garden lifts off the water and drifts upwards, each flower slowly
    // tumbling around its head with the stem trailing below, like a jellyfish.
    // (Island flowers stay on their islands.)
    vec3 lift = vec3(0.0);
    if (uFloat > 0.0 && order < 3.5 && !last) {
        float speed = mix(0.12, 0.55, hash(gSeed + 6.0));
        float height = uFloat * speed;
        float wander = 2.0 * PI * hash(gSeed + 7.0);
        lift = vec3(cos(wander), 0.0, sin(wander)) * 0.15 * height + vec3(0.0, height, 0.0);
        lift += 0.25 * vec3(sin(time * 0.4 + gPhase), 0.0, cos(time * 0.33 + gPhase)) * min(uFloat * 0.2, 1.0);

        float tumbleDirection = 2.0 * PI * hash(gSeed + 8.0);
        vec3 axis = vec3(cos(tumbleDirection), 0.0, sin(tumbleDirection));
        float angle = min(uFloat * mix(0.01, 0.04, hash(gSeed + 9.0)), 0.9) * sin(time * 0.15 + gPhase * 3.0 + 1.0);
        vec3 head = stemPoint(1.0);
        local = head + rotate(local - head, axis, angle);
        normal = rotate(normal, axis, angle);
    }

    mat4 m = model * aInstanceModel;
    vec4 world = m * vec4(local + rise, 1.0);
    world.xyz += islandOffset + lift;

    float start = dissolveStart((m * vec4(0.0, 0.0, 0.0, 1.0)).xyz + islandOffset, gSeed);
    vDissolve = last ? 0.0 : smoothstep(start, start + 0.22, uDissolve);
    vSeed = gSeed;
    vLast = last ? 1.0 : 0.0;
    vWorldPos = world.xyz;
    vNormal = normalize(mat3(m) * normal);
    vTint = aInstanceColor.rgb;
    // Petal tips shift towards a second color: a neighbour in hue, or white
    float pick = hash(gSeed + 5.0);
    vTipTint = pick < 0.35 ? aInstanceColor.gbr * 0.6 + aInstanceColor.rgb * 0.4
             : pick < 0.7  ? aInstanceColor.brg * 0.6 + aInstanceColor.rgb * 0.4
             : pick < 0.85 ? mix(aInstanceColor.rgb, vec3(1.0), 0.6)
                           : aInstanceColor.rgb;
    vParam = vec3(aPos.xy, aColor.w);
    vVisible = gVisible;
    gl_Position = projection * view * world;
}
