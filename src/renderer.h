#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include <fstream>
#include <iostream>
#include <mutex>
#include "global.h"
#include "color.h"
#include "sphere.h"
#include "triangle.h"
#include "hittable_list.h"
#include "camera.h"
#include "material.h"
#include "light.h"

using std::mutex;

const double P_RR = 1; // 俄罗斯轮盘赌概率


// 光线追踪函数
// 深度表示剩余可反弹次数
color ray_color(const ray& r, const hittable& world, int depth) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	if (world.hit(r, 0, infinity, rec)) {
		// 确定是否需要反弹
		bool scatter = random_double() < P_RR;
		//std::cout << "test\n";
		if (scatter)
		{
			// 随机采样入射光方向
			vec3 normal = unit_vector(rec.normal);
			vec3 wo = unit_vector(-r.direction());
			vec3 wi;
			double pdf = rec.mat_ptr->sample_wi(wi, wo, normal);
			vec3 brdf = rec.mat_ptr->brdf(wi, wo, normal, rec.uv);
			vec3 radiance = rec.mat_ptr->get_radiance();
			ray new_ray(rec.p + wi * 0.0001, wi);
			vec3 radiance_out = radiance + ray_color(new_ray, world, depth - 1) * brdf * dot(normal, wi) / pdf / P_RR;
			return radiance_out;
		}
		else
		{
			// 如果不反弹，直接返回该材质的radiance
			return rec.mat_ptr->get_radiance();
		}
	}

	// 如果什么都没打中，则返回环境光
	vec3 unit_direction = unit_vector(r.direction());
	auto t = 0.5*(unit_direction.y() + 1.0);
	//return (1.0 - t)*color(0.309, 0.737, 1) + t * color(0.309, 0.737, 1);
	return (1.0 - t)*color(0, 0, 0) + t * color(0, 0, 0);
}


// 光线追踪函数
// 深度表示剩余可反弹次数
// 增加对光源采样功能
color ray_color(const ray& r, const hittable& world, int depth, vector<shared_ptr<light>> light_ptr_list) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	if (world.hit(r, 0, infinity, rec)) {
		// 计算基本信息
		vec3 normal = unit_vector(rec.normal); // shading point法线
		vec3 wo = unit_vector(-r.direction()); // 出射方向

		// 计算直接光
		vec3 radiance_direct = vec3(0, 0, 0);

		for (shared_ptr<light> light_ptr : light_ptr_list)
		{
			// 采样光源
			vec3 position_hit = rec.p; // shading point坐标
			vec3 position_light; // 光源采样点坐标
			vec3 radiance_light; // 光源采样点radiance
			vec3 normal_light; // 光源采样点法线
			double pdf_light = light_ptr->sample_p(position_light, radiance_light, normal_light); // 光源采样点pdf
			vec3 wi_light = unit_vector(position_light - position_hit); // 入射方向（到光源采样点）

			// 计算
			ray ray_to_light(position_hit, wi_light);
			hit_record rec_check_block;
			if (world.hit(ray_to_light, 0.000001, infinity, rec_check_block) and rec_check_block.t < (position_light - position_hit).length())
			{
				radiance_direct += vec3(0, 0, 0); // 有遮挡物则直接光为0
			}
			else
			{
				double distance_light_square = (position_light - position_hit).length_squared(); // shading point到光源采样点距离的平方
				vec3 brdf_light = rec.mat_ptr->brdf(wi_light, wo, normal, rec.uv);
				vec3 radiance_direct_delta = radiance_light * brdf_light * dot(normal, wi_light) * dot(normal_light, -wi_light) / (distance_light_square * pdf_light); // 直接光
				radiance_direct += clamp(radiance_direct_delta, 0, std::numeric_limits<double>::infinity()); // 使radiance非负（解决从光源反向射出的问题）
			}
		}

		// 确定是否需要反弹
		bool scatter = random_double() < P_RR;
		if (scatter)
		{
			// 随机采样入射光方向获取间接光照
			vec3 wi; // 入射方向（随机采样）
			double pdf = rec.mat_ptr->sample_wi(wi, wo, normal);
			vec3 brdf = rec.mat_ptr->brdf(wi, wo, normal, rec.uv);
			ray new_ray(rec.p + wi * 0.0001, wi);
			vec3 radiance_indirect = ray_color(new_ray, world, depth - 1, light_ptr_list) * brdf * dot(normal, wi) / pdf / P_RR;
			radiance_indirect = clamp(radiance_indirect, 0, std::numeric_limits<double>::infinity());

			// 得到最终radiance
			vec3 radiance = rec.mat_ptr->get_radiance();
			return radiance + radiance_direct + radiance_indirect;
		}
		else
		{
			// 得到最终radiance
			vec3 radiance = rec.mat_ptr->get_radiance();
			return radiance + radiance_direct;
		}
	}

	// 如果什么都没打中，则返回环境光
	return vec3(0.1, 0.1, 0.1);
}

// 光线追踪函数
// 深度表示剩余可反弹次数
// 增加对光源采样功能
// 使用bvh
color ray_color(const ray& r, shared_ptr<hittable> &bvh_root, int depth, vector<shared_ptr<light>> light_ptr_list) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	if (bvh_root->hit(r, 0, infinity, rec)) {
		// 计算基本信息
		vec3 normal = unit_vector(rec.normal); // shading point法线
		vec3 wo = unit_vector(-r.direction()); // 出射方向

		// 计算直接光
		vec3 radiance_direct = vec3(0, 0, 0);

		for (shared_ptr<light> light_ptr : light_ptr_list)
		{
			// 采样光源
			vec3 position_hit = rec.p; // shading point坐标
			vec3 position_light; // 光源采样点坐标
			vec3 radiance_light; // 光源采样点radiance
			vec3 normal_light; // 光源采样点法线
			double pdf_light = light_ptr->sample_p(position_light, radiance_light, normal_light); // 光源采样点pdf
			vec3 wi_light = unit_vector(position_light - position_hit); // 入射方向（到光源采样点）

			// 计算
			ray ray_to_light(position_hit, wi_light);
			hit_record rec_check_block;
			if (bvh_root->hit(ray_to_light, 0.000001, infinity, rec_check_block) and rec_check_block.t < (position_light - position_hit).length())
			{
				radiance_direct += vec3(0, 0, 0); // 有遮挡物则直接光为0
			}
			else
			{
				double distance_light_square = (position_light - position_hit).length_squared(); // shading point到光源采样点距离的平方
				vec3 brdf_light = rec.mat_ptr->brdf(wi_light, wo, normal, rec.uv);
				vec3 radiance_direct_delta = radiance_light * brdf_light * dot(normal, wi_light) * dot(normal_light, -wi_light) / (distance_light_square * pdf_light); // 直接光
				radiance_direct += clamp(radiance_direct_delta, 0, std::numeric_limits<double>::infinity()); // 使radiance非负（解决从光源反向射出的问题）
			}
		}

		// 确定是否需要反弹，如果反弹则计算间接光
		bool scatter = random_double() < P_RR;
		if (scatter)
		{
			// 随机采样入射光方向获取间接光照
			vec3 wi; // 入射方向（随机采样）
			double pdf = rec.mat_ptr->sample_wi(wi, wo, normal);
			vec3 brdf = rec.mat_ptr->brdf(wi, wo, normal, rec.uv);
			ray new_ray(rec.p + wi * 0.0001, wi);
			vec3 radiance_indirect = ray_color(new_ray, bvh_root, depth - 1, light_ptr_list) * brdf * dot(normal, wi) / pdf / P_RR;
			radiance_indirect = clamp(radiance_indirect, 0, std::numeric_limits<double>::infinity());

			// 得到最终radiance
			vec3 radiance = rec.mat_ptr->get_radiance();
			return radiance + radiance_direct + radiance_indirect;
		}
		else
		{
			// 得到最终radiance
			vec3 radiance = rec.mat_ptr->get_radiance();
			return radiance + radiance_direct;
		}
	}

	// 如果什么都没打中，则返回环境光
	return vec3(0.1, 0.1, 0.1);
}


// 将颜色写入framebuffer中
// 像素将一行一行写入图像，从左向右，从上往下
// 这里j反向迭代是因为写入时u逐渐减小（在世界坐标中,u和y同向，v和x同向）
// i在迭代时每次的增加值可以不为1,通过step参数设置
// bias参数为i迭代的起点（包含）
void render(const int &image_height, const int &image_width, const int &samples_per_pixel, const int &max_depth,
	const hittable_list &world, const camera &cam, vector<vec3> &framebuffer, const int &step, const int &bias)
{
	for (int j = image_height - 1; j >= 0; --j) {
		std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush; // 进度显示
		for (int i = bias; i < image_width; i += step) {
			color pixel_color(0, 0, 0);
			// 多次采样计算平均值
			// 同时这么做也使得ray的方向的期望近似指向像素中心
			for (int s = 0; s < samples_per_pixel; ++s) {
				auto u = (i + random_double()) / (image_width - 1);
				auto v = (j + random_double()) / (image_height - 1);
				ray r = cam.get_ray(u, v);
				pixel_color += clamp(ray_color(r, world, max_depth), 0, 10000);
			}
			write_color_to_framebuffer(framebuffer, pixel_color, samples_per_pixel, (image_height - 1 - j) * image_width + i);
		}
	}
}


// render()的指针版本，支持多线程
// 其中world和framebuffer采用指针传递，其他参数采用值传递
mutex framebuffer_mutex;
void render_raw(int image_height, int image_width, int samples_per_pixel, int max_depth,
	hittable_list world, camera cam, vector<vec3> *framebuffer, vector<shared_ptr<light>> light_ptr_list, int step, int bias)
{
	for (int j = image_height - 1; j >= 0; --j) {
		std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush; // 进度显示
		for (int i = bias; i < image_width; i += step) {
			color pixel_color(0, 0, 0);
			// 多次采样计算平均值
			// 同时这么做也使得ray的方向的期望近似指向像素中心
			for (int s = 0; s < samples_per_pixel; ++s) {
				auto u = (i + random_double()) / (image_width - 1);
				auto v = (j + random_double()) / (image_height - 1);
				ray r = cam.get_ray(u, v);
				//pixel_color += ray_color(r, world, max_depth);
				//pixel_color += ray_color(r, world, max_depth, light_ptr);
				pixel_color += clamp(ray_color(r, world, max_depth, light_ptr_list), 0, 1);
			}
			// 不知道上锁有没有用
			framebuffer_mutex.lock();
			write_color_to_framebuffer(framebuffer, pixel_color, samples_per_pixel, (image_height - 1 - j) * image_width + i);
			framebuffer_mutex.unlock();
		}
	}
}

void render_bvh(int image_height, int image_width, int samples_per_pixel, int max_depth,
	shared_ptr<hittable> bvh_root, camera cam, vector<vec3> *framebuffer, vector<shared_ptr<light>> light_ptr_list, int step, int bias)
{
	for (int j = image_height - 1; j >= 0; --j) {
		std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush; // 进度显示
		for (int i = bias; i < image_width; i += step) {
			color pixel_color(0, 0, 0);
			// 多次采样计算平均值
			// 同时这么做也使得ray的方向的期望近似指向像素中心
			for (int s = 0; s < samples_per_pixel; ++s) {
				auto u = (i + random_double()) / (image_width - 1);
				auto v = (j + random_double()) / (image_height - 1);
				ray r = cam.get_ray(u, v);
				//pixel_color += ray_color(r, world, max_depth);
				//pixel_color += ray_color(r, world, max_depth, light_ptr);
				pixel_color += clamp(ray_color(r, bvh_root, max_depth, light_ptr_list), 0, 1);
			}
			// 不知道上锁有没有用
			framebuffer_mutex.lock();
			write_color_to_framebuffer(framebuffer, pixel_color, samples_per_pixel, (image_height - 1 - j) * image_width + i);
			framebuffer_mutex.unlock();
		}
	}
}

#endif