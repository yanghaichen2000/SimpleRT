#pragma once
#ifndef MATERIAL_H
#define MATERIAL_H

#include "global.h"

struct hit_record;

class material {
public:
	// �������䷽������wi
	// ����ֵΪPDF
	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const = 0;
	
	// ����brdf��
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal) const = 0;
	
	// ��ȡ�����Է���ǿ��
	virtual vec3 get_radiance() const = 0;
};


// Blinn-Phong diffuse����
class phong_material : public material {
public:
	vec3 radiance; // �Է���ǿ��
	vec3 albedo; // ������

public:
	phong_material(vec3 albedo_init = vec3(1.0, 1.0, 1.0), vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		radiance = radiance_init;
		albedo = albedo_init;
	}

	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const override {
		/*
		// ���ϰ�����Ȳ���
		wi = unit_vector(random_in_hemisphere(normal));
		// pdfΪ����
		return pi2_inv;
		*/

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

		double pdf = cos(theta) * pi_inv;

		return pdf;
	}

	// diffuse���ʵ�brdfΪ����
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal) const override {
		return albedo * pi_inv;
	}

	virtual vec3 get_radiance() const override {
		return radiance;
	}
};


// GGX����
class ggx_material : public material {
public:
	double a; // �ֲڶȣ�[0,1]
	vec3 F0; // ���������ʼֵ����ɫ��
	vec3 radiance; // �Է���ǿ��

public:
	// F0�ĳ�ʼֵΪ�ƽ�
	ggx_material(const double &a_init = 0.5, const vec3 &F0_init = vec3(1, 0.582, 0.0956)) {
		a = a_init;
		F0 = F0_init;
	}

	// �������뵥λ����!!!!!!!!
	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const override {
		/*
		// ���ϰ�����Ȳ���
		wi = unit_vector(random_in_hemisphere(normal));
		// pdfΪ����
		return pi2_inv;
		*/
		
		if (random_double() > 0)
		{
			// ��Ҫ�Բ���΢���淨�ߣ�ʹ�õķֲ�Ϊggx NDF
			double rand1, rand2, theta, phi;
			vec3 h;

			/*
			bool wi_ok = false;
			while (!wi_ok) {
				// ���ȼ���ֲ������theta��phi
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

				if (dot(wi, normal) > 0) {
					wi_ok = true;
				}
			}
			*/
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
		else
		{
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

			double pdf = cos(theta) * pi_inv;

			return pdf;
		}
	}

	// �������뵥λ����!!!!!!!!
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal) const override {
		// F ��������
		vec3 h = unit_vector(wi + wo);
		vec3 F = F0 + (vec3(1.0, 1.0, 1.0) - F0) * pow(1 - dot(wo, h), 5);

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
};


#endif