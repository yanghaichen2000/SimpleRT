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
	// ���������
	// ����ֵΪ{pdf������ǣ����䷽���Ƿ�ͷ���ͬ��}
	tuple<double, vec3, bool> sample_wi(const vec3& wo, const vec3& normali, bool wo_front) const override {

	};

	// ���������
	// ����ֵΪ{pdf������㷨�ߣ����������}
	tuple<double, vec3, vec3> sample_positioni(const vec3& normalo, const vec3& positiono, shared_ptr<hittable> world) const override {
		
	}


	// ����brdf��
	vec3 bsdf(const vec3 & wo, const vec3 & normalo, const vec3 & positiono, bool wo_front, const vec3 & uv,
		const vec3 & wi, const vec3 & normali, const vec3 & positioni, bool wi_front) const override {

	}

	// ��ȡ�����Է���ǿ��
	vec3 get_radiance() const override {

	}

	// ��ȡ������ͼָ��
	shared_ptr<texture> get_normal_map_ptr() const override;

	// ��ȡͼ������ָ��
	shared_ptr<texture> get_color_map_ptr() const override;

	// ��ȡ����
	// ���ܷ��ؿ�ָ��
	shared_ptr<medium> get_medium_outside_ptr() const override;
	shared_ptr<medium> get_medium_inside_ptr() const override;

	// �Ƿ���Ҫ�Թ�Դ����
	bool sample_light() const override;

	// ��ȡ���ʱ��
	int get_material_number() const override;
	
};


#endif