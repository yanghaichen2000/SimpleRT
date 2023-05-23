#pragma once
#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>

using std::sqrt;
using std::cos;
using std::sin;


class vec3 {

public:
	double e[3];

public:
	vec3() : e{ 0,0,0 } {}
	vec3(double e0) : e{ e0, e0, e0 } {}
	vec3(double e0, double e1, double e2) : e{ e0, e1, e2 } {}

	double x() const { return e[0]; }
	double y() const { return e[1]; }
	double z() const { return e[2]; }

	vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
	double operator[](int i) const { return e[i]; }
	double& operator[](int i) { return e[i]; }

	vec3& operator+=(const vec3 &v) {
		e[0] += v.e[0];
		e[1] += v.e[1];
		e[2] += v.e[2];
		return *this;
	}

	vec3& operator*=(const double t) {
		e[0] *= t;
		e[1] *= t;
		e[2] *= t;
		return *this;
	}

	vec3& operator/=(const double t) {
		return *this *= 1.0 / t;
	}

	double length() const {
		return sqrt(length_squared());
	}

	double length_squared() const {
		return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
	}

	// ���ű任
	vec3& scale(vec3 scale_vec) {
		e[0] *= scale_vec[0];
		e[1] *= scale_vec[1];
		e[2] *= scale_vec[2];
		return *this;
	}

	// ��ת�任���ֱ�Ϊ��x,y,z����ת�Ļ���
	vec3& rotate(vec3 rotate_vec) {
		// ��x����ת
		e[1] = cos(rotate_vec[0]) * e[1] - sin(rotate_vec[0]) * e[2];
		e[2] = sin(rotate_vec[0]) * e[1] + cos(rotate_vec[0]) * e[2];
		// ��y����ת
		e[0] = cos(rotate_vec[1]) * e[0] + sin(rotate_vec[1]) * e[2];
		e[2] = - sin(rotate_vec[1]) * e[0] + cos(rotate_vec[1]) * e[2];
		// ��z����ת
		e[0] = cos(rotate_vec[2]) * e[0] - sin(rotate_vec[2]) * e[1];
		e[1] = sin(rotate_vec[2]) * e[0] + cos(rotate_vec[2]) * e[1];
		return *this;
	}

	// ƽ�Ʊ任
	vec3& translate(vec3 scale_vec) {
		e[0] += scale_vec[0];
		e[1] += scale_vec[1];
		e[2] += scale_vec[2];
		return *this;
	}
	
	// ����[0, 1] ^ 3�о��ȷֲ����������
	inline static vec3 random() {
		return vec3(random_double(), random_double(), random_double());
	}

	// ָ����Χ�ľ����������
	inline static vec3 random(double min, double max) {
		return vec3(random_double(min, max), random_double(min, max), random_double(min, max));
	}
};


// iostream���
inline std::ostream& operator<<(std::ostream &out, const vec3 &v) {
	return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline vec3 operator+(const vec3 &u, const vec3 &v) {
	return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

inline vec3 operator-(const vec3 &u, const vec3 &v) {
	return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

// ����ĳ��Ƕ�ӦԪ����ˣ���glslһ��
inline vec3 operator*(const vec3 &u, const vec3 &v) {
	return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline vec3 operator*(double t, const vec3 &v) {
	return vec3(t*v.e[0], t*v.e[1], t*v.e[2]);
}

inline vec3 operator*(const vec3 &v, double t) {
	return t * v;
}

inline vec3 operator/(vec3 v, double t) {
	return (1 / t) * v;
}

inline double dot(const vec3 &u, const vec3 &v) {
	return u.e[0] * v.e[0]
		+ u.e[1] * v.e[1]
		+ u.e[2] * v.e[2];
}

inline vec3 cross(const vec3 &u, const vec3 &v) {
	return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
		u.e[2] * v.e[0] - u.e[0] * v.e[2],
		u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

// normalize
inline vec3 unit_vector(vec3 v) {
	return v / v.length();
}

// normalize
inline vec3 normalize(vec3 v) {
	return v / v.length();
}

// ��λ���ھ��Ȳ���
vec3 random_in_unit_sphere() {
	while (true) {
		auto p = vec3::random(-1, 1);
		if (p.length_squared() >= 1) continue;
		return p;
	}
}

// ��λ�����ھ��Ȳ���
vec3 random_in_hemisphere(const vec3& normal) {
	vec3 in_unit_sphere = random_in_unit_sphere();
	if (dot(in_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
		return in_unit_sphere;
	else
		return -in_unit_sphere;
}

// ����������
// Jeppe Revall Frisvad����
// ���ݵ�λ���������������� - ��С�������� - ֪�� zhuanlan.zhihu.com/p/351071035
void build_basis(const vec3& n, vec3& b1, vec3& b2) {
	if (n.z() < -0.9999999f)  // Handle the singularity��һ�㲻�ᷢ��
	{
		b1 = vec3(0, -1, 0);
		b2 = vec3(-1, 0, 0);
		return;
	}

	const double a = -1 / (1 + n.z());
	const double b = n.x() * n.y() * a;
	b1 = vec3(1 + n.x() * n.x() * a, b, -n.x());
	b2 = vec3(b, 1 + n.y() * n.y() * a, -n.y());
}

vec3 clamp(const vec3 &vec, double min, double max)
{
	return vec3(clamp(vec.x(), min, max), clamp(vec.y(), min, max), clamp(vec.z(), min, max));
}

vec3 mix(const vec3 &x, const vec3 &y, const vec3 &weight) {
	return y * (vec3(1, 1, 1) - weight) + x * weight;
}

vec3 mix(const vec3& x, const vec3& y, double weight) {
	return y * (1.0 - weight) + x * weight;
}

double mix(double x, double y, double weight) {
	return y * (1.0 - weight) + x * weight;
}

// Type aliases for vec3
using point3 = vec3;   // 3D point
using color = vec3;    // RGB color

#endif