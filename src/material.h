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

	// 获取图像纹理指针
	virtual shared_ptr<texture> get_color_map_ptr() const = 0;
};


// Blinn-Phong diffuse材质
class phong_material : public material {
public:
	vec3 radiance; // 自发光强度
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;
	double kd = 0.9; // 漫反射系数
	double ks = 0.1; // 镜面反射系数
	double a = 0; // 高光参数，越大则高光越小。默认a = 0，即漫反射材质

public:
	// 使用一个颜色来构造材质
	// 自动生成类型为simple_color_texture的纹理
	phong_material(vec3 albedo_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		radiance = radiance_init;
		color_map_ptr = make_shared<simple_color_texture>(albedo_init);
		normal_map_ptr = nullptr;
	}

	// 使用高光参数和一个颜色来构造材质
	// 自动生成类型为simple_color_texture的纹理
	phong_material(double a_init, vec3 albedo_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		radiance = radiance_init;
		color_map_ptr = make_shared<simple_color_texture>(albedo_init);
		a = a_init;
		normal_map_ptr = nullptr;
	}

	// 使用color_map来构造材质
	phong_material(shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr) {
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
	}

	// 使用高光参数和color_map来构造材质
	phong_material(double a_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr) {
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		a = a_init;
		normal_map_ptr = normal_map_ptr_init;
	}

	// www.cs.princeton.edu/courses/archive/fall08/cos526/assign3/lawrence.pdf
	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const override {
		
		// 混合cos-weighted采样和高光项重要性采样
		// 两种采样的频率的比值等于kd与ks之比
		double pdf;

		if (random_double() < kd / (kd + ks)) {

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

			pdf = cos(theta) * pi_inv;
		}
		else {

			// 高光项重要性采样
			double rand1 = random_double();
			double rand2 = random_double();

			// 随机采样半程向量
			double theta = acos(pow(rand1, 1.0 / (a + 1)));
			double phi = pi2 * rand2;
			
			vec3 b1, b2;
			build_basis(normal, b1, b2);

			vec3 h = normal * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta);
			
			// 根据镜面反射计算wi
			wi = 2 * h * dot(wo, h) - wo;

			// 如果光线穿透，返回一个很大的pdf使该结果无贡献
			if (dot(wi, normal) < 0) {
				return 1000000;
			}

			// 计算pdf
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


// GGX金属材质
class ggx_metal_material : public material {
public:
	double a; // 粗糙度，[0,1]
	vec3 radiance; // 自发光强度
	shared_ptr<texture> color_map_ptr = nullptr; // 菲涅尔项初始值（颜色）
	shared_ptr<texture> normal_map_ptr = nullptr;

public:
	// 用一种颜色构造material
	ggx_metal_material(const double &a_init, const vec3 &F0_init = vec3(1, 0.582, 0.0956), vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		a = a_init;
		color_map_ptr = make_shared<simple_color_texture>(F0_init);
		radiance = radiance_init;
	}

	// 用color_map构造material，同时也支持法线贴图
	ggx_metal_material(const double &a_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr) {
		
		a = a_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
	}

	// 必须输入单位向量!!!!!!!!
	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const override {

		// 重要性采样微表面法线，使用的分布为ggx NDF
		double rand1, rand2, theta, phi;
		vec3 h;

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

	// 必须输入单位向量!!!!!!!!
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal, const vec3 &uv) const override {
		// F 菲涅尔项
		vec3 h = unit_vector(wi + wo);
		vec3 F = color_map_ptr->get_value(uv) + (vec3(1.0, 1.0, 1.0) - color_map_ptr->get_value(uv)) * pow(1 - dot(wo, h), 5);

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

	virtual shared_ptr<texture> get_color_map_ptr() const override {
		return color_map_ptr;
	}
};

// GGX非金属材质
class ggx_nonmetal_material : public material {
public:
	double a; // 高光项粗糙度，[0,1]
	vec3 radiance; // 自发光强度
	double F0; // 菲涅尔系数，决定了高光项和漫反射项的比例
	shared_ptr<texture> color_map_ptr = nullptr; // 漫反射项颜色
	shared_ptr<texture> normal_map_ptr = nullptr;

public:
	// 用color_map构造material，同时也支持法线贴图
	ggx_nonmetal_material(const double &a_init, const double &F0_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr) {

		a = a_init;
		F0 = F0_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
	}

	// 接下来需要完成brdf和采样函数

	// 必须输入单位向量!!!!!!!!
	// 随机使用ggx重要性采样或漫反射重要性采样（cos-weighted采样）
	virtual double sample_wi(vec3 &wi, const vec3 &wo, const vec3 &normal) const override {

		double pdf;

		// 两种采样方法的权重根据菲涅尔项决定，因为它代表了高光项和漫反射项的能量比例
		if (random_double() < F0) {
			
			// 重要性采样微表面法线，使用的分布为ggx NDF
			double rand1, rand2, theta, phi;
			vec3 h;

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
			pdf = a2 * cos(theta) * pi_inv / pow(pow(cos(theta), 2) * (a2 - 1) + 1, 2) * 0.25 / dot(wo, h);
		}
		else {
			
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

			pdf = cos(theta) * pi_inv;
		}

		return pdf;
	}

	// 必须输入单位向量!!!!!!!!
	// brdf是漫反射项与高光项的和
	virtual vec3 brdf(const vec3 &wi, const vec3 &wo, const vec3 &normal, const vec3 &uv) const override {

		// 高光项（使用ggx模型）

		// F 菲涅尔项
		// 对于非金属，菲涅尔项的RGB值相等
		vec3 h = unit_vector(wi + wo);
		double F_value = F0 + (1.0 - F0) * pow(1 - dot(wo, h), 5);
		vec3 F(F_value, F_value, F_value);
		// D NDF项
		double a2 = a * a;
		double D = a2 / (pi * pow(pow(dot(normal, h), 2) * (a2 - 1) + 1, 2));
		// G 几何项（考虑遮挡）
		double dot_n_v = dot(normal, wo);
		double dot_n_l = dot(normal, wi);
		double k = a * 0.5;
		double G = dot_n_v / (dot_n_v * (1 - k) + k) * dot_n_l / (dot_n_l * (1 - k) + k);

		vec3 component_ggx = F * D * G / (4 * dot_n_v * dot_n_l);
		component_ggx = clamp(component_ggx, 0, 1);

		// 漫反射项
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
