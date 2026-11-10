#version 330 core
// Large camera-facing cloud, drifting slowly around the sky

layout (location = 0) in vec3 aPos;   // quad corner in [-1, 1]
layout (location = 4) in mat4 aInstanceModel;
layout (location = 8) in vec4 aInstanceColor;
layout (location = 9) in vec4 aInstanceData;  // (seed, size, emergence order, drift speed)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

uniform float time;
uniform float uIslands;

out vec2 vCorner;
out vec3 vColor;
out float vSeed;
out float vPresence;

void main()
{
    float seed = aInstanceData.x;
    float size = aInstanceData.y;
    float order = aInstanceData.z;
    float drift = aInstanceData.w;

    vec3 center = (model * aInstanceModel * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    center += vec3(sin(time * 0.02 * drift + seed), 0.0, cos(time * 0.017 * drift + seed)) * 8.0;
    center.y += 1.5 * sin(time * 0.05 + seed);

    // Fade out when the camera flies close, rather than filling the view with light
    float near = smoothstep(size * 0.8, size * 2.5, length(center - viewPos));
    vPresence = smoothstep(order * 0.6, order * 0.6 + 0.4, uIslands) * near;

    // Wider than tall, like a bank of cloud
    vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 up = vec3(view[0][1], view[1][1], view[2][1]);
    vec3 world = center + (right * aPos.x * 1.6 + up * aPos.y * 0.7) * size * (0.6 + 0.4 * vPresence);

    vCorner = aPos.xy;
    vColor = aInstanceColor.rgb;
    vSeed = seed;
    gl_Position = projection * view * vec4(world, 1.0);
}
