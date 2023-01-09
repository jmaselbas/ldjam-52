#version 410 core

in vec3 in_pos;
in vec2 in_texcoord;
out vec3 dir;
uniform mat4 proj;
uniform mat4 view;

void main(void)
{
	gl_Position = vec4(in_pos.x, in_pos.y, 1, 1.0);
	dir = (transpose(view) * (inverse(proj) * gl_Position)).xyz;
}
