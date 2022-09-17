#pragma once
#ifndef MIXED_MATERIAL_H
#define MIXED_MATERIAL_H
#include "material.h"
using std::vector;

class mixed_material : public material {
public:
	shared_ptr<material> mat_ptr_1;
	shared_ptr<material> mat_ptr_2;

public:
	// 采样入射角
	// 返回值为{pdf，入射角，入射方向是否和法线同向}
	tuple<double, vec3, bool> sample_wi(const vec3& wo, const vec3& normali, bool wo_front) const override {

	};

	// 采样入射点
	// 返回值为{pdf，入射点法线，入射点坐标}
	tuple<double, vec3, vec3> sample_positioni(const vec3& normalo, const vec3& positiono, shared_ptr<hittable> world) const override {
		
	}


	// 计算brdf项
	vec3 bsdf(const vec3 & wo, const vec3 & normalo, const vec3 & positiono, bool wo_front, const vec3 & uv,
		const vec3 & wi, const vec3 & normali, const vec3 & positioni, bool wi_front) const override {

	}

	// 获取材质自发光强度
	vec3 get_radiance() const override {

	}

	// 获取法线贴图指针
	shared_ptr<texture> get_normal_map_ptr() const override;

	// 获取图像纹理指针
	shared_ptr<texture> get_color_map_ptr() const override;

	// 获取介质
	// 可能返回空指针
	shared_ptr<medium> get_medium_outside_ptr() const override;
	shared_ptr<medium> get_medium_inside_ptr() const override;

	// 是否需要对光源采样
	bool sample_light() const override;

	// 获取材质编号
	int get_material_number() const override;
	
};


#endif