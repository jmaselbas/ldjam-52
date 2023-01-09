#version 410 core

in vec3 dir;
out vec4 out_color;

const float density = 0.01;

#if 0
#if 0
const vec3 top_color = vec3(0.5*0.624, 0.478, 0.200);
const vec3 mid_color = vec3(0.5*0.247, 0.231, 0.165);
const vec3 bot_color = vec3(0.5*0.075, 0.075, 0.063);
const vec3 sun_color = vec3(0.79, 0.79, 0.78);
#else
const vec3 top_color = vec3(0.624, 0.478, 0.200);
const vec3 mid_color = vec3(0.247, 0.231, 0.165);
const vec3 bot_color = vec3(0.075, 0.075, 0.063);
const vec3 sun_color = vec3(0.79, 0.79, 0.78);
#endif
#endif

const vec3 top_color = vec3(0.324, 0.0420, 0.0178);
const vec3 mid_color = vec3(0.1,0.1,0.1);
const vec3 sun_color = vec3(0.9, 0.99, 0.97);

uniform float time;

#define PI2 (2*3.1416)

/* sun direction */
uniform vec3 lightd;
void main(void)
{
	vec3 sun = -lightd;
	vec3 pos = normalize(dir);
	float mu = smoothstep(0.995, 1, dot(normalize(pos), (sun)));
	float mo = max(0, dot(normalize(pos), (sun)));
	float mi = max(0, -dot(normalize(pos), (sun)));
	float up = dot(pos, vec3(0,1,0));
	vec3 top, col;

	/* color dodge */
	col = top_color/(1.0-sun_color);
	float f = - (mi * mi) + (mo * mo);
	top = top_color * (1 + f);

	/* mix top color with sun color */
	col = mix(top, col, mu);
	/* mix top and bot color */
//	col = mix(bot_color, col, step(0, up));
	/* Add a layer of fog */
	float fog = exp(1.0 / -density * up * up);
	col = mix(col, mid_color, fog);

	out_color = vec4(col,1);
}
