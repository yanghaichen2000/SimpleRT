#pragma once
#ifndef MATERIAL_H
#define MATERIAL_H

#include "global.h"
#include "texture.h"

struct hit_record;

class material {
public:
	// 采样入射方向存放于wi
	// 返回值为PDF
	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const = 0;
	
	// 计算brdf项
	// 传入的normal是几何法线，而不是经过normal map变换后的法线
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal, const vec3 &uv) const = 0;
	
	// 获取材质自发光强度
	virtual vec3 get_radiance() const = 0;

	// 获取法线贴图指针
	virtual shared_ptr<texture> get_normal_map_ptr() const = 0;
};


// Blinn-Phong diffuse材质
class phong_material : public material {
public:
	vec3 radiance; // 自发光强度
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;
	vec3 ks = vec3(0.01, 0.01, 0.01); // 镜面反射系数 TODO：暂时固定
	float a = 50; // 高光参数，越大表示越光滑 TODO：暂时固定

public:
	// 使用一个颜色来构造材质，自动生成类型为simple_color_texture的纹理
	phong_material(vec3 albedo_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		radiance = radiance_init;
		color_map_ptr = make_shared<simple_color_texture>(albedo_init);
		normal_map_ptr = nullptr;
	}

	// 使用color_map来构造材质
	phong_material(shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr) {
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
	}

	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const override {
		/*
		// 在上半球均匀采样
		wi = unit_vector(random_in_hemisphere(normal));
		// pdf为常数
		return pi2_inv;
		*/

		// cos-weighted采样
		double rand1 = random_double();
		double rand2 = random_double();

		double theta = acos(sqrt(1 - rand1));
		double phi = pi2 * rand2;

		// 利用法线构建坐标系
		vec3 b1, b2;
		build_basis(normal, b1, b2);

		// 得到wi
		wi = normal * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta);

		double pdf = cos(theta) * pi_inv;

		return pdf;
	}

	// phong brdf
	// zhuanlan.zhihu.com/p/500811555
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal, const vec3 &uv) const override {
		auto ret1 = color_map_ptr->get_value(uv) * pi_inv;
		return ret1;
	}

	virtual vec3 get_radiance() const override {
		return radiance;
	}

	virtual shared_ptr<texture> get_normal_map_ptr() const override {
		return normal_map_ptr;
	}
};


// GGX材质
class ggx_material : public material {
public:
	double a; // 粗糙度，[0,1]
	vec3 F0; // 菲涅尔项初始值（颜色）
	vec3 radiance; // 自发光强度
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;

public:
	// F0的初始值为黄金
	ggx_material(const double &a_init = 0.5, const vec3 &F0_init = vec3(1, 0.582, 0.0956)) {
		a = a_init;
		F0 = F0_init;
	}

	// 必须输入单位向量!!!!!!!!
	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const override {
		/*
		// 在上半球均匀采样
		wi = unit_vector(random_in_hemisphere(normal));
		// pdf为常数
		return pi2_inv;
		*/
		
		if (random_double() > 0)
		{
			// 重要性采样微表面法线，使用的分布为ggx NDF
			double rand1, rand2, theta, phi;
			vec3 h;

			/*
			bool wi_ok = false;
			while (!wi_ok) {
				// 首先计算局部坐标的theta和phi
				rand1 = random_double();
				rand2 = random_double();
				theta = atan(a * sqrt(rand1 / (1 - rand1))); // 微观法线与宏观法线的夹角
				phi = pi2 * rand2; // 微观法线在切平面的角度

				// 利用法线构建坐标系
				vec3 b1, b2;
				build_basis(normal, b1, b2);

				// 得到微观法线(即wi和wo的半程向量)
				h = normal * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta); // 弄错了一次，注意。

				// 根据镜面反射计算wi
				wi = 2 * h * dot(wo, h) - wo;

				if (dot(wi, normal) > 0) {
					wi_ok = true;
				}
			}
			*/
			rand1 = random_double();
			rand2 = random_double();
			theta = atan(a * sqrt(rand1 / (1 - rand1))); // 微观法线与宏观法线的夹角
			phi = pi2 * rand2; // 微观法线在切平面的角度

			// 利用法线构建坐标系
			vec3 b1, b2;
			build_basis(normal, b1, b2);

			// 得到微观法线(即wi和wo的半程向量)
			h = normal * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta); // 弄错了一次，注意。

			// 根据镜面反射计算wi
			wi = 2 * h * dot(wo, h) - wo;

			// 如果光线穿透，返回一个很大的pdf使该结果无贡献
			if (dot(wi, normal) < 0) {
				return 10000;
			}

			// pdf由NDF推出
			double a2 = a * a;
			double pdf = a2 * cos(theta) * pi_inv / pow(pow(cos(theta), 2) * (a2 - 1) + 1, 2) * 0.25 / dot(wo, h);

			return pdf;
		}
		else
		{
			// cos-weighted采样
			double rand1 = random_double();
			double rand2 = random_double();

			double theta = acos(sqrt(1 - rand1));
			double phi = pi2 * rand2;

			// 利用法线构建坐标系
			vec3 b1, b2;
			build_basis(normal, b1, b2);

			// 得到wi
			wi = normal * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta);

			double pdf = cos(theta) * pi_inv;

			return pdf;
		}
	}

	// 必须输入单位向量!!!!!!!!
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal, const vec3 &uv) const override {
		// F 菲涅尔项
		vec3 h = unit_vector(wi + wo);
		vec3 F = F0 + (vec3(1.0, 1.0, 1.0) - F0) * pow(1 - dot(wo, h), 5);

		// D NDF项
		double a2 = a * a;
		double D = a2 / (pi * pow(pow(dot(normal, h), 2) * (a2 - 1) + 1, 2));

		// G 几何项（考虑遮挡）
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
};


#endif
