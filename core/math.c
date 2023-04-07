#include <stdio.h>
#include <string.h>
#include "util.h"
#include "math.h"

#define VAL "%8.2f"
#define SEP ", "

float mix(float a, float b, float x);

vec3 vec3_neg(vec3 v);
vec3 vec3_add(vec3 u, vec3 v);
vec3 vec3_sub(vec3 u, vec3 v);
vec3 vec3_mult(float s, vec3 v);
float vec3_dot(vec3 u, vec3 v);
float vec3_norm(vec3 v);
vec3 vec3_normalize(vec3 v);
float vec3_max(vec3 v);
vec3 vec3_lerp(vec3 u, vec3 v, float x);
vec3 vec3_fma(vec3 u, vec3 v, vec3 w);
vec3 vec3_cross(vec3 u, vec3 v);
float vec3_dist(vec3 u, vec3 v);

void print_vec3(vec3 v)
{
	printf("{" VAL SEP VAL SEP VAL "}\n", v.x, v.y, v.z);
}

void print_vec4(vec4 v)
{
	printf("{" VAL SEP VAL SEP VAL SEP VAL "}\n", v.x, v.y, v.z, v.w);
}

vec4 vec4_from_float(float x, float y, float z, float w) {
	vec4 v = { x, y, z, w };
	return v;
}

vec4 vec4_from_vec3(vec3 v) {
	return vec4_from_float(v.x, v.y, v.z, 0.0);
}

vec4 vec4_neg(vec4 v);
vec4 vec4_add(vec4 u, vec4 v);
vec4 vec4_sub(vec4 u, vec4 v);
vec4 vec4_mult(float s, vec4 v);
float vec4_dot(vec4 u, vec4 v);
float vec4_norm(vec4 v);
vec4 vec4_normalize(vec4 v);

void print_mat3(mat3 m)
{
	printf(
		"{" VAL SEP VAL SEP VAL "\n"
		" " VAL SEP VAL SEP VAL "\n"
		" " VAL SEP VAL SEP VAL "}\n",
		m.m[0][0], m.m[1][0], m.m[2][0],
		m.m[0][1], m.m[1][1], m.m[2][1],
		m.m[0][2], m.m[1][2], m.m[2][2]
		);
}

mat3 mat3_id(void)
{
	return MAT3_IDENTITY;
}

vec3
mat3_col(const mat3 *m, const size_t i)
{
	vec3 v = { m->m[0][i], m->m[1][i], m->m[2][i] };
	return v;
}

vec3
mat3_row(const mat3 *m, const size_t i)
{
	vec3 v = { m->m[i][0], m->m[i][1], m->m[i][2] };
	return v;
}

mat3
mat3_from_rows(vec3 r0, vec3 r1, vec3 r2)
{
	mat3 m = {{{ r0.x, r0.y, r0.z },
		   { r1.x, r1.y, r1.z },
		   { r2.x, r2.y, r2.z },
		}};
	return m;
}

mat3
mat3_from_cols(vec3 c0, vec3 c1, vec3 c2)
{
	mat3 m = {{{ c0.x, c1.x, c2.x },
		   { c0.y, c1.y, c2.y },
		   { c0.z, c1.z, c2.z },
		}};
	return m;
}

mat3 mat3_mult(mat3 *m, float s)
{
	mat3 r;

	r.m[0][0] = m->m[0][0] * s;
	r.m[0][1] = m->m[0][1] * s;
	r.m[0][2] = m->m[0][2] * s;
	r.m[1][0] = m->m[1][0] * s;
	r.m[1][1] = m->m[1][1] * s;
	r.m[1][2] = m->m[1][2] * s;
	r.m[2][0] = m->m[2][0] * s;
	r.m[2][1] = m->m[2][1] * s;
	r.m[2][2] = m->m[2][2] * s;

	return r;
}

vec3 mat3_mult_vec3(mat3 *m, vec3 v)
{
	vec3 r;

	r.x = m->m[0][0] * v.x + m->m[1][0] * v.y + m->m[2][0] * v.z;
	r.y = m->m[0][1] * v.x + m->m[1][1] * v.y + m->m[2][1] * v.z;
	r.z = m->m[0][2] * v.x + m->m[1][2] * v.y + m->m[2][2] * v.z;

	return r;
}

mat3 mat3_mult_mat3(mat3 *a, mat3 *b)
{
	vec3 r0 = {
		vec3_dot(mat3_row(a, 0), mat3_col(b, 0)),
		vec3_dot(mat3_row(a, 0), mat3_col(b, 1)),
		vec3_dot(mat3_row(a, 0), mat3_col(b, 2)),
	};
	vec3 r1 = {
		vec3_dot(mat3_row(a, 1), mat3_col(b, 0)),
		vec3_dot(mat3_row(a, 1), mat3_col(b, 1)),
		vec3_dot(mat3_row(a, 1), mat3_col(b, 2)),
	};
	vec3 r2 = {
		vec3_dot(mat3_row(a, 2), mat3_col(b, 0)),
		vec3_dot(mat3_row(a, 2), mat3_col(b, 1)),
		vec3_dot(mat3_row(a, 2), mat3_col(b, 2)),
	};
	return mat3_from_rows(r0, r1, r2);
}

float mat3_det(mat3 *m)
{
	float d;

	d  = m->m[0][0] * m->m[1][1] * m->m[2][2];
	d += m->m[0][1] * m->m[1][2] * m->m[2][0];
	d += m->m[1][0] * m->m[2][1] * m->m[0][2];
	d -= m->m[1][1] * m->m[0][2] * m->m[2][0];
	d -= m->m[0][0] * m->m[1][2] * m->m[2][1];
	d -= m->m[0][1] * m->m[1][0] * m->m[2][2];

	return d;
}

mat3 mat3_look_at(vec3 dir, vec3 up)
{
	vec3 s, u, f;

	/*
	  We want to rotate the camera from it's initial orientation
	  determined by the orthonormal familly of vectors
	  B = (s,u,f) = ({1,0,0}, {0,1,0}, {0,0,1}) to a new
	  a orientation (s',u',f') such that:
	  - f' = normalize(dir)
	  - u' must be in the plane (up,dir) and orthogonal to f'.
	  - s' must be orthogonal to (up,dir).
	*/

	/*
	  Compute the new orthonormal family of vectors
	  B' = (s',u',f')
	*/
#if 0
	f = vec3_normalize(f);
	u = vec3_normalize(u);
	s = vec3_cross(f, u);
	s = vec3_normalize(s);
#else
	f = vec3_normalize(dir);
	s = vec3_cross(up, f);
	s = vec3_normalize(s);
	u = vec3_cross(f, s);
	u = vec3_normalize(u);
#endif

	/*
	  The transition matrix from B to B' is simply the matrix
	  with column vectors ({s'},{u'},{f'}).
	*/
	return mat3_from_cols(s, u, f);
}

#if 0
mat3 invert3m(mat3 *m)
{
	float det = mat3_det(m);

	if (det != 0) {
		float idet = 1.0 / det;

		d->m[0][0] = idet * (s.m[1][1] * s.m[2][2] - s.m[2][1] * s.m[1][2]);
		d->m[0][1] = idet * (s.m[2][1] * s.m[0][2] - s.m[0][1] * s.m[2][2]);
		d->m[0][2] = idet * (s.m[0][1] * s.m[1][2] - s.m[1][1] * s.m[0][2]);

		d->m[1][0] = idet * (s.m[2][0] * s.m[1][2] - s.m[1][0] * s.m[2][2]);
		d->m[1][1] = idet * (s.m[0][0] * s.m[2][2] - s.m[2][0] * s.m[0][2]);
		d->m[1][2] = idet * (s.m[1][0] * s.m[0][2] - s.m[0][0] * s.m[1][2]);

		d->m[2][0] = idet * (s.m[1][0] * s.m[2][1] - s.m[1][1] * s.m[2][0]);
		d->m[2][1] = idet * (s.m[2][0] * s.m[0][1] - s.m[0][0] * s.m[2][1]);
		d->m[2][2] = idet * (s.m[0][0] * s.m[1][1] - s.m[1][0] * s.m[0][1]);

		return 1;
	}

	return 0;
}
#endif

void print_mat4(const mat4 *m)
{
	printf(
		"{"  VAL SEP VAL SEP VAL SEP VAL "\n"
		" "  VAL SEP VAL SEP VAL SEP VAL "\n"
		" "  VAL SEP VAL SEP VAL SEP VAL "\n"
		" " VAL SEP VAL SEP VAL SEP VAL "}\n",
		m->m[0][0], m->m[1][0], m->m[2][0], m->m[3][0],
		m->m[0][1], m->m[1][1], m->m[2][1], m->m[3][1],
		m->m[0][2], m->m[1][2], m->m[2][2], m->m[3][2],
		m->m[0][3], m->m[1][3], m->m[2][3], m->m[3][3]
		);
}

mat4 mat4_id(void)
{
	return MAT4_IDENTITY;
}

static void mat4_mult_local(mat4 *m, float s)
{
	m->m[0][0] *= s;
	m->m[0][1] *= s;
	m->m[0][2] *= s;
	m->m[0][3] *= s;
	m->m[1][0] *= s;
	m->m[1][1] *= s;
	m->m[1][2] *= s;
	m->m[1][3] *= s;
	m->m[2][0] *= s;
	m->m[2][1] *= s;
	m->m[2][2] *= s;
	m->m[2][3] *= s;
	m->m[3][0] *= s;
	m->m[3][1] *= s;
	m->m[3][2] *= s;
	m->m[3][3] *= s;
}

mat4 mat4_mult(const mat4 *m, float s)
{
	mat4 r = *m;

	mat4_mult_local(&r, s);

	return r;
}

//#define MAT4_ROW_MAJOR
#ifdef MAT4_ROW_MAJOR
vec4
mat4_col(const mat4 *m, const size_t i)
{
	vec4 v = { m->m[0][i], m->m[1][i], m->m[2][i], m->m[3][i] };
	return v;
}

vec4
mat4_row(const mat4 *m, const size_t i)
{
	vec4 v = { m->m[i][0], m->m[i][1], m->m[i][2], m->m[i][3] };
	return v;
}

mat4
mat4_from_rows(vec4 r0, vec4 r1, vec4 r2, vec4 r3)
{
	mat4 m = {{{ r0.x, r0.y, r0.z, r0.w },
		   { r1.x, r1.y, r1.z, r1.w },
		   { r2.x, r2.y, r2.z, r2.w },
		   { r3.x, r3.y, r3.z, r3.w },
		}};
	return m;
}

mat4
mat4_from_cols(vec4 c0, vec4 c1, vec4 c2, vec4 c3)
{
	mat4 m = {{{ c0.x, c1.x, c2.x, c3.x },
		   { c0.y, c1.y, c2.y, c3.y },
		   { c0.z, c1.z, c2.z, c3.z },
		   { c0.w, c1.w, c2.w, c3.w },
		}};
	return m;
}
#else
vec4
mat4_col(const mat4 *m, const size_t i)
{
	vec4 v = { m->m[i][0], m->m[i][1], m->m[i][2], m->m[i][3] };
	return v;
}

vec4
mat4_row(const mat4 *m, const size_t i)
{
	vec4 v = { m->m[0][i], m->m[1][i], m->m[2][i], m->m[3][i] };
	return v;
}

mat4
mat4_from_rows(vec4 v0, vec4 v1, vec4 v2, vec4 v3)
{
	mat4 m = {{{ v0.x, v1.x, v2.x, v3.x },
		   { v0.y, v1.y, v2.y, v3.y },
		   { v0.z, v1.z, v2.z, v3.z },
		   { v0.w, v1.w, v2.w, v3.w },
		}};
	return m;
}

mat4
mat4_from_cols(vec4 v0, vec4 v1, vec4 v2, vec4 v3)
{
	mat4 m = {{{ v0.x, v0.y, v0.z, v0.w },
		   { v1.x, v1.y, v1.z, v1.w },
		   { v2.x, v2.y, v2.z, v2.w },
		   { v3.x, v3.y, v3.z, v3.w },
		}};
	return m;
}
#endif

mat4
mat4_transpose(mat4 m)
{
	vec4 r0 = mat4_row(&m, 0);
	vec4 r1 = mat4_row(&m, 1);
	vec4 r2 = mat4_row(&m, 2);
	vec4 r3 = mat4_row(&m, 3);

	return mat4_from_cols(r0, r1, r2, r3);
}

static mat4
mat4_from_mat3(mat3 m)
{
	vec4 r0 = vec4_from_vec3(mat3_row(&m, 0));
	vec4 r1 = vec4_from_vec3(mat3_row(&m, 1));
	vec4 r2 = vec4_from_vec3(mat3_row(&m, 2));
	vec4 r3 = {0, 0, 0, 1};
	return mat4_from_rows(r0, r1, r2, r3);
}

static mat3
mat3_from_quaternion(quaternion q)
{
	float n = quaternion_sqnorm(q);
	float s = (n > 0.0) ? 2.0 / n : 0;
	float x = q.v.x;
	float y = q.v.y;
	float z = q.v.z;
	float w = q.w;
	float xs = x * s,  ys = y * s,  zs = z * s;
	float wx = w * xs, wy = w * ys, wz = w * zs;
	float xx = x * xs, xy = x * ys, xz = x * zs;
	float yy = y * ys, yz = y * zs, zz = z * zs;
	vec3 r0 = { 1.0 - (yy + zz), xy - wz, xz + wy };
	vec3 r1 = { xy + wz, 1.0 - (xx + zz), yz - wx };
	vec3 r2 = { xz - wy, yz + wx, 1.0 - (xx + yy) };
	return mat3_from_rows(r0, r1, r2);
}

static mat4
mat4_from_quaternion(quaternion q)
{
	return mat4_from_mat3(mat3_from_quaternion(q));
}

vec4 mat4_mult_vec4(const mat4 *m, vec4 v)
{
	vec4 r;
	r.x = vec4_dot(mat4_row(m, 0), v);
	r.y = vec4_dot(mat4_row(m, 1), v);
	r.z = vec4_dot(mat4_row(m, 2), v);
	r.w = vec4_dot(mat4_row(m, 3), v);
	return r;
}

vec3 mat4_mult_vec3(const mat4 *m, vec3 v)
{
	vec3 r;

	r.x = m->m[0][0] * v.x + m->m[0][1] * v.y + m->m[0][2] * v.z + m->m[3][0];
	r.y = m->m[1][0] * v.x + m->m[1][1] * v.y + m->m[1][2] * v.z + m->m[3][1];
	r.z = m->m[2][0] * v.x + m->m[2][1] * v.y + m->m[2][2] * v.z + m->m[3][2];

	return r;
}

mat4 mat4_mult_mat4(const mat4 *a, const mat4 *b)
{
	vec4 r0 = {
		vec4_dot(mat4_row(a, 0), mat4_col(b, 0)),
		vec4_dot(mat4_row(a, 0), mat4_col(b, 1)),
		vec4_dot(mat4_row(a, 0), mat4_col(b, 2)),
		vec4_dot(mat4_row(a, 0), mat4_col(b, 3)),
	};
	vec4 r1 = {
		vec4_dot(mat4_row(a, 1), mat4_col(b, 0)),
		vec4_dot(mat4_row(a, 1), mat4_col(b, 1)),
		vec4_dot(mat4_row(a, 1), mat4_col(b, 2)),
		vec4_dot(mat4_row(a, 1), mat4_col(b, 3)),
	};
	vec4 r2 = {
		vec4_dot(mat4_row(a, 2), mat4_col(b, 0)),
		vec4_dot(mat4_row(a, 2), mat4_col(b, 1)),
		vec4_dot(mat4_row(a, 2), mat4_col(b, 2)),
		vec4_dot(mat4_row(a, 2), mat4_col(b, 3)),
	};
	vec4 r3 = {
		vec4_dot(mat4_row(a, 3), mat4_col(b, 0)),
		vec4_dot(mat4_row(a, 3), mat4_col(b, 1)),
		vec4_dot(mat4_row(a, 3), mat4_col(b, 2)),
		vec4_dot(mat4_row(a, 3), mat4_col(b, 3)),
	};
	return mat4_from_rows(r0, r1, r2, r3);
}

static vec4
plane_normalize(const vec4 *plane)
{
	vec4 r;
	vec3 normal = { plane->x, plane->y, plane->z };
	float mag = vec3_dot(normal, normal);

	r.x = plane->x / mag;
	r.y = plane->y / mag;
	r.z = plane->z / mag;
	r.w = plane->w / mag;

	return r;
}

void
mat4_projection_frustum(const mat4 *m, vec4 planes[6])
{
	vec4 v0 = mat4_row(m, 0);
	vec4 v1 = mat4_row(m, 1);
	vec4 v2 = mat4_row(m, 2);
	vec4 v3 = mat4_row(m, 3);

	/* Gribb and Hartmann method: Fast Extraction of Viewing
	 * Frustum Planes from the World-View-Projection Matrix
	 * www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
	 */

	/* Left clipping plane */
	planes[0] = vec4_add(v3, v0);
	/* Right clipping plane */
	planes[1] = vec4_sub(v3, v0);
	/* Bottom clipping plane */
	planes[2] = vec4_add(v3, v1);
	/* Top clipping plane */
	planes[3] = vec4_sub(v3, v1);
	/* Near clipping plane */
	planes[4] = vec4_add(v3, v2);
	/* Far clipping plane */
	planes[5] = vec4_sub(v3, v2);

	/* normalize planes */
	planes[0] = plane_normalize(&planes[0]);
	planes[1] = plane_normalize(&planes[1]);
	planes[2] = plane_normalize(&planes[2]);
	planes[3] = plane_normalize(&planes[3]);
	planes[4] = plane_normalize(&planes[4]);
	planes[5] = plane_normalize(&planes[5]);
}

float
plane_signed_distance(vec4 plane, vec3 point)
{
	vec3 p = { plane.x, plane.y, plane.z };
	return vec3_dot(p, point) + plane.w;
}

int
sphere_outside_frustum(const vec4 planes[6], vec3 center, float radius)
{
	int i;

	for (i = 0; i < 6; i++) {
		if (plane_signed_distance(planes[i], center) + radius < 0)
			return 1;
	}

	return 0;
}

#if 0
void load_rot4(mat4 *d, vec3 axis, float angle) {
	float s = sin(angle), c = cos(angle), v = 1 - c;
	float xv, xs, yv, ys, zv, zs;
	vec3 a = vec3_normalize(axis);

	if (angle == 0) {
		*d = MAT4_IDENTITY;
		return;
	}

	xv = a.x * v;
	xs = a.x * s;
	yv = a.y * v;
	ys = a.y * s;
	zv = a.z * v;
	zs = a.z * s;

	d->m[0][0] = a.x * xv + c;
	d->m[0][1] = a.x * yv + zs;
	d->m[0][2] = a.x * zv - ys;
	d->m[0][3] = 0.0;

	d->m[1][0] = a.y * xv - zs;
	d->m[1][1] = a.y * yv + c;
	d->m[1][2] = a.y * zv + xs;
	d->m[1][3] = 0.0;

	d->m[2][0] = a.z * xv + ys;
	d->m[2][1] = a.z * yv - xs;
	d->m[2][2] = a.z * zv + c;
	d->m[2][3] = 0.0;

	d->m[3][0] = 0.0;
	d->m[3][1] = 0.0;
	d->m[3][2] = 0.0;
	d->m[3][3] = 1.0;
}
#endif

mat4 mat4_transform(vec3 pos, quaternion rot)
{
	mat4 r = mat4_from_quaternion(rot);

	vec4 c0 = mat4_col(&r, 0);
	vec4 c1 = mat4_col(&r, 1);
	vec4 c2 = mat4_col(&r, 2);
	vec4 c3 = { pos.x, pos.y, pos.z, 1.0 };

	return mat4_from_cols(c0, c1, c2, c3);
}

mat4 mat4_transform_scale(vec3 pos, quaternion rot, vec3 scale)
{
	mat4 r = mat4_from_quaternion(rot);
	vec4 c0 = vec4_mult(scale.x, mat4_col(&r, 0));
	vec4 c1 = vec4_mult(scale.y, mat4_col(&r, 1));
	vec4 c2 = vec4_mult(scale.z, mat4_col(&r, 2));
	vec4 c3 = { pos.x, pos.y, pos.z, 1.0 };

	return mat4_from_cols(c0, c1, c2, c3);
}

quaternion quaternion_look_at(vec3 dir, vec3 up)
{
	mat3 rot = mat3_look_at(dir, up);

	/*
	  TODO
	  This matrix tranform an orthonormal familly of
	  vectors into another orthonormal familly.
	  The transformation matrix here represent an isometry.
	  We should be able to calculate the corresponding quaternion easily.
	  quaternion_from_rot3 seems to be overkill for this AND
	  we do not have a rotation here.
	  If quaternion_from_rot3 work for any orthogonal matrix
	  rename this quaternion_from_mat3 and add ortho precondition
	  in the spec.
	*/
	return quaternion_from_rot3(&rot);
}

quaternion quaternion_axis_angle(vec3 axis, float angle)
{
	quaternion r;
	float s = sin(angle * 0.5);
	float w = cos(angle * 0.5);

	if (axis.x == 0 && axis.y == 0 && axis.z == 0) {
		return QUATERNION_IDENTITY;
	}

	r.v = vec3_mult(s, axis);
	r.w = w;
	return r;
}

quaternion quaternion_mult(quaternion a, quaternion b)
{
	quaternion r;

	r.v = vec3_cross(a.v, b.v);
	r.v.x += a.w * b.v.x + b.w * a.v.x;
	r.v.y += a.w * b.v.y + b.w * a.v.y;
	r.v.z += a.w * b.v.z + b.w * a.v.z;
	r.w = a.w * b.w - vec3_dot(a.v, b.v);

	return r;
}

float quaternion_sqnorm(quaternion q)
{
	return vec3_dot(q.v, q.v) + q.w * q.w;
}

float quaternion_norm(quaternion q)
{
	return sqrt(quaternion_sqnorm(q));
}

quaternion quaternion_normalize(quaternion q)
{
	float n = quaternion_norm(q);
	quaternion r = { { q.v.x / n, q.v.y / n, q.v.z / n }, q.w / n };
	return r;
}

quaternion quaternion_conjugate(quaternion q)
{
	quaternion r = { vec3_neg(q.v), q.w };
	return r;
}

vec3 quaternion_rotate(quaternion q, vec3 v)
{
	quaternion r = { v, 0 };
	quaternion c = quaternion_conjugate(q);

	/* r = q * r * conjugate(q); */
	r = quaternion_mult(r, c);
	r = quaternion_mult(q, r);

	return r.v;
}

void quaternion_to_mat4(mat4 *d, quaternion q)
{
#if 1
	*d = mat4_from_quaternion(q);
#else
	mat3 m = mat3_from_quaternion(q);
//	quaternion_to_rot3(&m, q);
	d->m[0][0] = m.m[0][0];
	d->m[0][1] = m.m[1][0];
	d->m[0][2] = m.m[2][0];
	d->m[0][3] = 0;
	d->m[1][0] = m.m[0][1];
	d->m[1][1] = m.m[1][1];
	d->m[1][2] = m.m[2][1];
	d->m[1][3] = 0;
	d->m[2][0] = m.m[0][2];
	d->m[2][1] = m.m[1][2];
	d->m[2][2] = m.m[2][2];
	d->m[2][3] = 0;
	d->m[3][3] = 1;
#endif
}

void quaternion_to_rot3(mat3 *d, quaternion q) {
	*d = mat3_from_quaternion(q);
}

quaternion quaternion_from_rot3(mat3 *m)
{
	quaternion r;
	float m00 = m->m[0][0];
	float m01 = m->m[0][1];
	float m02 = m->m[0][2];
	float m10 = m->m[1][0];
	float m11 = m->m[1][1];
	float m12 = m->m[1][2];
	float m20 = m->m[2][0];
	float m21 = m->m[2][1];
	float m22 = m->m[2][2];
        float lengthSquared = m00 * m00 + m10 * m10 + m20 * m20;
	float s, t;
	float x, y, z, w;

	if (lengthSquared != 1 && lengthSquared != 0) {
		lengthSquared = 1.0 / sqrt(lengthSquared);
		m00 *= lengthSquared;
		m10 *= lengthSquared;
		m20 *= lengthSquared;
	}
	lengthSquared = m01 * m01 + m11 * m11 + m21 * m21;
	if (lengthSquared != 1 && lengthSquared != 0) {
		lengthSquared = 1.0 / sqrt(lengthSquared);
		m01 *= lengthSquared;
		m11 *= lengthSquared;
		m21 *= lengthSquared;
	}
	lengthSquared = m02 * m02 + m12 * m12 + m22 * m22;
	if (lengthSquared != 1 && lengthSquared != 0) {
		lengthSquared = 1.0 / sqrt(lengthSquared);
		m02 *= lengthSquared;
		m12 *= lengthSquared;
		m22 *= lengthSquared;
	}

        t = m00 + m11 + m22;

        if (t >= 0) { // |w| >= .5
		s = sqrt(t + 1); // |s|>=1 ...
		w = 0.5 * s;
		s = 0.5 / s;                 // so this division isn't bad
		x = (m21 - m12) * s;
		y = (m02 - m20) * s;
		z = (m10 - m01) * s;
	} else if ((m00 > m11) && (m00 > m22)) {
		s = sqrt(1.0 + m00 - m11 - m22); // |s|>=1
		x = 0.5 * s; // |x| >= .5
		s = 0.5 / s;
		y = (m10 + m01) * s;
		z = (m02 + m20) * s;
		w = (m21 - m12) * s;
	} else if (m11 > m22) {
		s = sqrt(1.0 + m11 - m00 - m22); // |s|>=1
		y = 0.5 * s; // |y| >= .5
		s = 0.5 / s;
		x = (m10 + m01) * s;
		z = (m21 + m12) * s;
		w = (m02 - m20) * s;
	} else {
		s = sqrt(1.0 + m22 - m00 - m11); // |s|>=1
		z = 0.5 * s; // |z| >= .5
		s = 0.5 / s;
		x = (m02 + m20) * s;
		y = (m21 + m12) * s;
		w = (m10 - m01) * s;
	}

	r.v = (vec3) {x, y, z};
	r.w = w;
	return r;
}

#if 0
// http://allenchou.net/2018/05/game-math-swing-twist-interpolation-sterp/
void quaternion_decompose_swing_twist(quaternion src, vec3 direction, quaternion swing, quaternion twist) {
	quaternion tmp;
	vec3 d;
	float norm;

	d = vec3_normalize(direction);
	twist.v = vec3_mult(vec3_dot(src.v, d), d);
	twist[0] = src[0];
	if ((norm = norm4(twist)) != 0.0) {
		scale4v(twist, 1.0 / norm);
		tmp[0] = twist[0];
		tmp[1] = -twist[1];
		tmp[2] = -twist[2];
		tmp[3] = -twist[3];
		quaternion_mult(swing, src, tmp);
	} else {
		quaternion_load_id(twist);
		swing = src;
	}
}
#endif

void quaternion_print(quaternion q) {
	printf("{{ %f, %f, %f }, %f}\n", q.v.x, q.v.y, q.v.z, q.w);
}

float
ray_distance_to_plane(vec3 org, vec3 dir, vec4 plane)
{
	/* p on ray   : p = t * dir + org */
	/* p on plane : (p-s) . n = 0 */
	/* t = (s-o).n / d.n */
	vec3 n = { plane.x , plane.y, plane.z };
	vec3 s = vec3_mult(plane.w, n);
	return vec3_dot(vec3_sub(s, org), n) / vec3_dot(dir, n);
}

int
point_in_triangle(vec3 q, vec3 a, vec3 b, vec3 c)
{
	/* triangle:
	 *      + c
	 *     / \
	 *    /   \
	 * a +-----+ b
	 */
	/* n = (b - a) x (c - a) */
	vec3 n = vec3_normalize(vec3_cross(vec3_sub(b, a), vec3_sub(c, a)));
	vec3 e1 = vec3_sub(b, a);
	vec3 e2 = vec3_sub(c, b);
	vec3 e3 = vec3_sub(a, c);
	vec3 qa = vec3_sub(q, a);
	vec3 qb = vec3_sub(q, b);
	vec3 qc = vec3_sub(q, c);
	vec3 x1 = vec3_cross(e1, qa);
	vec3 x2 = vec3_cross(e2, qb);
	vec3 x3 = vec3_cross(e3, qc);

	return vec3_dot(x1, n) >= 0
		&& vec3_dot(x2, n) >= 0
		&& vec3_dot(x3, n) >= 0;
}
#if 0
static float
mat4_minor(const mat4 *m, const size_t r0, const size_t r1, const size_t r2, const size_t c0, const size_t c1, const size_t c2)
{
	float mi;
	mi  = m->m[r0][c0] * (m->m[r1][c1] * m->m[r2][c2] - m->m[r2][c1] * m->m[r1][c2]);
	mi -= m->m[r0][c1] * (m->m[r1][c0] * m->m[r2][c2] - m->m[r2][c0] * m->m[r1][c2]);
	mi += m->m[r0][c2] * (m->m[r1][c0] * m->m[r2][c1] - m->m[r2][c0] * m->m[r1][c1]);
	return mi;
}

mat4 mat4_adjoint(const mat4 *m)
{
	mat4 a = {{
			{ mat4_minor(m, 1, 2, 3, 1, 2, 3), -mat4_minor(m, 0, 2, 3, 1, 2, 3), mat4_minor(m, 0, 1, 3, 1, 2, 3), -mat4_minor(m, 0, 1, 2, 1, 2, 3)},
			{ -mat4_minor(m, 1, 2, 3, 0, 2, 3), mat4_minor(m, 0, 2, 3, 0, 2, 3), -mat4_minor(m, 0, 1, 3, 0, 2, 3), mat4_minor(m, 0, 1, 2, 0, 2, 3)},
			{ mat4_minor(m, 1, 2, 3, 0, 1, 3), -mat4_minor(m, 0, 2, 3, 0, 1, 3), mat4_minor(m, 0, 1, 3, 0, 1, 3), -mat4_minor(m, 0, 1, 2, 0, 1, 3)},
			{-mat4_minor(m, 1, 2, 3, 0, 1, 2), mat4_minor(m, 0, 2, 3, 0, 1, 2), -mat4_minor(m, 0, 1, 3, 0, 1, 2), mat4_minor(m, 0, 1, 2, 0, 1, 2)},
		}};
	return a;
}

float
mat4_det(const mat4 *m)
{
	float d = 0.0;
	d += m->m[0][0] * mat4_minor(m, 1, 2, 3, 1, 2, 3);
	d -= m->m[0][1] * mat4_minor(m, 1, 2, 3, 0, 2, 3);
	d += m->m[0][2] * mat4_minor(m, 1, 2, 3, 0, 1, 3);
	d -= m->m[0][3] * mat4_minor(m, 1, 2, 3, 0, 1, 2);
	return d;
}

mat4
mat4_inverse(const mat4 *m)
{
	mat4 a = mat4_adjoint(m);
	return mat4_mult(&a, (1.0f / mat4_det(m)));
}
#endif
