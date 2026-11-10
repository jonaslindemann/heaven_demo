#version 330 core
// Soft round glow with a bright core; drawn with additive blending

in vec2 vCorner;
in vec3 vColor;
in float vBrightness;

out vec4 fragColor;

void main()
{
    float r2 = dot(vCorner, vCorner);
    if (r2 > 1.0)
        discard;
    float glow = exp(-r2 * 5.0) + 0.6 * exp(-r2 * 40.0);
    fragColor = vec4(vColor * glow * vBrightness * 1.6, 1.0);
}
