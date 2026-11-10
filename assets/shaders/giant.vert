#version 330 core
// Giant fern stalk: a tube along a planar curve, a tall gently curved stalk that
// ends in a fiddlehead spiral. It grows from the lake and unfurls with uIslands.

layout (location = 0) in vec3 aPos;   // (s along the stalk, angle around it in [0, 1])
layout (location = 4) in mat4 aInstanceModel;
layout (location = 8) in vec4 aInstanceColor;
layout (location = 9) in vec4 aInstanceData;  // (seed, height, curl radius, emergence order)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float time;
uniform float uIslands;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vTint;
out float vAlong;
out float vGrow;
out float vSeed;

const float PI = 3.14159265;
const float STALK = 0.62; // part of the length that is straight stalk; the rest curls

// Spine in the curl plane: x towards the curl, y up
vec3 spine(float s, float height, float curl, float seed)
{
    float stalk = height * 0.78;
    float sway = 0.6 * sin(time * 0.2 + seed);
    if (s <= STALK) {
        float t = s / STALK;
        // An S-bend that ends vertical, so the spiral continues smoothly
        float x = 0.06 * stalk * sin(2.0 * PI * t) * (1.0 - t) + sway * t * t;
        return vec3(x, t * stalk, 0.0);
    }
    // Logarithmic spiral, starting upwards at the top of the stalk and curling inwards
    float a = (s - STALK) / (1.0 - STALK) * 2.6 * PI;
    float r = curl * exp(-0.22 * a);
    vec3 top = vec3(sway, stalk, 0.0);
    vec3 center = top + vec3(curl, 0.0, 0.0);
    return center + vec3(-cos(a) * r, sin(a) * r, 0.0);
}

void main()
{
    float seed = aInstanceData.x;
    float height = aInstanceData.y;
    float curl = aInstanceData.z;
    float order = aInstanceData.w;

    // Grows from the lake, then unfurls; shrinks back the same way
    float grow = smoothstep(order * 0.3, order * 0.3 + 0.3, uIslands);
    float s = min(aPos.x, grow);

    vec3 p = spine(s, height, curl, seed);
    vec3 tangent = spine(min(s + 0.002, 1.0), height, curl, seed) - spine(max(s - 0.002, 0.0), height, curl, seed);
    tangent = dot(tangent, tangent) > 1e-12 ? normalize(tangent) : vec3(0.0, 1.0, 0.0);
    vec3 binormal = vec3(0.0, 0.0, 1.0); // the curve lies in the xy plane
    vec3 normal = normalize(cross(binormal, tangent));

    // Thick at the base, thin at the tip, closed where it stops growing
    float radius = mix(1.3, 0.12, s) * clamp((grow - aPos.x) / 0.02 + 1.0, 0.0, 1.0);
    float around = aPos.y * 2.0 * PI;
    vec3 direction = cos(around) * normal + sin(around) * binormal;
    p += direction * radius;

    mat4 m = model * aInstanceModel;
    vec4 world = m * vec4(p, 1.0);
    vWorldPos = world.xyz;
    vNormal = normalize(mat3(m) * direction);
    vTint = aInstanceColor.rgb;
    vAlong = s;
    vGrow = grow;
    vSeed = seed;
    // Fully submerged until it starts to grow
    gl_Position = projection * view * world;
}
