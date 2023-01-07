#version 330 core
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec4 a_pos_xfrm;
layout (location = 2) in vec4 a_shp_xfrm;
layout (location = 3) in vec4 a_img_xfrm;
out vec2 v_shape;
out vec2 v_color;
vec2 xfrm(vec4 x) { return a_pos * x.zw + x.xy; }
void main() {
	gl_Position = vec4(xfrm(a_pos_xfrm), 0.0, 0.5);
	v_shape = xfrm(a_shp_xfrm);
	v_color = xfrm(a_img_xfrm);
}
