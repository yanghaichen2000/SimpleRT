#pragma once
#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"

class sphere : public hittable {
public:
	point3 center;
	double radius;
	shared_ptr<material> mat_ptr;

public:
	sphere() {}
	sphere(point3 cen, double r, shared_ptr<material> m) : center(cen), radius(r), mat_ptr(m){};

	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
		// 计算t
		vec3 oc = r.origin() - center;
		auto a = r.direction().length_squared();
		auto half_b = dot(oc, r.direction());
		auto c = oc.length_squared() - radius * radius;

		auto discriminant = half_b * half_b - a * c;
		// 如果判别式小于零则无交点
		if (discriminant < 0) return false;
		auto sqrtd = sqrt(discriminant);

		// Find the nearest root that lies in the acceptable range.
		// 注意这里的参数a恒正，所以较小的t可以直接计算
		// 如果较小的t不符合条件，那么再验证较大的t（发生这种情况可能是因为相机在求内部）
		auto root = (-half_b - sqrtd) / a;
		if (root < t_min || t_max < root) {
			root = (-half_b + sqrtd) / a;
			if (root < t_min || t_max < root)
				return false;
		}

		rec.t = root;
		rec.p = r.at(rec.t);
		rec.mat_ptr = mat_ptr;

		// 先计算出向外的法线，再根据ray的方向决定法线是否需要反向
		vec3 outward_normal = (rec.p - center) / radius;
		rec.set_face_normal(r, outward_normal);

		rec.uv = vec3(0, 0, 0); // 暂时未启用
		rec.texture_id = 0; // 暂时未启用

		return true;
	}

	virtual bounds3 bounds() const override {
		vec3 tmp_vec(radius, radius, radius);
		return bounds3(center + tmp_vec, center - tmp_vec);
	}
};

#endif