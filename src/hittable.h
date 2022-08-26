#pragma once
#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"
#include "bounds.h"

class material;

struct hit_record {
	vec3 p;
	vec3 normal;
	shared_ptr<material> mat_ptr;
	double t;
	bool front_face;
	vec3 uv;
	int texture_id;

	// 自动设置法线，使得它和光线位于同一个半球
	inline void set_face_normal(const ray& r, const vec3& outward_normal) {
		front_face = dot(r.direction(), outward_normal) < 0;
		normal = front_face ? outward_normal : -outward_normal;
	}
};


// 所有物体的基类
class hittable {
public:
	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const = 0;
	virtual bounds3 bounds() const = 0;
};

#endif