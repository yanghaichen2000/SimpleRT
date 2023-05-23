#pragma once
#ifndef CONE_H
#define CONE_H

#include "hittable.h"
#include "vec3.h"
#include "global.h"


class cone : public hittable {
public:
	point3 center; // 半径为0处
	double height; // 盖子相比顶点在y方向的位移距离
	double k; // r = k * delta_y
	shared_ptr<material> mat_ptr;

public:
	cone() {}
	cone(point3 cen, double h, double k, shared_ptr<material> m) : center(cen), height(h), k(k), mat_ptr(m){};

	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {

		double t_min_in_cone = infinity;
		bool hit_flag = false;

		// 对光线位置进行偏移（将圆锥中心变为原点）
		ray ray_0 = r;
		ray_0.dir = normalize(ray_0.dir);
		ray_0.orig = ray_0.orig - center;

		// 获取求交方程系数
		double k2 = k * k;
		double func_a = ray_0.dir[0] * ray_0.dir[0] + ray_0.dir[2] * ray_0.dir[2] -
			k2 * ray_0.dir[1] * ray_0.dir[1];
		double func_b = 2.0 * (ray_0.dir[0] * ray_0.orig[0] + ray_0.dir[2] * ray_0.orig[2] - 
			k2 * ray_0.dir[1] * ray_0.orig[1]);
		double func_c = ray_0.orig[0] * ray_0.orig[0] + ray_0.orig[2] * ray_0.orig[2] - 
			k2 * ray_0.orig[1] * ray_0.orig[1];

		double b2_4ac = func_b * func_b - 4.0 * func_a * func_c;

		
		// 判断与圆锥相交情况（不含盖）
		if (b2_4ac < 0) { // 不相交
			return false;
		}
		else {

			double y_min, y_max;
			if (height > 0) {
				y_max = center[1] + height;
				y_min = center[1];
			}
			else {
				y_max = center[1];
				y_min = center[1] + height;
			}

			// 验证交点1
			double t_1 = (-func_b - sqrt(b2_4ac)) / (2.0 * func_a);
			vec3 p_1 = r.orig + t_1 * r.dir;
			if (t_1 > t_min and t_1 < t_max and p_1[1] >= y_min and p_1[1] <= y_max) {
				rec.p = p_1;
				vec3 outward_normal = normalize(vec3(p_1[0] - center[0], 0.0, p_1[2] - center[2]));
				if (height < 0) {
					outward_normal = normalize(outward_normal + vec3(0.0, abs(k), 0.0));
				}
				else {
					outward_normal = normalize(outward_normal - vec3(0.0, abs(k), 0.0));
				}
				rec.set_face_normal(r, outward_normal);
				rec.t = t_1;
				rec.mat_ptr = mat_ptr;
				rec.uv = vec3(0.0, 0.0, 0.0);
				hit_flag = true;
				t_min_in_cone = t_1;
			}

			// 验证交点2
			double t_2 = (-func_b + sqrt(b2_4ac)) / (2.0 * func_a);
			vec3 p_2 = r.orig + t_2 * r.dir;
			if (t_2 < t_min_in_cone and
				t_2 > t_min and t_2 < t_max and p_2[1] >= y_min and p_2[1] <= y_max) {
				rec.p = p_2;
				vec3 outward_normal = normalize(vec3(p_2[0] - center[0], 0.0, p_2[2] - center[2]));
				if (height < 0) {
					outward_normal = normalize(outward_normal + vec3(0.0, abs(k), 0.0));
				}
				else {
					outward_normal = normalize(outward_normal - vec3(0.0, abs(k), 0.0));
				}
				rec.set_face_normal(r, outward_normal);
				rec.t = t_2;
				rec.mat_ptr = mat_ptr;
				rec.uv = vec3(0.0, 0.0, 0.0);
				hit_flag = true;
				t_min_in_cone = t_2;
			}
		}
		

		// 判断与上盖或下盖相交情况
		if (ray_0.dir[1] != 0) {
			double t_3 = height / ray_0.dir[1];
			vec3 p_3 = r.orig + t_3 * r.dir;
			double radius = abs(height * k);
			if (t_3 < t_min_in_cone and
				t_3 > t_min and t_3 < t_max and
				(p_3 - center - vec3(0.0, height, 0.0)).length_squared() <= radius * radius) {
				rec.p = p_3;
				vec3 outward_normal = height > 0 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, -1.0, 0.0);
				rec.set_face_normal(r, outward_normal);
				rec.t = t_3;
				rec.mat_ptr = mat_ptr;
				rec.uv = vec3(0.0, 0.0, 0.0);
				hit_flag = true;
				t_min_in_cone = t_3;
			}
		}

		return hit_flag;
		
	}

	virtual bounds3 bounds() const override {
		double radius = abs(height * k);
		vec3 displacement_1(-radius, 0, -radius);
		vec3 displacement_2(radius, height, radius);
		return bounds3(center + displacement_1, center + displacement_2);
	}
};

#endif