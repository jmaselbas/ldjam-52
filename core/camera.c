#include <string.h>
#include "engine.h"

static void camera_update_view(struct camera *c);
static void camera_update_proj(struct camera *c);

void
camera_init(struct camera* c, float fov, float ratio)
{
	c->position = VEC3_ZERO;
	c->rotation = QUATERNION_IDENTITY;
	c->view = MAT4_IDENTITY;
	c->proj = MAT4_IDENTITY;

	c->fov = fov;
	c->ratio = ratio;
	c->zNear = 0.4;
	c->zFar = 2000;

	camera_update_view(c);
	camera_update_proj(c);
}

void
camera_set(struct camera *c, vec3 position, quaternion rotation)
{
	c->position = position;
	c->rotation = rotation;
	camera_update_view(c);
}

void
camera_set_znear(struct camera *c, float z)
{
	c->zNear = z;
	camera_update_proj(c);
}

void
camera_set_position(struct camera *c, vec3 position)
{
	camera_set(c, position, c->rotation);
}

void
camera_set_rotation(struct camera *c, quaternion rotation)
{
	camera_set(c, c->position, rotation);
}

void
camera_set_projection(struct camera *c, float fov, float ratio)
{
	c->fov = fov;
	c->ratio = ratio;
	camera_update_proj(c);
}

void
camera_set_ratio(struct camera *c, float ratio)
{
	camera_set_projection(c, c->fov, ratio);
}

vec3
camera_get_left(struct camera *c)
{
#if 0
	vec3 s;
	mat3 rot;
	quaternion_to_rot3(&rot, c->rotation);
	s.x = rot.m[0][0];
	s.y = rot.m[1][0];
	s.z = rot.m[2][0];
	return s;
#else
	mat3 rot;
	quaternion_to_rot3(&rot, c->rotation);
	return mat3_col(&rot, 0);
#endif
}

vec3
camera_get_up(struct camera *c)
{
#if 0
	vec3 s;
	mat3 rot;
	quaternion_to_rot3(&rot, c->rotation);
	s.x = rot.m[0][1];
	s.y = rot.m[1][1];
	s.z = rot.m[2][1];
	return s;
#else
	mat3 rot;
	quaternion_to_rot3(&rot, c->rotation);
	return mat3_col(&rot, 1);
#endif
}

vec3
camera_get_dir(struct camera *c)
{
#if 0
	vec3 s;
	mat3 rot;
	quaternion_to_rot3(&rot, c->rotation);
	s.x = rot.m[0][2];
	s.y = rot.m[1][2];
	s.z = rot.m[2][2];
	return s;
#else
	mat3 rot;
	quaternion_to_rot3(&rot, c->rotation);
	return mat3_col(&rot, 2);
#endif
}

void
camera_move(struct camera *c, vec3 off)
{
	camera_set_position(c, vec3_add(c->position, off));
}

void
camera_apply(struct camera *c, quaternion q)
{
	c->rotation = quaternion_mult(q, c->rotation);
	camera_update_view(c);
}

void
camera_rotate(struct camera *c, vec3 axis, float angle)
{
	quaternion q = quaternion_axis_angle(axis, angle);
	camera_apply(c, q);
}

void
camera_look_at(struct camera *c, vec3 look_at, vec3 up)
{
	vec3 dir = vec3_normalize(vec3_sub(look_at, c->position));

	c->rotation = quaternion_look_at(dir, up);
	camera_update_view(c);
}

static void
camera_update_view(struct camera *c)
{
	float x, y, z;
	vec3 s, u, f;

	u = camera_get_up(c);
	f = camera_get_dir(c);

	f = vec3_normalize(f);
	u = vec3_normalize(u);
	s = vec3_cross(f, u);
	s = vec3_normalize(s);

	/* changement de repère worldview to eyeview */
	x = -vec3_dot(s, c->position);
	y = -vec3_dot(u, c->position);
	z = -vec3_dot(f, c->position);
	vec4 r0 = {  s.x,  u.x, -f.x, 0 };
	vec4 r1 = {  s.y,  u.y, -f.y, 0 };
	vec4 r2 = {  s.z,  u.z, -f.z, 0 };
	vec4 r3 = {    x,    y,   -z, 1 };
	/* transpose the matrix for the inverse rotation */
	c->view = mat4_from_cols(r0, r1, r2, r3);
}

// https://github.com/g-truc/glm/blob/master/manual.md#section5_2
// https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml
// https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

static void
camera_update_proj(struct camera *cam)
{
	float t = tan(cam->fov * 0.5);
	float r = cam->ratio * t;
	float f = cam->zFar;
	float n = cam->zNear;

	float a = 1.0 / r;
	float b = 1.0 / t;
	float c = -(2.0 * f * n) / (f - n);
	float d = -1.0;
	float z = -(f + n) / (f - n);

	vec4 v0 = {a, 0, 0, 0};
	vec4 v1 = {0, b, 0, 0};
	vec4 v2 = {0, 0, z, c};
	vec4 v3 = {0, 0, d, 0};
	cam->proj = mat4_from_rows(v0, v1, v2, v3);
}

void
camera_update_orth(struct camera *cam, int width, int height)
{
	float f = 800.0;//c->zFar;
	float n = 0.0; //c->zNear;

	float a = 2.0 / width;
	float b = 2.0 / height;
	float c = -2.0 / (f - n);
	float z = -(f + n) / (f - n);

	vec4 v0 = {a, 0, 0, 0};
	vec4 v1 = {0, b, 0, 0};
	vec4 v2 = {0, 0, c, z};
	vec4 v3 = {0, 0, 0, 1};
	cam->proj = mat4_from_rows(v0, v1, v2, v3);
}
