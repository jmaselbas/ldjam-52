#version 330 core
precision highp float;
precision highp int;

in vec3 normal;
in vec2 texcoord;
in vec3 position;
in vec4 shadowpos;

out vec4 out_color;
uniform vec3 lightd;
uniform vec3 color;

//uniform float fog_density;
//uniform vec3 fog_color;
const float fog_density = 0.005;
const vec3 fog_color = vec3(0.05,0.1,0.1);

#if 0
uniform sampler2DShadow shadowmap;
float shadowlookup(vec4 coord, vec2 uv)
{
	vec2 off = uv / vec2(textureSize(shadowmap, 0));
//	return texture(shadowmap, coord.xyz, + vec3(off, - 0.00001));
	return texture(shadowmap, coord.xyz, 0* 0.01);
}
#else
uniform sampler2D shadowmap;
float shadow(void)
{
	vec3 p = 0.5 + 0.5 * (shadowpos.xyz / shadowpos.w);
	vec2 r = 1./textureSize(shadowmap, 0);
	float d;

	d = texture(shadowmap, p.xy).r;
	return p.z - (d - 0.000001);
}
#endif

void main(void)
{
	const vec3 sky_color = vec3(0.324, 0.0420, 0.0178);
	float inc = dot(normal, lightd);
	vec2 uv = texcoord;
	vec3 col = vec3(uv.yyy) + sky_color * 0.01;
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fog = exp(-fog_density * z * z);

	vec3 p = 0.5 + 0.5 * (shadowpos.xyz / shadowpos.w);
	bool inshadow = shadow() > 0.0|| inc > 0.0;

	col = mix(col, col+vec3(0.2,0.1,0.15)*0.5, 1.0- dot(normal, vec3(0,1,0)));
	if (inshadow)
		col *= 0.0;
	col = mix(fog_color, col, clamp(fog, 0.0, 1.0));
	out_color = vec4(col, 1);
}
