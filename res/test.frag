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
float shadowlookup(vec4 coord, vec2 uv)
{
	vec2 off = uv / vec2(textureSize(shadowmap, 0));
	float d = coord.z + 0.00001;
	float s = texture(shadowmap, coord.xy + off).z;
	return (s <= d) ? 0.0: 1.0;
}
#endif
float shadow(void)
{
	float shadow = 0.0;
	return shadowlookup(shadowpos, vec2(0));
}
void main(void)
{
	float inc = dot(normal, lightd);
	vec2 uv = texcoord;
	vec3 col = vec3(uv.yyy);//texture(tex, uv).rgb;
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fog = exp(-fog_density * z * z);

	vec3 p = 0.5 + 0.5 * (shadowpos.xyz / shadowpos.w);
	float dep = texture(shadowmap, p.xy).r;
	bool inshadow = p.z > (dep + 0.000005) || inc > 0.0;
	col = mix(col, col+vec3(0.2,0.1,0.15)*0.5, 1.0- dot(normal, vec3(0,1,0)));
	if (inshadow)
		col *= 0.0;

	col = mix(fog_color, col, clamp(fog, 0.0, 1.0));

	out_color = vec4(col, 1);
}

