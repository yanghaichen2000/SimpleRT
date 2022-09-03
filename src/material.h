#pragma once
#ifndef MATERIAL_H
#define MATERIAL_H

#include "global.h"
#include "texture.h"

struct hit_record;

class material {
public:
	// �������䷽������wi
	// ����ֵΪPDF
	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const = 0;
	
	// ����brdf��
	// �����normal�Ǽ��η��ߣ������Ǿ���normal map�任��ķ���
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal, const vec3 &uv) const = 0;
	
	// ��ȡ�����Է���ǿ��
	virtual vec3 get_radiance() const = 0;

	// ��ȡ������ͼָ��
	virtual shared_ptr<texture> get_normal_map_ptr() const = 0;

	// ��ȡͼ������ָ��
	virtual shared_ptr<texture> get_color_map_ptr() const = 0;
};


// Blinn-Phong diffuse����
class phong_material : public material {
public:
	vec3 radiance; // �Է���ǿ��
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;
	double kd = 0.9; // ������ϵ��
	double ks = 0.1; // ���淴��ϵ��
	double a = 0; // �߹������Խ����߹�ԽС��Ĭ��a = 0�������������

public:
	// ʹ��һ����ɫ���������
	// �Զ���������Ϊsimple_color_texture������
	phong_material(vec3 albedo_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		radiance = radiance_init;
		color_map_ptr = make_shared<simple_color_texture>(albedo_init);
		normal_map_ptr = nullptr;
	}

	// ʹ�ø߹������һ����ɫ���������
	// �Զ���������Ϊsimple_color_texture������
	phong_material(double a_init, vec3 albedo_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		radiance = radiance_init;
		color_map_ptr = make_shared<simple_color_texture>(albedo_init);
		a = a_init;
		normal_map_ptr = nullptr;
	}

	// ʹ��color_map���������
	phong_material(shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr) {
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
	}

	// ʹ�ø߹������color_map���������
	phong_material(double a_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr) {
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		a = a_init;
		normal_map_ptr = normal_map_ptr_init;
	}

	// www.cs.princeton.edu/courses/archive/fall08/cos526/assign3/lawrence.pdf
	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const override {
		
		// ���cos-weighted�����͸߹�����Ҫ�Բ���
		// ���ֲ�����Ƶ�ʵı�ֵ����kd��ks֮��
		double pdf;

		if (random_double() < kd / (kd + ks)) {

			// cos-weighted����
			double rand1 = random_double();
			double rand2 = random_double();

			double theta = acos(sqrt(1 - rand1));
			double phi = pi2 * rand2;

			// ���÷��߹�������ϵ
			vec3 b1, b2;
			build_basis(normal, b1, b2);

			// �õ�wi
			wi = normal * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta);

			pdf = cos(theta) * pi_inv;
		}
		else {

			// �߹�����Ҫ�Բ���
			double rand1 = random_double();
			double rand2 = random_double();

			// ��������������
			double theta = acos(pow(rand1, 1.0 / (a + 1)));
			double phi = pi2 * rand2;
			
			vec3 b1, b2;
			build_basis(normal, b1, b2);

			vec3 h = normal * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta);
			
			// ���ݾ��淴�����wi
			wi = 2 * h * dot(wo, h) - wo;

			// ������ߴ�͸������һ���ܴ��pdfʹ�ý���޹���
			if (dot(wi, normal) < 0) {
				return 1000000;
			}

			// ����pdf
			pdf = (a + 1) * pi2_inv * pow(cos(theta), a);
		}

		return pdf;
		
	}

	// phong brdf
	// zhuanlan.zhihu.com/p/500811555
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal, const vec3 &uv) const override {
		//auto ret1 = color_map_ptr->get_value(uv) * pi_inv;
		vec3 ret_d = color_map_ptr->get_value(uv) * kd * pi_inv;
		vec3 ret_s = vec3(ks, ks, ks) * (a + 2) * pi2_inv * pow(std::max(0.0, dot(normal, unit_vector(wi + wo))), a);

		return ret_d + ret_s;
	}

	virtual vec3 get_radiance() const override {
		return radiance;
	}

	virtual shared_ptr<texture> get_normal_map_ptr() const override {
		return normal_map_ptr;
	}

	virtual shared_ptr<texture> get_color_map_ptr() const override {
		return color_map_ptr;
	}
};


// GGX��������
class ggx_metal_material : public material {
public:
	double a; // �ֲڶȣ�[0,1]
	vec3 radiance; // �Է���ǿ��
	shared_ptr<texture> color_map_ptr = nullptr; // ���������ʼֵ����ɫ��
	shared_ptr<texture> normal_map_ptr = nullptr;

public:
	// ��һ����ɫ����material
	ggx_metal_material(const double &a_init, const vec3 &F0_init = vec3(1, 0.582, 0.0956), vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		a = a_init;
		color_map_ptr = make_shared<simple_color_texture>(F0_init);
		radiance = radiance_init;
	}

	// ��color_map����material��ͬʱҲ֧�ַ�����ͼ
	ggx_metal_material(const double &a_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr) {
		
		a = a_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
	}

	// �������뵥λ����!!!!!!!!
	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const override {

		// ��Ҫ�Բ���΢���淨�ߣ�ʹ�õķֲ�Ϊggx NDF
		double rand1, rand2, theta, phi;
		vec3 h;

		rand1 = random_double();
		rand2 = random_double();
		theta = atan(a * sqrt(rand1 / (1 - rand1))); // ΢�۷������۷��ߵļн�
		phi = pi2 * rand2; // ΢�۷�������ƽ��ĽǶ�

		// ���÷��߹�������ϵ
		vec3 b1, b2;
		build_basis(normal, b1, b2);

		// �õ�΢�۷���(��wi��wo�İ������)
		h = normal * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta); // Ū����һ�Σ�ע�⡣

		// ���ݾ��淴�����wi
		wi = 2 * h * dot(wo, h) - wo;

		// ������ߴ�͸������һ���ܴ��pdfʹ�ý���޹���
		if (dot(wi, normal) < 0) {
			return 10000;
		}

		// pdf��NDF�Ƴ�
		double a2 = a * a;
		double pdf = a2 * cos(theta) * pi_inv / pow(pow(cos(theta), 2) * (a2 - 1) + 1, 2) * 0.25 / dot(wo, h);

		return pdf;
	}

	// �������뵥λ����!!!!!!!!
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal, const vec3 &uv) const override {
		// F ��������
		vec3 h = unit_vector(wi + wo);
		vec3 F = color_map_ptr->get_value(uv) + (vec3(1.0, 1.0, 1.0) - color_map_ptr->get_value(uv)) * pow(1 - dot(wo, h), 5);

		// D NDF��
		double a2 = a * a;
		double D = a2 / (pi * pow(pow(dot(normal, h), 2) * (a2 - 1) + 1, 2));

		// G ����������ڵ���
		double dot_n_v = dot(normal, wo);
		double dot_n_l = dot(normal, wi);
		double k = a * 0.5;
		double G = dot_n_v / (dot_n_v * (1 - k) + k) * dot_n_l / (dot_n_l * (1 - k) + k);

		return D * F * G / (4 * dot_n_v * dot_n_l);
	}

	virtual vec3 get_radiance() const override {
		return radiance;
	}

	virtual shared_ptr<texture> get_normal_map_ptr() const override {
		return normal_map_ptr;
	}

	virtual shared_ptr<texture> get_color_map_ptr() const override {
		return color_map_ptr;
	}
};

// GGX�ǽ�������
class ggx_nonmetal_material : public material {
public:
	double a; // �߹���ֲڶȣ�[0,1]
	vec3 radiance; // �Է���ǿ��
	double F0; // ������ϵ���������˸߹������������ı���
	shared_ptr<texture> color_map_ptr = nullptr; // ����������ɫ
	shared_ptr<texture> normal_map_ptr = nullptr;

public:
	// ��color_map����material��ͬʱҲ֧�ַ�����ͼ
	ggx_nonmetal_material(const double &a_init, const double &F0_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr) {

		a = a_init;
		F0 = F0_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
	}

	// ��������Ҫ���brdf�Ͳ�������

	// �������뵥λ����!!!!!!!!
	// ���ʹ��ggx��Ҫ�Բ�������������Ҫ�Բ�����cos-weighted������
	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const override {

		double pdf;

		// ���ֲ���������Ȩ�ظ��ݷ��������������Ϊ�������˸߹���������������������
		if (random_double() < F0) {
			
			// ��Ҫ�Բ���΢���淨�ߣ�ʹ�õķֲ�Ϊggx NDF
			double rand1, rand2, theta, phi;
			vec3 h;

			rand1 = random_double();
			rand2 = random_double();
			theta = atan(a * sqrt(rand1 / (1 - rand1))); // ΢�۷������۷��ߵļн�
			phi = pi2 * rand2; // ΢�۷�������ƽ��ĽǶ�

			// ���÷��߹�������ϵ
			vec3 b1, b2;
			build_basis(normal, b1, b2);

			// �õ�΢�۷���(��wi��wo�İ������)
			h = normal * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta); // Ū����һ�Σ�ע�⡣

			// ���ݾ��淴�����wi
			wi = 2 * h * dot(wo, h) - wo;

			// ������ߴ�͸������һ���ܴ��pdfʹ�ý���޹���
			if (dot(wi, normal) < 0) {
				return 10000;
			}

			// pdf��NDF�Ƴ�
			double a2 = a * a;
			pdf = a2 * cos(theta) * pi_inv / pow(pow(cos(theta), 2) * (a2 - 1) + 1, 2) * 0.25 / dot(wo, h);
		}
		else {
			
			// cos-weighted����
			double rand1 = random_double();
			double rand2 = random_double();

			double theta = acos(sqrt(1 - rand1));
			double phi = pi2 * rand2;

			// ���÷��߹�������ϵ
			vec3 b1, b2;
			build_basis(normal, b1, b2);

			// �õ�wi
			wi = normal * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta);

			pdf = cos(theta) * pi_inv;
		}

		return pdf;
	}

	// �������뵥λ����!!!!!!!!
	// brdf������������߹���ĺ�
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal, const vec3 &uv) const override {

		// �߹��ʹ��ggxģ�ͣ�

		// F ��������
		// ���ڷǽ��������������RGBֵ���
		vec3 h = unit_vector(wi + wo);
		double F_value = F0 + (1.0 - F0) * pow(1 - dot(wo, h), 5);
		vec3 F(F_value, F_value, F_value);
		// D NDF��
		double a2 = a * a;
		double D = a2 / (pi * pow(pow(dot(normal, h), 2) * (a2 - 1) + 1, 2));
		// G ����������ڵ���
		double dot_n_v = dot(normal, wo);
		double dot_n_l = dot(normal, wi);
		double k = a * 0.5;
		double G = dot_n_v / (dot_n_v * (1 - k) + k) * dot_n_l / (dot_n_l * (1 - k) + k);

		vec3 component_ggx = F * D * G / (4 * dot_n_v * dot_n_l);
		component_ggx = clamp(component_ggx, 0, 1);

		// ��������
		vec3 component_diffuse = (vec3(1 - F_value, 1 - F_value, 1 - F_value)) * color_map_ptr->get_value(uv) * pi_inv;

		return component_ggx + component_diffuse;
	}

	virtual vec3 get_radiance() const override {
		return radiance;
	}

	virtual shared_ptr<texture> get_normal_map_ptr() const override {
		return normal_map_ptr;
	}

	virtual shared_ptr<texture> get_color_map_ptr() const override {
		return color_map_ptr;
	}
};


#endif
