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
	int texture_id;
	vec3 vertex[3]; // 顶点坐标
	vec3 uv[3]; // 纹理空间坐标，使用xy项作为uv
	shared_ptr<material> mat_ptr; // material
	vec3 normal; // 三角形自身的法线

public:
	// 使用三个顶点和一个material初始化
	// 同时也可以传入三个顶点的uv
	triangle(const vec3 &vertex_1, const vec3 &vertex_2, const vec3 &vertex_3, const shared_ptr<material> &mat_ptr_all, 
		const vec3 &uv_1 = vec3(), const vec3 &uv_2 = vec3(), const vec3 &uv_3 = vec3(), int texture_id_init = -1) {
		
		// 设置基本参数
		
		vertex[0] = vertex_1;
		vertex[1] = vertex_2;
		vertex[2] = vertex_3;
		mat_ptr = mat_ptr_all;
		uv[0] = uv_1;
		uv[1] = uv_2;
		uv[2] = uv_3;
		texture_id = texture_id_init;

		// 计算辅助参数
		
		// 法线，右手系
		normal = unit_vector(cross(vertex[1] - vertex[0], vertex[2] - vertex[0]));
	}

	// 这个函数没有标const是因为与map[]操作冲突，实际上该函数并不会改变成员变量
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

		// 计算法线
		rec.set_face_normal(r, normal);

		// 计算texture中的material参数并修改
		rec.mat_ptr = mat_ptr;

		// 返回自己的指针
		rec.texture_id = texture_id;

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