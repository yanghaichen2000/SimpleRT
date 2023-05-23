#pragma once
#ifndef CYLINDER_H
#define CYLINDER_H

#include "hittable.h"
#include "vec3.h"


class cylinder : public hittable {
public:
	point3 center;
	double radius;
	double height;
	shared_ptr<material> mat_ptr;

public:
	cylinder() {}
	cylinder(point3 cen, double r, double h, shared_ptr<material> m) : center(cen), radius(r), height(h), mat_ptr(m){};

	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {

		double t_min_in_cylinder = infinity;
		bool hit_flag = false;
		
		// 对光线位置进行偏移（将圆柱中心的x和z分量变为0）
		ray ray_0 = r;
		ray_0.dir = normalize(ray_0.dir);
		ray_0.orig = ray_0.orig - vec3(center[0], 0.0, center[2]);

		// 获取求交方程系数
		double func_a = ray_0.dir[0] * ray_0.dir[0] + ray_0.dir[2] * ray_0.dir[2];
		double func_b = 2.0 * (ray_0.dir[0] * ray_0.orig[0] + ray_0.dir[2] * ray_0.orig[2]);
		double func_c = ray_0.orig[0] * ray_0.orig[0] + ray_0.orig[2] * ray_0.orig[2] - radius * radius;

		double b2_4ac = func_b * func_b - 4.0 * func_a * func_c;

		
		// 判断与圆柱相交情况（不含盖）
		if (b2_4ac < 0) { // 不相交
			return false;
		}
		else {

			// 验证交点1
			double t_1 = (-func_b - sqrt(b2_4ac)) / (2.0 * func_a);
			vec3 p_1 = r.orig + t_1 * r.dir;
			if (t_1 > t_min and t_1 < t_max and
				p_1[1] >= center[1] - 0.5 * height and p_1[1] <= center[1] + 0.5 * height) {
				rec.p = p_1;
				vec3 outward_normal = normalize(vec3(p_1[0] - center[0], 0.0, p_1[2] - center[2]));
				rec.set_face_normal(r, outward_normal);
				rec.t = t_1;
				rec.mat_ptr = mat_ptr;
				rec.uv = vec3(0.0, 0.0, 0.0);
				hit_flag = true;
				t_min_in_cylinder = t_1;
			}

			// 验证交点2
			double t_2 = (-func_b + sqrt(b2_4ac)) / (2.0 * func_a);
			vec3 p_2 = r.orig + t_2 * r.dir;
			if (t_2 < t_min_in_cylinder and
				t_2 > t_min and t_2 < t_max and
				p_2[1] >= center[1] - 0.5 * height and p_2[1] <= center[1] + 0.5 * height) {
				rec.p = p_2;
				vec3 outward_normal = normalize(vec3(p_2[0] - center[0], 0.0, p_2[2] - center[2]));
				rec.set_face_normal(r, outward_normal);
				rec.t = t_2;
				rec.mat_ptr = mat_ptr;
				rec.uv = vec3(0.0, 0.0, 0.0);
				hit_flag = true;
				t_min_in_cylinder = t_2;
			}
		}
		

		// 判断与上下盖相交情况
		
		if (ray_0.dir[1] != 0) {

			double t_3 = (center[1] + 0.5 * height - ray_0.orig[1]) / ray_0.dir[1];
			vec3 p_3 = r.orig + t_3 * r.dir;
			if (t_3 < t_min_in_cylinder and
				t_3 > t_min and t_3 < t_max and
				(p_3 - center - vec3(0.0, 0.5 * height, 0.0)).length_squared() <= radius * radius) {
				rec.p = p_3;
				vec3 outward_normal = vec3(0.0, 1.0, 0.0);
				rec.set_face_normal(r, outward_normal);
				rec.t = t_3;
				rec.mat_ptr = mat_ptr;
				rec.uv = vec3(0.0, 0.0, 0.0);
				hit_flag = true;
				t_min_in_cylinder = t_3;
			}

			double t_4 = (center[1] + 0.5 * height - ray_0.orig[1]) / ray_0.dir[1];
			vec3 p_4 = r.orig + t_4 * r.dir;
			if (t_4 < t_min_in_cylinder and 
				t_4 > t_min and t_4 < t_max and
				(p_4 - center + vec3(0.0, 0.5 * height, 0.0)).length_squared() <= radius * radius) {
				rec.p = p_4;
				vec3 outward_normal = vec3(0.0, -1.0, 0.0);
				rec.set_face_normal(r, outward_normal);
				rec.t = t_4;
				rec.mat_ptr = mat_ptr;
				rec.uv = vec3(0.0, 0.0, 0.0);
				hit_flag = true;
				t_min_in_cylinder = t_4;
			}
		}
		
		return hit_flag;
	}

	virtual bounds3 bounds() const override {
		vec3 tmp_vec(radius, height * 0.5, radius);
		return bounds3(center + tmp_vec, center - tmp_vec);
	}
};

#endif