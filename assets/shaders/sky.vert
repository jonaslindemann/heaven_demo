#version 330 core
// Sky dome centered on the camera, so it never comes closer

layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

out vec3 vDir;

void main()
{
    vDir = aPos;
    gl_Position = projection * view * vec4(viewPos + aPos, 1.0);
}
