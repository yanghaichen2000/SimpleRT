#pragma once
#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <array>
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "texture.h"
using std::array;


class triangle : public hittable {
public:
	vec3 vertex[3]; // 顶点坐标
	vec3 uv[3]; // 纹理空间坐标，使用xy项作为uv
	shared_ptr<material> mat_ptr; // material
	vec3 vertex_normal[3]; // 顶点法线
	vec3 tangent; // 切线。实现法线贴图时需要用到该信息

public:
	// 使用三个顶点的位置和法线以及一个material初始化
	triangle(const vec3 &vertex_1, const vec3 &vertex_2, const vec3 &vertex_3, 
		const shared_ptr<material> &mat_ptr_all, 
		const vec3 &uv_1, const vec3 &uv_2, const vec3 &uv_3, 
		const vec3 &vertex_normal_1, const vec3 &vertex_normal_2, const vec3 &vertex_normal_3) {
		
		// 设置基本参数
		vertex[0] = vertex_1;
		vertex[1] = vertex_2;
		vertex[2] = vertex_3;

		mat_ptr = mat_ptr_all;

		uv[0] = uv_1;
		uv[1] = uv_2;
		uv[2] = uv_3;
		
		// 直接记录传入的顶点法线
		vertex_normal[0] = vertex_normal_1;
		vertex_normal[1] = vertex_normal_2;
		vertex_normal[2] = vertex_normal_3;

		// 计算切线
		// 原理可以参考zhuanlan.zhihu.com/p/414940275
		vec3 AB = vertex[1] - vertex[0];
		vec3 AC = vertex[2] - vertex[0];
		double du1 = uv[2][0] - uv[0][0];
		double du2 = uv[1][0] - uv[0][0];
		double dv1 = uv[2][1] - uv[0][1];
		double dv2 = uv[1][1] - uv[0][1];
		// 得到切线（未正交化）
		tangent = (dv1 * AB - dv2 * AC) / (du2 * dv1 - du1 * dv2);
		tangent = unit_vector(tangent);
	}

	// 使用三个顶点的位置（无法线）以及一个material初始化
	triangle(const vec3 &vertex_1, const vec3 &vertex_2, const vec3 &vertex_3,
		const shared_ptr<material> &mat_ptr_all) {

		// 设置基本参数
		vertex[0] = vertex_1;
		vertex[1] = vertex_2;
		vertex[2] = vertex_3;

		mat_ptr = mat_ptr_all;

		// 不支持uv
		uv[0] = vec3(0, 0, 0);
		uv[1] = vec3(0, 0, 0);
		uv[2] = vec3(0, 0, 0);

		// 根据顶点位置计算法线（右手系）
		vertex_normal[0] = unit_vector(cross(vertex[1] - vertex[0], vertex[2] - vertex[0]));
		vertex_normal[1] = vertex_normal[0];
		vertex_normal[2] = vertex_normal[0];

		// 该构造函数不支持uv，所以用不了法线贴图，所以不需要计算切线
		tangent = vec3(0, 0, 0);
	}

	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
		// 计算t以及重心坐标，同时判断光线与三角形是否相交
		// 顶点0,1,2的权重分别为(1-b1-b2),b1,b2
		vec3 E1 = vertex[1] - vertex[0];
		vec3 E2 = vertex[2] - vertex[0];
		vec3 S = r.orig - vertex[0];
		vec3 S1 = cross(r.dir, E2);
		vec3 S2 = cross(S, E1);
		double S1E1_inv = 1 / dot(S1, E1);
		double t = dot(S2, E2) * S1E1_inv;
		if (t < t_min or t > t_max) return false; // t在设定范围之外则认为没有交点
		double b1 = dot(S1, S) * S1E1_inv;
		double b2 = dot(S2, r.dir) * S1E1_inv;
		if (b1 < 0 or b2 < 0 or b1 + b2 > 1) return false; // 交点在三角形外也无交点

		// 将重心坐标存入三个变量中
		double w0 = 1 - b1 - b2;
		double w1 = std::move(b1);
		double w2 = std::move(b2);

		// 记录距离
		rec.t = t;

		// 计算交点坐标
		rec.p = r.orig + t * r.dir;

		// 计算uv
		rec.uv = w0 * uv[0] + w1 * uv[1] + w2 * uv[2];

		// 计算法线，
		// 这里使用平滑着色，根据顶点法线插值得到shading point法线
		vec3 normal = w0 * vertex_normal[0] + w1 * vertex_normal[1] + w2 * vertex_normal[2];
		normal = unit_vector(normal);

		// 如果存在法线贴图则使用法线贴图对法线进行处理
		if (mat_ptr->get_normal_map_ptr() != nullptr) {
			// 根据成员变量tangent来计算正交且单位的切线基向量
			vec3 basis_t = unit_vector(tangent - dot(tangent, normal) * normal);
			// 计算副切线基向量
			vec3 basis_b = cross(basis_t, normal);
			// 从法线贴图获取切线空间坐标
			vec3 tangent_coord = mat_ptr->get_normal_map_ptr()->get_value(rec.uv);
			// 计算法线
			normal = basis_t * tangent_coord[0] + basis_b * tangent_coord[1] + normal * tangent_coord[2];
		}

		rec.set_face_normal(r, normal);

		// 计算texture中的material参数并修改
		rec.mat_ptr = mat_ptr;

		return true;
	}

	virtual bounds3 bounds() const override {
		bounds3 ret(vertex[0], vertex[1]);
		ret = Union(ret, vertex[2]);

		// 因为三角形可能和坐标平面平行，所以将包围盒扩大一点，否则容易导致求交结果必定为false
		return Expand(ret, 0.00000001);
	}
};

#endif