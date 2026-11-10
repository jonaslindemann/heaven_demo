#version 330 core
// A bright core with a soft glow across the ribbon; drawn with additive blending

in float vSide;
in float vAlong;
in vec3 vColor;
in float vIntensity;

out vec4 fragColor;

void main()
{
    float across = exp(-vSide * vSide * 3.0) + 1.5 * exp(-vSide * vSide * 30.0);
    // The head of the trail is a little hotter, close to white
    vec3 color = mix(vColor, vec3(1.0), 0.5 * vAlong * vAlong);
    fragColor = vec4(color * across * vIntensity * 1.8, 1.0);
}
