#version 330 core
precision highp float;
precision highp int;

in vec3 normal;
in vec2 texcoord;
in vec3 position;
out vec4 out_color;

uniform vec3 color;

//uniform float fog_density;
//uniform vec3 fog_color;
const float fog_density = 0.001;
const vec3 fog_color = vec3(0.1,0.1,0.1);
void main(void)
{
	vec2 uv = texcoord;
	vec3 col = vec3(uv, 0.0);//texture(tex, uv).rgb;
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fog = exp(-fog_density * z * z);

	col = mix(fog_color, col, clamp(fog, 0.0, 1.0));
	col = mix(col, fog_color, clamp(-0.5*(position.y), 0.0, 1.0));

	out_color = vec4(col, 1);
}

