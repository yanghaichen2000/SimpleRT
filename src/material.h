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
	virtual tuple<double, vec3, bool> sample_wi(const vec3& wo, const vec3& normali, bool wo_front) const = 0;

	// 采样入射点
	// 返回值为{pdf，入射点法线，入射点坐标}
	virtual tuple<double, vec3, vec3> sample_positioni(const vec3& normalo, const vec3& positiono, shared_ptr<hittable> world) const = 0;

	// 计算brdf项
	virtual vec3 bsdf(const vec3& wo, const vec3& normalo, const vec3& positiono, bool wo_front, const vec3& uv,
		const vec3& wi, const vec3& normali, const vec3& positioni, bool wi_front) const = 0;

	// 获取材质自发光强度
	virtual vec3 get_radiance() const = 0;

	// 获取法线贴图指针
	virtual shared_ptr<texture> get_normal_map_ptr() const = 0;

	// 获取图像纹理指针
	virtual shared_ptr<texture> get_color_map_ptr() const = 0;

	// 获取置换贴图指针
	virtual shared_ptr<texture> get_displacement_map_ptr() const = 0;

	// 获取介质
	// 可能返回空指针
	virtual shared_ptr<medium> get_medium_outside_ptr() const = 0;
	virtual shared_ptr<medium> get_medium_inside_ptr() const = 0;

	// 是否需要对光源采样
	virtual bool sample_light() const = 0;

	// 获取材质编号
	virtual int get_material_number() const = 0;
};


// Blinn-Phong 材质
class phong_material : public material {
public:
	vec3 radiance; // 自发光强度
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;
	shared_ptr<texture> displacement_map_ptr = nullptr;
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
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr, shared_ptr<texture> displacement_map_ptr_init = nullptr) {
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
		displacement_map_ptr = displacement_map_ptr_init;
	}

	// 使用高光参数和color_map来构造材质
	phong_material(double a_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr, shared_ptr<texture> displacement_map_ptr_init = nullptr) {
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		a = a_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
		displacement_map_ptr = displacement_map_ptr_init;
	}

	// www.cs.princeton.edu/courses/archive/fall08/cos526/assign3/lawrence.pdf
	virtual tuple<double, vec3, bool> sample_wi(const vec3& wo, const vec3& normali, bool wo_front) const {

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
		sample_positioni(const vec3& normalo, const vec3& positiono, shared_ptr<hittable> world) const {
		return make_tuple(1, normalo, positiono);
	}

	// phong brdf
	// zhuanlan.zhihu.com/p/500811555
	virtual vec3 bsdf(const vec3& wo, const vec3& normalo, const vec3& positiono, bool wo_front, const vec3& uv,
		const vec3& wi, const vec3& normali, const vec3& positioni, bool wi_front) const override {
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

	virtual shared_ptr<texture> get_displacement_map_ptr() const override {
		return displacement_map_ptr;
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

	virtual int get_material_number() const override {
		return 0;
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
	shared_ptr<texture> displacement_map_ptr = nullptr;

public:
	// 用一种颜色构造material
	ggx_metal_material(const double& a_init, const vec3& F0_init = vec3(1, 0.582, 0.0956), vec3 radiance_init = vec3(0.0, 0.0, 0.0)) {
		a = a_init;
		color_map_ptr = make_shared<simple_color_texture>(F0_init);
		radiance = radiance_init;
		medium_outside_ptr = default_medium_ptr;
		medium_inside_ptr = default_medium_ptr;
	}

	// 用color_map构造material，同时也支持法线贴图
	ggx_metal_material(const double& a_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr, shared_ptr<texture> displacement_map_ptr_init = nullptr) {

		a = a_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
		displacement_map_ptr = displacement_map_ptr_init;
	}

	// 必须输入单位向量!!!!!!!!
	virtual tuple<double, vec3, bool> sample_wi(const vec3& wo, const vec3& normali, bool wo_front) const override {

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
		sample_positioni(const vec3& normalo, const vec3& positiono, shared_ptr<hittable> world) const {
		return make_tuple(1, normalo, positiono);
	}

	// 必须输入单位向量!!!!!!!!
	virtual vec3 bsdf(const vec3& wo, const vec3& normalo, const vec3& positiono, bool wo_front, const vec3& uv,
		const vec3& wi, const vec3& normali, const vec3& positioni, bool wi_front) const override {
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

	virtual shared_ptr<texture> get_displacement_map_ptr() const override {
		return displacement_map_ptr;
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

	virtual int get_material_number() const override {
		return 1;
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
	shared_ptr<texture> displacement_map_ptr = nullptr;

public:
	// 用color_map构造material，同时也支持法线贴图
	ggx_nonmetal_material(const double& a_init, const double& F0_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr, shared_ptr<texture> displacement_map_ptr_init = nullptr) {

		a = a_init;
		F0 = F0_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
		displacement_map_ptr = displacement_map_ptr_init;
	}

	// 接下来需要完成brdf和采样函数

	// 必须输入单位向量!!!!!!!!
	// 随机使用ggx重要性采样或漫反射重要性采样（cos-weighted采样）
	virtual tuple<double, vec3, bool> sample_wi(const vec3& wo, const vec3& normali, bool wo_front) const override {

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
		sample_positioni(const vec3& normalo, const vec3& positiono, shared_ptr<hittable> world) const {
		return make_tuple(1, normalo, positiono);
	}

	// 必须输入单位向量!!!!!!!!
	// brdf是漫反射项与高光项的和
	virtual vec3 bsdf(const vec3& wo, const vec3& normalo, const vec3& positiono, bool wo_front, const vec3& uv,
		const vec3& wi, const vec3& normali, const vec3& positioni, bool wi_front) const override {

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

	virtual shared_ptr<texture> get_displacement_map_ptr() const override {
		return displacement_map_ptr;
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

	virtual int get_material_number() const override {
		return 2;
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
	shared_ptr<texture> displacement_map_ptr = nullptr;

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
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr, shared_ptr<texture> displacement_map_ptr_init = nullptr) {
		color_map_ptr = color_map_ptr_init;
		radiance = radiance_init;
		normal_map_ptr = normal_map_ptr_init;
		F0 = F0_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
		displacement_map_ptr = displacement_map_ptr_init;
	}

	virtual tuple<double, vec3, bool> sample_wi(const vec3& wo, const vec3& normali, bool wo_front) const override {

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
		sample_positioni(const vec3& normalo, const vec3& positiono, shared_ptr<hittable> world) const override {

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

	virtual vec3 bsdf(const vec3& wo, const vec3& normalo, const vec3& positiono, bool wo_front, const vec3& uv,
		const vec3& wi, const vec3& normali, const vec3& positioni, bool wi_front) const override {

		// 出射折射菲涅尔项
		double F_value_o = F0 + (1.0 - F0) * pow(1 - std::max(dot(wo, normalo), 0.0), 5);
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

	virtual shared_ptr<texture> get_displacement_map_ptr() const override {
		return displacement_map_ptr;
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

	virtual int get_material_number() const override {
		return 3;
	}
};


// ggx玻璃材质
class ggx_translucent_material : public material {
public:
	double a = 0.001; // 表面粗糙度
	double m = 200; // 高光项系数，用于调整bsdf的能量聚集度
	vec3 radiance; // 自发光强度
	shared_ptr<medium> medium_outside_ptr = default_medium_ptr;
	shared_ptr<medium> medium_inside_ptr = default_medium_ptr;
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;
	shared_ptr<texture> displacement_map_ptr = nullptr;

public:

	ggx_translucent_material(double a_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr, shared_ptr<texture> displacement_map_ptr_init = nullptr) {
		a = a_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
		displacement_map_ptr = displacement_map_ptr_init;
	}

	virtual tuple<double, vec3, bool> sample_wi(const vec3& wo, const vec3& normali, bool wo_front) const override {
		vec3 wi;

		// 计算菲涅尔加权系数
		//double F_value = F0 + (1.0 - F0) * pow(1 - dot(wo, normali), 5);
		double F_value;
		double n_o = wo_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
		double n_i = wo_front ? get_medium_inside_ptr()->n : get_medium_outside_ptr()->n;
		double cos_theta_o = dot(wo, normali);
		double cos_theta_i_2 = 1 - pow(n_o / n_i, 2) * (1 - pow(cos_theta_o, 2));
		if (cos_theta_i_2 < 0) {
			F_value = 1;
		}
		else {
			double cos_theta_i = sqrt(cos_theta_i_2);
			double Rs_value_1 = n_o * cos_theta_o;
			double Rs_value_2 = n_i * cos_theta_i;
			double Rs = pow((Rs_value_1 - Rs_value_2) / (Rs_value_1 + Rs_value_2), 2);
			double Rp_value_1 = n_o * cos_theta_i;
			double Rp_value_2 = n_i * cos_theta_o;
			double Rp = pow((Rp_value_1 - Rp_value_2) / (Rp_value_1 + Rp_value_2), 2);
			F_value = 0.5 * (Rs + Rp);
		}

		if (random_double() < F_value) { //TODO

			// 根据镜面反射计算wi
			wi = 2 * normali * dot(wo, normali) - wo;

			double pdf = (m + 1) * pi2_inv * dot(wi, normali);
			return make_tuple(pdf, wi, wo_front);
		}
		else {
			// 根据折射计算wi

			// 获得折射角sin值
			double sin_theta_i = sqrt(1 - cos_theta_i_2);
			// 处理不存在折射光的情况
			if (sin_theta_i > 1) {
				//return make_tuple(10000000, normali, wo_front);
				// 根据镜面反射计算wi
				wi = 2 * normali * dot(wo, normali) - wo;

				double pdf = (m + 1) * pi2_inv * dot(wi, normali);
				return make_tuple(pdf, wi, wo_front);
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

		if (wi_front == wo_front) { // 如果wi和wo同侧，则根据反射计算

			// 此时菲涅尔项为1
			// D NDF项
			double a2 = a * a;
			vec3 h = unit_vector(wo + wi);
			double D = a2 / (pi * pow(pow(dot(normalo, h), 2) * (a2 - 1) + 1, 2));

			// G 几何项（考虑遮挡）
			double dot_n_v = dot(normalo, wo);
			double dot_n_l = dot(normalo, wi);
			double k = a * 0.5;
			double G = dot_n_v / (dot_n_v * (1 - k) + k) * dot_n_l / (dot_n_l * (1 - k) + k);
			double bsdf_value = 0.0001 * D * G / (4 * dot_n_v * dot_n_l);
			return vec3(bsdf_value, bsdf_value, bsdf_value);
		}
		else { // 如果wi和wo不同侧，则根据折射计算
			// 先根据wi恢复出wo
			vec3 wo_re;

			double n_i = wi_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
			double n_o = wo_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
			double sin_theta_o = n_i / n_o * sqrt(1 - pow(dot(wi, normali), 2));

			if (sin_theta_o > 1) {
				return vec3(0, 0, 0); // 如果不存在折射，bsdf为0
			}
			double theta_o = asin(sin_theta_o);
			vec3 tan_vec = -unit_vector(wi - dot(normali, wi) * normali);
			wo_re = tan_vec * sin(theta_o) + normali * cos(theta_o);
			vec3 wi_re = -tan_vec * sin(theta_o) + normali * cos(theta_o);

			// 计算bsdf
			// D NDF项
			double a2 = a * a;
			vec3 h = unit_vector(wo + wi_re);
			double D = a2 / (pi * pow(pow(dot(normalo, h), 2) * (a2 - 1) + 1, 2));
			// G 几何项（考虑遮挡）
			double dot_n_v = dot(normalo, wo);
			double dot_n_l = dot(normalo, wi_re);
			double k = a * 0.5;
			double G = dot_n_v / (dot_n_v * (1 - k) + k) * dot_n_l / (dot_n_l * (1 - k) + k);
			double bsdf_value = 0.0001 * D * G / (4 * dot_n_v * dot_n_l);

			// 计算能量补偿倍率（因为入射光和折射光方向取值范围不同）
			double arcsin_max = std::min(n_i / n_o, n_o / n_i);
			if (n_o < n_i) {
				bsdf_value /= 1 - sqrt(1 - pow(arcsin_max, 2));
			}
			else {
				bsdf_value *= 1 - sqrt(1 - pow(arcsin_max, 2));
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
		return true;
	}

	virtual int get_material_number() const override {
		return 5;
	}
};


// 玻璃材质
class translucent_material : public material {
public:
	double m = 2000; // 高光项系数，用于调整bsdf的能量聚集度
	vec3 radiance; // 自发光强度
	shared_ptr<medium> medium_outside_ptr = default_medium_ptr;
	shared_ptr<medium> medium_inside_ptr = default_medium_ptr;
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;
	shared_ptr<texture> displacement_map_ptr = nullptr;

public:

	translucent_material(shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr, shared_ptr<texture> displacement_map_ptr_init = nullptr) {
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
		displacement_map_ptr = displacement_map_ptr_init;
	}

	virtual tuple<double, vec3, bool> sample_wi(const vec3& wo, const vec3& normali, bool wo_front) const override {
		vec3 wi;

		// 计算菲涅尔加权系数
		//double F_value = F0 + (1.0 - F0) * pow(1 - dot(wo, normali), 5);
		double F_value;
		double n_o = wo_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
		double n_i = wo_front ? get_medium_inside_ptr()->n : get_medium_outside_ptr()->n;
		double cos_theta_o = dot(wo, normali);
		double cos_theta_i_2 = 1 - pow(n_o / n_i, 2) * (1 - pow(cos_theta_o, 2));
		if (cos_theta_i_2 < 0) {
			F_value = 1;
		}
		else {
			double cos_theta_i = sqrt(cos_theta_i_2);
			double Rs_value_1 = n_o * cos_theta_o;
			double Rs_value_2 = n_i * cos_theta_i;
			double Rs = pow((Rs_value_1 - Rs_value_2) / (Rs_value_1 + Rs_value_2), 2);
			double Rp_value_1 = n_o * cos_theta_i;
			double Rp_value_2 = n_i * cos_theta_o;
			double Rp = pow((Rp_value_1 - Rp_value_2) / (Rp_value_1 + Rp_value_2), 2);
			F_value = 0.5 * (Rs + Rp);
		}

		if (random_double() < F_value) { //TODO

			// 根据镜面反射计算wi
			wi = 2 * normali * dot(wo, normali) - wo;

			double pdf = (m + 1) * pi2_inv * dot(wi, normali);
			return make_tuple(pdf, wi, wo_front);
		}
		else {
			// 根据折射计算wi

			// 获得折射角sin值
			double sin_theta_i = sqrt(1 - cos_theta_i_2);
			// 处理不存在折射光的情况
			if (sin_theta_i > 1) {
				//return make_tuple(10000000, normali, wo_front);
				// 根据镜面反射计算wi
				wi = 2 * normali * dot(wo, normali) - wo;

				double pdf = (m + 1) * pi2_inv * dot(wi, normali);
				return make_tuple(pdf, wi, wo_front);
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


		if (wi_front == wo_front) { // 如果wi和wo同侧，则根据反射计算
			//wo_re = (2.0 * dot(normali, wi) * normali - wo);
			double bsdf_value = (m + 1) * pi2_inv * pow(dot(normali, unit_vector(wi + wo)), m);
			return vec3(bsdf_value, bsdf_value, bsdf_value);
		}
		else { // 如果wi和wo不同侧，则根据折射计算
			// 先根据wi恢复出wo
			vec3 wo_re;

			double n_i = wi_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
			double n_o = wo_front ? get_medium_outside_ptr()->n : get_medium_inside_ptr()->n;
			double sin_theta_o = n_i / n_o * sqrt(1 - pow(dot(wi, normali), 2));

			if (sin_theta_o > 1) {
				return vec3(0, 0, 0); // 如果不存在折射，bsdf为0
			}
			double theta_o = asin(sin_theta_o);
			vec3 tan_vec = -unit_vector(wi - dot(normali, wi) * normali);
			wo_re = tan_vec * sin(theta_o) + normali * cos(theta_o);
			vec3 wi_re = -tan_vec * sin(theta_o) + normali * cos(theta_o);

			double bsdf_value = (m + 1) * pi2_inv * pow(dot(normali, unit_vector(wi_re + wo)), m);

			// 计算能量补偿项（因为入射光和折射光方向取值范围不同）
			double arcsin_max = std::min(n_i / n_o, n_o / n_i);
			if (n_o < n_i) {
				bsdf_value /= 1 - sqrt(1 - pow(arcsin_max, 2));
			}
			else {
				bsdf_value *= 1 - sqrt(1 - pow(arcsin_max, 2));
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

	virtual shared_ptr<texture> get_displacement_map_ptr() const override {
		return displacement_map_ptr;
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

	virtual int get_material_number() const override {
		return 5;
	}
};


struct disney_brdf_property {
	double metallic = 0;
	double subsurface = 0;
	double specular = 0.5;
	double roughness = 0.5;
	double specularTint = 0;
	double anisotropic = 0;
	double sheen = 0;
	double sheenTint = 0.5;
	double clearcoat = 0;
	double clearcoatGloss = 1;
};

// disney brdf
class disney_material : public material {

public:

	disney_brdf_property property; // brdf参数
	vec3 radiance; // 自发光强度
	shared_ptr<medium> medium_outside_ptr = default_medium_ptr;
	shared_ptr<medium> medium_inside_ptr = default_medium_ptr;
	shared_ptr<texture> color_map_ptr = nullptr;
	shared_ptr<texture> normal_map_ptr = nullptr;
	shared_ptr<texture> displacement_map_ptr = nullptr;

public:

	disney_material(disney_brdf_property property_init, shared_ptr<texture> color_map_ptr_init, vec3 radiance_init = vec3(0.0, 0.0, 0.0), shared_ptr<texture> normal_map_ptr_init = nullptr,
		shared_ptr<medium> medium_outside_ptr_init = nullptr, shared_ptr<medium> medium_inside_ptr_init = nullptr, shared_ptr<texture> displacement_map_ptr_init = nullptr) {
		property = property_init;
		radiance = radiance_init;
		color_map_ptr = color_map_ptr_init;
		normal_map_ptr = normal_map_ptr_init;
		medium_outside_ptr = medium_outside_ptr_init ? medium_outside_ptr_init : default_medium_ptr;
		medium_inside_ptr = medium_inside_ptr_init ? medium_inside_ptr_init : default_medium_ptr;
		displacement_map_ptr = displacement_map_ptr_init;
	}

	virtual tuple<double, vec3, bool> sample_wi(const vec3& wo, const vec3& normali, bool wo_front) const override {

		vec3 wi;
		double pdf;

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

		return make_tuple(pdf, wi, wo_front);
	}

	virtual tuple<double, vec3, vec3>
		sample_positioni(const vec3& normalo, const vec3& positiono, shared_ptr<hittable> world) const override {

		return make_tuple(1, normalo, positiono);
	}

	virtual vec3 bsdf(const vec3& wo, const vec3& normalo, const vec3& positiono, bool wo_front, const vec3& uv,
		const vec3& wi, const vec3& normali, const vec3& positioni, bool wi_front) const override {
		
		vec3 X = normalo;
		vec3 Y = normalo;

		double NdotL = dot(normalo, wi);
		double NdotV = dot(normalo, wo);
		if (NdotL < 0 || NdotV < 0) return vec3(0, 0, 0);

		vec3 H = unit_vector(wi + wo);
		double NdotH = dot(normalo, H);
		double LdotH = dot(wi, H);

		vec3 Cdlin = vec3(1, 1, 1);
		double Cdlum = .3 * Cdlin[0] + .6 * Cdlin[1] + .1 * Cdlin[2]; // luminance approx.

		vec3 Ctint = Cdlum > 0 ? Cdlin / Cdlum : vec3(1, 1, 1); // normalize lum. to isolate hue+sat
		vec3 Cspec0 = mix(property.specular * .08 * mix(vec3(1, 1, 1), Ctint, property.specularTint), Cdlin, property.metallic);
		vec3 Csheen = mix(vec3(1, 1, 1), Ctint, property.sheenTint);

		// Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
		// and mix in diffuse retro-reflection based on roughness
		double FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
		double Fd90 = 0.5 + 2 * LdotH * LdotH * property.roughness;
		double Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

		// Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
		// 1.25 scale is used to (roughly) preserve albedo
		// Fss90 used to "flatten" retroreflection based on roughness
		double Fss90 = LdotH * LdotH * property.roughness;
		double Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
		double ss = 1.25 * (Fss * (1 / (NdotL + NdotV) - .5) + .5);

		// specular
		double aspect = sqrt(1 - property.anisotropic * .9);
		double ax = std::max(.001, sqr(property.roughness) / aspect);
		double ay = std::max(.001, sqr(property.roughness) * aspect);
		double Ds = GTR2_aniso(NdotH, dot(H, X), dot(H, Y), ax, ay);
		double FH = SchlickFresnel(LdotH);
		vec3 Fs = mix(Cspec0, vec3(1, 1, 1), FH);
		double Gs;
		Gs = smithG_GGX_aniso(NdotL, dot(wi, X), dot(wi, Y), ax, ay);
		Gs *= smithG_GGX_aniso(NdotV, dot(wo, X), dot(wo, Y), ax, ay);

		// sheen
		vec3 Fsheen = FH * property.sheen * Csheen;

		// clearcoat (ior = 1.5 -> F0 = 0.04)
		double Dr = GTR1(NdotH, mix(.1, .001, property.clearcoatGloss));
		double Fr = mix(.04, 1.0, FH);
		double Gr = smithG_GGX(NdotL, .25) * smithG_GGX(NdotV, .25);

		return ((1 / pi) * mix(Fd, ss, property.subsurface) * Cdlin + Fsheen)
			* (1 - property.metallic)
			+ vec3(Gs * Fs * Ds) + vec3(.25 * property.clearcoat * Gr * Fr * Dr);
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

	virtual shared_ptr<texture> get_displacement_map_ptr() const override {
		return displacement_map_ptr;
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

	virtual int get_material_number() const override {
		return 6;
	}
};

template <typename material_name>
constexpr int get_material_number() {
	if (std::is_same_v<material_name, phong_material>) return 0;
	if (std::is_same_v<material_name, ggx_metal_material>) return 1;
	if (std::is_same_v<material_name, ggx_nonmetal_material>) return 2;
	if (std::is_same_v<material_name, sss_material>) return 3;
	if (std::is_same_v<material_name, ggx_translucent_material>) return 4;
	if (std::is_same_v<material_name, translucent_material>) return 5;
	if (std::is_same_v<material_name, disney_material>) return 6;
}


#endif