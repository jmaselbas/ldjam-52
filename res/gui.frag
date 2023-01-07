#version 330 core
in vec2 v_shape;
in vec2 v_color;
out vec4 out_color;
uniform sampler2D t_shape;
uniform sampler2D t_color;
void main() {
	if (texture(t_shape, v_shape).r < 0.5) discard;
	out_color = vec4(texture(t_color, v_color).rgb, 1.0);
};
