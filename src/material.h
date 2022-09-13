#pragma once
#ifndef MATERIAL_H
#define MATERIAL_H

#include "global.h"
#include "texture.h"
#include "hittable.h"

struct hit_record;
using std::tuple;
using std::make_tuple;
using std::tie;

class material {
public:
	// 采样入射角
	// 返回值为{pdf，入射角，入射方向是否和法线同向}
	virtual tuple<double, vec3, bool> sample_wi(const vec3 &wo, const vec3 &normali, bool wo_front) const = 0;
	
	// 采样入射点
	// 返回值为{pdf，入射点法线，入射点坐标}
	virtual tuple<double, vec3, vec3> sample_positioni(const vec3 &normalo, const vec3 &positiono, shared_ptr<hittable> world) const = 0;

	// 计算brdf项
	virtual vec3 bsdf(const vec3 &wo, const vec3 &normalo, const vec3 &positiono, bool wo_front, const vec3 &uv,
		const vec3 &wi, const vec3 &normali, const vec3 &positioni, bool wi_front) const = 0;
	
	// 获取材质自发光强度
	virtual vec3 get_radiance() const = 0;

	// 获取法线贴图指针
	virtual shared_ptr<texture> get_normal_map_ptr() const = 0;

	// 获取图像纹理指针
	virtual shared_ptr<texture> get_color_map_ptr() const = 0;

	// 获取介质
	// 可能返回空指针
	virtual shared_ptr<medium> get_medium_outside_ptr() const = 0;
	virtual shared_ptr<medium> get_medium_inside_ptr() const = 0;

	// 是否需要对光源采样
	virtual bool sample_light() const = 0;
};


// Blinn-Phong 材质
class phong_material : public material {
public:
	vec3 radiance; // 自发光强度
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;
	shared_ptr<medium> medium_outside_ptr = default_medium_ptr;
	shared_ptr<medium> medium_inside_ptr = default_medium_ptr;
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
		medium_outside_ptr = default_medium_ptr;
		medium_inside_ptr = default_medium_ptr;
	}

	// 使用高光参数和一个颜色来构造材质
	// 自动生成类型为simple_color_texture的纹理
	phong_material(double a_init, vec3 albedo_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		radiance = radiance_init;
		color_map_ptr = make_shared<simple_color_texture>(albedo_init);
		a = a_init;
		normal_map_ptr = nullptr;
		medium_outside_ptr = default_medium_ptr;
		medium_inside_ptr = default_medium_ptr;
	}

	// 使用color_map来构造材质
	phong_material(shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr, 
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr) {
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
	}

	// 使用高光参数和color_map来构造材质
	phong_material(double a_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr) {
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		a = a_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
	}

	// www.cs.princeton.edu/courses/archive/fall08/cos526/assign3/lawrence.pdf
	virtual tuple<double, vec3, bool> sample_wi(const vec3 &wo, const vec3 &normali, bool wo_front) const {

		// 混合cos-weighted采样和高光项重要性采样
		// 两种采样的频率的比值等于kd与ks之比
		vec3 wi;
		double pdf;

		if (random_double() < kd / (kd + ks)) {

			// cos-weighted采样
			double rand1 = random_double();
			double rand2 = random_double();

			double theta = acos(sqrt(1 - rand1));
			double phi = pi2 * rand2;

			// 利用法线构建坐标系
			vec3 b1, b2;
			build_basis(normali, b1, b2);

			// 得到wi
			wi = normali * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta);

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
			build_basis(normali, b1, b2);

			vec3 h = normali * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta);
			
			// 根据镜面反射计算wi
			wi = 2 * h * dot(wo, h) - wo;

			if (dot(wi, normali) < 0) {
				// 如果光线穿透，返回一个很大的pdf使该结果无贡献
				pdf = 10000000;
			}
			else {
				// 计算pdf
				pdf = (a + 1) * pi2_inv * pow(cos(theta), a);
			}
		}

		return make_tuple(pdf, wi, wo_front);
		
	}

	virtual tuple<double, vec3, vec3>
		sample_positioni(const vec3 &normalo, const vec3 &positiono, shared_ptr<hittable> world) const {
		return make_tuple(1, normalo, positiono);
	}

	// phong brdf
	// zhuanlan.zhihu.com/p/500811555
	virtual vec3 bsdf(const vec3 &wo, const vec3 &normalo, const vec3 &positiono, bool wo_front, const vec3 &uv,
		const vec3 &wi, const vec3 &normali, const vec3 &positioni, bool wi_front) const override {
		//auto ret1 = color_map_ptr->get_value(uv) * pi_inv;
		vec3 ret_d = color_map_ptr->get_value(uv) * kd * pi_inv;
		vec3 ret_s = vec3(ks, ks, ks) * (a + 2) * pi2_inv * pow(std::max(0.0, dot(normalo, unit_vector(wi + wo))), a);

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

	virtual shared_ptr<medium> get_medium_outside_ptr() const override {
		return medium_outside_ptr;
	};

	virtual shared_ptr<medium> get_medium_inside_ptr() const override {
		return medium_inside_ptr;
	};

	virtual bool sample_light() const override {
		return true;
	}
};


// GGX金属材质
class ggx_metal_material : public material {
public:
	double a; // 粗糙度，[0,1]
	vec3 radiance; // 自发光强度
	shared_ptr<medium> medium_outside_ptr = default_medium_ptr;
	shared_ptr<medium> medium_inside_ptr = default_medium_ptr;
	shared_ptr<texture> color_map_ptr = nullptr; // 菲涅尔项初始值（颜色）
	shared_ptr<texture> normal_map_ptr = nullptr;

public:
	// 用一种颜色构造material
	ggx_metal_material(const double &a_init, const vec3 &F0_init = vec3(1, 0.582, 0.0956), vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		a = a_init;
		color_map_ptr = make_shared<simple_color_texture>(F0_init);
		radiance = radiance_init;
		medium_outside_ptr = default_medium_ptr;
		medium_inside_ptr = default_medium_ptr;
	}

	// 用color_map构造material，同时也支持法线贴图
	ggx_metal_material(const double &a_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr) {
		
		a = a_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
	}

	// 必须输入单位向量!!!!!!!!
	virtual tuple<double, vec3, bool> sample_wi(const vec3 &wo, const vec3 &normali, bool wo_front) const override {

		// 重要性采样微表面法线，使用的分布为ggx NDF
		double pdf;
		vec3 wi;

		double rand1, rand2, theta, phi;
		vec3 h;

		rand1 = random_double();
		rand2 = random_double();
		theta = atan(a * sqrt(rand1 / (1 - rand1))); // 微观法线与宏观法线的夹角
		phi = pi2 * rand2; // 微观法线在切平面的角度

		// 利用法线构建坐标系
		vec3 b1, b2;
		build_basis(normali, b1, b2);

		// 得到微观法线(即wi和wo的半程向量)
		h = normali * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta); // 弄错了一次，注意。

		// 根据镜面反射计算wi
		wi = 2 * h * dot(wo, h) - wo;

		// 如果光线穿透，返回一个很大的pdf使该结果无贡献
		if (dot(wi, normali) < 0) {
			pdf = 1000000;
		}
		else {
			// pdf由NDF推出
			double a2 = a * a;
			pdf = a2 * cos(theta) * pi_inv / pow(pow(cos(theta), 2) * (a2 - 1) + 1, 2) * 0.25 / dot(wo, h);
		}

		return make_tuple(pdf, wi, wo_front);
	}

	virtual tuple<double, vec3, vec3>
		sample_positioni(const vec3 &normalo, const vec3 &positiono, shared_ptr<hittable> world) const {
		return make_tuple(1, normalo, positiono);
	}

	// 必须输入单位向量!!!!!!!!
	virtual vec3 bsdf(const vec3 &wo, const vec3 &normalo, const vec3 &positiono, bool wo_front, const vec3 &uv,
		const vec3 &wi, const vec3 &normali, const vec3 &positioni, bool wi_front) const override {
		// F 菲涅尔项
		vec3 h = unit_vector(wi + wo);
		vec3 F = color_map_ptr->get_value(uv) + (vec3(1.0, 1.0, 1.0) - color_map_ptr->get_value(uv)) * pow(1 - dot(wo, h), 5);

		// D NDF项
		double a2 = a * a;
		double D = a2 / (pi * pow(pow(dot(normalo, h), 2) * (a2 - 1) + 1, 2));

		// G 几何项（考虑遮挡）
		double dot_n_v = dot(normalo, wo);
		double dot_n_l = dot(normalo, wi);
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

	virtual shared_ptr<medium> get_medium_outside_ptr() const override {
		return medium_outside_ptr;
	};

	virtual shared_ptr<medium> get_medium_inside_ptr() const override {
		return medium_inside_ptr;
	};

	virtual bool sample_light() const override {
		return true;
	}
};


// GGX非金属材质
class ggx_nonmetal_material : public material {
public:
	double a; // 高光项粗糙度，[0,1]
	vec3 radiance; // 自发光强度
	double F0; // 菲涅尔系数，决定了高光项和漫反射项的比例
	shared_ptr<medium> medium_outside_ptr = default_medium_ptr;
	shared_ptr<medium> medium_inside_ptr = default_medium_ptr;
	shared_ptr<texture> color_map_ptr = nullptr; // 漫反射项颜色
	shared_ptr<texture> normal_map_ptr = nullptr;

public:
	// 用color_map构造material，同时也支持法线贴图
	ggx_nonmetal_material(const double &a_init, const double &F0_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr) {

		a = a_init;
		F0 = F0_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
	}

	// 接下来需要完成brdf和采样函数

	// 必须输入单位向量!!!!!!!!
	// 随机使用ggx重要性采样或漫反射重要性采样（cos-weighted采样）
	virtual tuple<double, vec3, bool> sample_wi(const vec3 &wo, const vec3 &normali, bool wo_front) const override {

		double pdf;
		vec3 wi;

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
			build_basis(normali, b1, b2);

			// 得到微观法线(即wi和wo的半程向量)
			h = normali * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta); // 弄错了一次，注意。

			// 根据镜面反射计算wi
			wi = 2 * h * dot(wo, h) - wo;

			// 如果光线穿透，返回一个很大的pdf使该结果无贡献
			if (dot(wi, normali) < 0) {
				pdf = 1000000;
			}
			else {
				// pdf由NDF推出
				double a2 = a * a;
				pdf = a2 * cos(theta) * pi_inv / pow(pow(cos(theta), 2) * (a2 - 1) + 1, 2) * 0.25 / dot(wo, h);
			}
		}
		else {
			
			// cos-weighted采样
			double rand1 = random_double();
			double rand2 = random_double();

			double theta = acos(sqrt(1 - rand1));
			double phi = pi2 * rand2;

			// 利用法线构建坐标系
			vec3 b1, b2;
			build_basis(normali, b1, b2);

			// 得到wi
			wi = normali * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta);

			pdf = cos(theta) * pi_inv;
		}

		return make_tuple(pdf, wi, wo_front);
	}

	virtual tuple<double, vec3, vec3>
		sample_positioni(const vec3 &normalo, const vec3 &positiono, shared_ptr<hittable> world) const {
		return make_tuple(1, normalo, positiono);
	}

	// 必须输入单位向量!!!!!!!!
	// brdf是漫反射项与高光项的和
	virtual vec3 bsdf(const vec3 &wo, const vec3 &normalo, const vec3 &positiono, bool wo_front, const vec3 &uv,
		const vec3 &wi, const vec3 &normali, const vec3 &positioni, bool wi_front) const override {

		// 高光项（使用ggx模型）

		// F 菲涅尔项
		// 对于非金属，菲涅尔项的RGB值相等
		vec3 h = unit_vector(wi + wo);
		double F_value = F0 + (1.0 - F0) * pow(1 - dot(wo, h), 5);
		vec3 F(F_value, F_value, F_value);
		// D NDF项
		double a2 = a * a;
		double D = a2 / (pi * pow(pow(dot(normalo, h), 2) * (a2 - 1) + 1, 2));
		// G 几何项（考虑遮挡）
		double dot_n_v = dot(normalo, wo);
		double dot_n_l = dot(normalo, wi);
		double k = a * 0.5;
		double G = dot_n_v / (dot_n_v * (1 - k) + k) * dot_n_l / (dot_n_l * (1 - k) + k);

		vec3 component_ggx = F * D * G / (4 * dot_n_v * dot_n_l);
		component_ggx = clamp(component_ggx, 0, infinity);

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

	virtual shared_ptr<medium> get_medium_outside_ptr() const override {
		return medium_outside_ptr;
	};

	virtual shared_ptr<medium> get_medium_inside_ptr() const override {
		return medium_inside_ptr;
	};

	virtual bool sample_light() const override {
		return true;
	}
};


// 次表面散射材质
constexpr double V = 0.1;
class sss_material : public material {
public:
	vec3 radiance; // 自发光强度
	double F0;
	double v = V; // 控制散射程度，暂时不能改变值 TODO
	double v_inv = 1.0 / V; // 控制散射程度，暂时不能改变值 TODO
	double Rm = 10; // 圆盘采样半径，暂时不能改变值 TODO
	double Rm2 = 100; // 圆盘采样半径，暂时不能改变值 TODO
	shared_ptr<medium> medium_outside_ptr = default_medium_ptr;
	shared_ptr<medium> medium_inside_ptr = default_medium_ptr;
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;

public:
	// 使用一个颜色来构造材质
	// 自动生成类型为simple_color_texture的纹理
	sss_material(double F0_init, vec3 albedo_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		color_map_ptr = make_shared<simple_color_texture>(albedo_init);
		radiance = radiance_init;
		F0 = F0_init;
		medium_outside_ptr = default_medium_ptr;
		medium_inside_ptr = default_medium_ptr;
	}


	// 使用color_map来构造材质
	sss_material(double F0_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr) {
		color_map_ptr = color_map_ptr_init;
		radiance = radiance_init;
		normal_map_ptr = normal_map_ptr_init;
		F0 = F0_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
	}

	virtual tuple<double, vec3, bool> sample_wi(const vec3 &wo, const vec3 &normali, bool wo_front) const override {
		
		// cos-weighted采样
		double rand1 = random_double();
		double rand2 = random_double();

		double theta = acos(sqrt(1 - rand1));
		double phi = pi2 * rand2;

		// 利用法线构建坐标系
		vec3 b1, b2;
		build_basis(normali, b1, b2);

		// 得到wi
		vec3 wi = normali * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta);

		double pdf = cos(theta) * pi_inv;

		return make_tuple(pdf, wi, wo_front);
	}

	virtual tuple<double, vec3, vec3>
		sample_positioni(const vec3 &normalo, const vec3 &positiono, shared_ptr<hittable> world) const override {

		// 圆盘投影采样
		// 先在与wo垂直的圆盘上采样
		double rand1 = random_double();
		double rand2 = random_double();
		double r = sqrt(-2 * v * log(1 - rand1 * (1 - exp(-Rm2 * v_inv * 0.5))));
		double phi = pi2 * rand2;

		// 利用法线构建坐标系
		vec3 b1, b2;
		build_basis(normalo, b1, b2);

		// 获取投影前点的位置
		vec3 positioni_0 = positiono + b1 * r * cos(phi) + b2 * r * sin(phi);

		// 进行投影
		hit_record rec1, rec2;
		bool hit1 = world->hit(ray(positioni_0, normalo), 0.000001, infinity, rec1);
		bool hit2 = world->hit(ray(positioni_0, -normalo), 0.000001, infinity, rec2);
		vec3 positioni, normali;
		if (hit1) {
			if (hit2) {
				if (rec1.t < rec2.t) {
					positioni = rec1.p;
					normali = rec1.normal;
				}
				else {
					positioni = rec2.p;
					normali = rec2.normal;
				}
			}
			else {
				positioni = rec1.p;
				normali = rec1.normal;
			}
		}
		else {
			if (hit2) {
				positioni = rec2.p;
				normali = rec2.normal;
			}
			else {
				positioni = positioni_0;
				normali = normalo;
			}
		}

		// 计算pdf
		double pdf = exp(-r * r * v_inv * 0.5) * v_inv * pi2_inv / (1 - exp(-Rm2 * v_inv * 0.5)) * abs(dot(normali, normalo));

		return make_tuple(pdf, normali, positioni);
	}

	virtual vec3 bsdf(const vec3 &wo, const vec3 &normalo, const vec3 &positiono, bool wo_front, const vec3 &uv,
		const vec3 &wi, const vec3 &normali, const vec3 &positioni, bool wi_front) const override {

		// 出射折射菲涅尔项
		double F_value_o = F0 + (1.0 - F0) * pow(1 -std::max(dot(wo, normalo), 0.0), 5);
		vec3 F_o(1 - F_value_o, 1 - F_value_o, 1 - F_value_o);

		// 入射折射菲涅尔项
		double F_value_i = F0 + (1.0 - F0) * pow(1 - std::max(dot(wi, normali), 0.0), 5);
		vec3 F_i(1 - F_value_i, 1 - F_value_i, 1 - F_value_i);

		// Rd项
		// 这里使用的是二维高斯分布
		double r2 = (positiono - positioni).length_squared();
		double Rd = exp(-r2 * v_inv * 0.5) * v_inv * pi2_inv;

		//return pi_inv * Rd * F_o * F_i * color_map_ptr->get_value(uv);
		return pi_inv * Rd * color_map_ptr->get_value(uv);
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

	virtual shared_ptr<medium> get_medium_outside_ptr() const override {
		return medium_outside_ptr;
	};

	virtual shared_ptr<medium> get_medium_inside_ptr() const override {
		return medium_inside_ptr;
	};

	virtual bool sample_light() const override {
		return true;
	}
};


// ggx玻璃材质
class ggx_translucent_material : public material {
public:
	double F0; // 菲涅尔项
	double a; // 粗糙度
	vec3 radiance; // 自发光强度
	shared_ptr<medium> medium_outside_ptr = default_medium_ptr;
	shared_ptr<medium> medium_inside_ptr = default_medium_ptr;
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;

public:
	
	ggx_translucent_material(double a_init, double F0_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr) {
		a = a_init;
		F0 = F0_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
	}

	virtual tuple<double, vec3, bool> sample_wi(const vec3 &wo, const vec3 &normali, bool wo_front) const override {
		vec3 wi;
		double pdf;

		// 先采样法线
		// 重要性采样微表面法线，使用的分布为ggx NDF
		double rand1, rand2, theta, phi;
		vec3 h;

		rand1 = random_double();
		rand2 = random_double();
		theta = atan(a * sqrt(rand1 / (1 - rand1))); // 微观法线与宏观法线的夹角
		phi = pi2 * rand2; // 微观法线在切平面的角度

		// 利用法线构建坐标系
		vec3 b1, b2;
		build_basis(normali, b1, b2);

		// 得到微观法线(即wi和wo的半程向量)
		h = normali * cos(theta) + b1 * sin(phi) * sin(theta) + b2 * cos(phi) * sin(theta); // 弄错了一次，注意。

		// 计算菲涅尔加权系数
		double F_value = F0 + (1.0 - F0) * pow(1 - dot(wo, h), 5);

		if (random_double() < F_value) {

			// 根据镜面反射计算wi
			wi = 2 * h * dot(wo, h) - wo;

			// 如果光线穿透，返回一个很大的pdf使该结果无贡献
			if (dot(wi, normali) < 0) {
				pdf = 1000000;
			}
			else {
				// pdf由NDF推出
				double a2 = a * a;
				pdf = a2 * cos(theta) * pi_inv / pow(pow(cos(theta), 2) * (a2 - 1) + 1, 2) * 0.25 / dot(wo, h);
			}

			pdf = 999999999; // test

			return make_tuple(pdf * F_value, wi, wo_front);
		}
		else {

			// 根据折射计算wi

			// 获取折射率，两侧分别为内侧和外侧
			double n_o = wo_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
			double n_i = wo_front ? get_medium_inside_ptr()->n : get_medium_outside_ptr()->n;

			// 获得折射角sin值
			double sin_theta_i = n_o / n_i * sqrt(1 - pow(dot(wo, h), 2));
			// 处理不存在折射光的情况
			if (sin_theta_i > 1 or sin_theta_i < -1) {
				return make_tuple(10000000, h, wo_front);
			}
			// 计算折射角
			double theta_i = asin(sin_theta_i);

			// 切向单位向量（用于表示入射方向，注意与uv的切向含义不同）
			vec3 tan_vec = -unit_vector(wo - dot(wo, h) * h);
			// 计算出射方向
			vec3 wi = unit_vector(tan_vec * sin_theta_i - h * cos(theta_i));

			// pdf由NDF推出
			double a2 = a * a;
			pdf = a2 * cos(theta) * pi_inv / pow(pow(cos(theta), 2) * (a2 - 1) + 1, 2) * 0.25 / dot(wo, h);

#ifdef test_mode
			std::cout << "\nsample_wi()" << '\n';
			std::cout << "F_value = " << F_value << '\n';
			std::cout << acos(dot(h, normali)) * pi2_inv * 360.0 << '\n';
			std::cout << acos(dot(wo, normali)) * pi2_inv * 360.0 << ' ' << theta_i * pi2_inv * 360.0 << '\n';
			std::cout << n_o << ' ' << n_i << '\n';
			std::cout << "pdf = " << pdf << '\n';
#endif
			return make_tuple(pdf * (1 - F_value), wi, not wo_front);
		}

	}

	virtual tuple<double, vec3, vec3>
		sample_positioni(const vec3 &normalo, const vec3 &positiono, shared_ptr<hittable> world) const override {

		return make_tuple(1, normalo, positiono);
	}

	virtual vec3 bsdf(const vec3 &wo, const vec3 &normalo, const vec3 &positiono, bool wo_front, const vec3 &uv,
		const vec3 &wi, const vec3 &normali, const vec3 &positioni, bool wi_front) const override {

		// 如果入射角出射角位于同一侧，根据反射计算bsdf
		if (wo_front == wi_front) {
			
			// ggx模型
			
			// F 菲涅尔项
			// 对于非金属，菲涅尔项的RGB值相等
			vec3 h = unit_vector(wi + wo);
			double F_value = F0 + (1.0 - F0) * pow(1 - dot(wo, h), 5);
			vec3 F(F_value, F_value, F_value);
			// D NDF项
			double a2 = a * a;
			double D = a2 / (pi * pow(pow(dot(normalo, h), 2) * (a2 - 1) + 1, 2));
			// G 几何项（考虑遮挡）
			double dot_n_v = dot(normalo, wo);
			double dot_n_l = dot(normalo, wi);
			double k = a * 0.5;
			double G = dot_n_v / (dot_n_v * (1 - k) + k) * dot_n_l / (dot_n_l * (1 - k) + k);

			vec3 ans = F * D * G / (4 * dot_n_v * dot_n_l);
			return clamp(ans, 0, infinity);
		}
		else // 入射角出射角不同侧，根据折射计算bsdf
		{
			// 计算法线方向
			double n_o = wo_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
			double n_i = wi_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
			double theta_o = acos(abs(dot(wo, normalo)));
			double theta_i = acos(abs(dot(wi, normalo)));
			double theta_h = atan((n_o * sin(theta_o) - n_i * sin(theta_i)) / (n_i * cos(theta_i) - n_o * cos(theta_o)));

			// ggx模型

			// F 菲涅尔项
			// 对于非金属，菲涅尔项的RGB值相等
			double F_value = F0 + (1.0 - F0) * pow(1 - cos(theta_o + theta_h), 5);
			vec3 F(1 - F_value, 1 - F_value, 1 - F_value);
			// D NDF项
			double a2 = a * a;
			double D = a2 / (pi * pow(pow(cos(theta_h), 2) * (a2 - 1) + 1, 2));
			// G 几何项
			double dot_n_v = abs(dot(normalo, wo));
			double dot_n_l = abs(dot(normalo, wi));
			double k = a * 0.5;
			double G = dot_n_v / (dot_n_v * (1 - k) + k) * dot_n_l / (dot_n_l * (1 - k) + k);

			vec3 ans = F * D * G / (4 * dot_n_v * dot_n_l);

#ifdef test_mode
			std::cout << "\n bsdf()" << '\n';
			std::cout << theta_h * pi2_inv * 360.0 << '\n';
			std::cout << (theta_o + theta_h) * pi2_inv * 360.0 << ' ' << (theta_i + theta_h) * pi2_inv * 360.0 << '\n';
			std::cout << n_o << ' ' << n_i << '\n';
			std::cout << "bsdf = " << ans << '\n';
#endif

			return clamp(ans, 0, infinity);
		}
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

	virtual shared_ptr<medium> get_medium_outside_ptr() const override {
		return medium_outside_ptr;
	};

	virtual shared_ptr<medium> get_medium_inside_ptr() const override {
		return medium_inside_ptr;
	};

	virtual bool sample_light() const override {
		return true;
	}
};


// 玻璃材质
class translucent_material : public material {
public:
	double F0; // 菲涅尔项
	double m = 4; // 高光项系数，用于调整bsdf的能量聚集度
	vec3 radiance; // 自发光强度
	shared_ptr<medium> medium_outside_ptr = default_medium_ptr;
	shared_ptr<medium> medium_inside_ptr = default_medium_ptr;
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;

public:

	translucent_material(double F0_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr) {
		F0 = F0_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
	}

	virtual tuple<double, vec3, bool> sample_wi(const vec3& wo, const vec3& normali, bool wo_front) const override {
		vec3 wi;
		
		// 计算菲涅尔加权系数
		double F_value = F0 + (1.0 - F0) * pow(1 - dot(wo, normali), 5);

		if (random_double() < F_value) { //TODO

			// 根据镜面反射计算wi
			wi = 2 * normali * dot(wo, normali) - wo;

			double pdf = (m + 1) * pi2_inv * dot(wi, normali);
			return make_tuple(pdf, wi, wo_front);
		}
		else {

			// 根据折射计算wi

			// 获取折射率，两侧分别为内侧和外侧
			double n_o = wo_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
			double n_i = wo_front ? get_medium_inside_ptr()->n : get_medium_outside_ptr()->n;

			// 获得折射角sin值
			double sin_theta_i = n_o / n_i * sqrt(1 - pow(dot(wo, normali), 2));
			// 处理不存在折射光的情况
			if (sin_theta_i > 1) {
				return make_tuple(10000000, normali, wo_front);
			}
			// 计算折射角
			double theta_i = asin(sin_theta_i);

			// 切向单位向量（用于表示入射方向，注意与uv的切向含义不同）
			vec3 tan_vec = -unit_vector(wo - dot(wo, normali) * normali);
			// 计算出射方向
			vec3 wi = unit_vector(tan_vec * sin_theta_i - normali * cos(theta_i));

			double pdf = (m + 1) * pi2_inv * dot(wi, normali);
			return make_tuple(pdf, wi, not wo_front);
		}
	}

	virtual tuple<double, vec3, vec3>
		sample_positioni(const vec3& normalo, const vec3& positiono, shared_ptr<hittable> world) const override {

		return make_tuple(1, normalo, positiono);
	}

	virtual vec3 bsdf(const vec3& wo, const vec3& normalo, const vec3& positiono, bool wo_front, const vec3& uv,
		const vec3& wi, const vec3& normali, const vec3& positioni, bool wi_front) const override {

		// 先根据wi恢复出wo
		vec3 wo_re;
		if (wi_front == wo_front) { // 如果wi和wo同侧，则根据反射计算
			//wo_re = (2.0 * dot(normali, wi) * normali - wo);
			double bsdf_value = (m + 1) * pi2_inv * pow(dot(normali, unit_vector(wi + wo)), m);
			return vec3(bsdf_value, bsdf_value, bsdf_value);
		} 
		else { // 如果wi和wo不同侧，则根据折射计算
			if (sample_light_flag) {
				std::cout << "entered" << '\n';
			}
			double n_i = wi_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
			double n_o = wo_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
			double sin_theta_o = n_i / n_o * sqrt(1 - pow(dot(wi, normali), 2));

			if (sin_theta_o > 1) {
				return vec3(0, 0, 0); // 如果不存在折射，bsdf为0
			}
			double theta_o = asin(sin_theta_o);
			vec3 tan_vec = - unit_vector(wi - dot(normali, wi) * normali);
			wo_re = tan_vec * sin(theta_o) + normali * cos(theta_o);
			vec3 wi_re = -tan_vec * sin(theta_o) + normali * cos(theta_o);

			//double bsdf_value = (m + 1) * pi2_inv * pow(dot(wo, wo_re), m);
			double bsdf_value = (m + 1) * pi2_inv * pow(dot(normali, unit_vector(wi_re + wo)), m);
			if (sample_light_flag) {
				std::cout << bsdf_value << '\n';
			}
			return vec3(bsdf_value, bsdf_value, bsdf_value);
		}
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

	virtual shared_ptr<medium> get_medium_outside_ptr() const override {
		return medium_outside_ptr;
	};

	virtual shared_ptr<medium> get_medium_inside_ptr() const override {
		return medium_inside_ptr;
	};

	virtual bool sample_light() const override {
		return false;
	}
};


#endif
