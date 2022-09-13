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
// 增加对光源采样功能
// 使用bvh
// 支持透明材质
color ray_color(const ray& r, shared_ptr<hittable> &bvh_root, int depth, vector<shared_ptr<light>> light_ptr_list) {
	hit_record rec;

	if (depth <= 0)
		return color(0, 0, 0);

	if (bvh_root->hit(r, 0, infinity, rec)) {
		// 获取透明度
		double alpha = rec.mat_ptr->get_color_map_ptr()->get_alpha(rec.uv);

		// 随机决定是穿透还是不穿透，概率取决于透明度
		if (random_double() > alpha) { // 穿透
			// 直接生成一束光线继续向前传播
			ray new_ray(rec.p + r.dir * 0.00001, r.dir, r.med);
			// 传入新函数的depth值不会减少
			if (random_double() > 0.9)
				return ray_color(new_ray, bvh_root, depth - 1, light_ptr_list);
			else
				return ray_color(new_ray, bvh_root, depth, light_ptr_list);
		}
		else {
			// 计算基本信息
			vec3 normalo = unit_vector(rec.normal); // 出射点法线
			vec3 wo = unit_vector(-r.direction()); // 出射方向
			vec3 positiono = rec.p; // 出射点坐标

			// 计算直接光
			vec3 radiance_direct = vec3(0, 0, 0);
			bool include_direct_light = false;
			if (rec.mat_ptr->sample_light()) {
				for (shared_ptr<light> light_ptr : light_ptr_list)
				{
					// 获取光源的信息
					vec3 position_light; // 光源采样点坐标
					vec3 radiance_light; // 光源采样点radiance
					vec3 normal_light; // 光源采样点法线
					double pdf_light = light_ptr->sample_p(position_light, radiance_light, normal_light); // 光源采样点pdf

					// 随机采样入射点
					double pdf_p; // 入射点采样pdf
					vec3 normali; // 入射点法线
					vec3 positioni; // 入射点坐标
					tie(pdf_p, normali, positioni) = rec.mat_ptr->sample_positioni(normalo, positiono, bvh_root); // 获取入射点采样pdf，入射点法线，入射方向

					// 得到入射方向（到光源采样点）
					vec3 wi_light = unit_vector(position_light - positioni);

					// 计算光源的贡献
					ray ray_to_light(positioni, wi_light);
					hit_record rec_check_block;

					if (bvh_root->hit(ray_to_light, 0.000001, infinity, rec_check_block) and rec_check_block.t < (position_light - positioni).length())
					{
						radiance_direct += vec3(0, 0, 0); // 有遮挡物则直接光为0
						include_direct_light = true;
					}
					else
					{
						double distance_light_square = (position_light - positioni).length_squared(); // shading point到光源采样点距离的平方
						bool wo_front = rec.front_face;
						bool wi_front = dot(normali, wi_light) > 0 ? wo_front : not wo_front;
						sample_light_flag = true;
						vec3 brdf_light = rec.mat_ptr->bsdf(wo, normalo, positiono, wo_front, rec.uv, wi_light, normali, positioni, wi_front); // 计算到光源的bsdf
						vec3 radiance_direct_delta = radiance_light * brdf_light * dot(normalo, wi_light) * dot(normal_light, -wi_light) / (distance_light_square * pdf_light * pdf_p); // 直接光
						radiance_direct += clamp(radiance_direct_delta, 0, std::numeric_limits<double>::infinity()); // 使radiance非负（解决从光源反向射出的问题）
					}
				}
			}

			// 确定是否需要反弹，如果反弹则计算间接光
			bool scatter = random_double() < P_RR;
			if (scatter)
			{
				// 随机采样入射光方向获取间接光照
				vec3 wi; // 入射方向（随机采样）
				double pdf_w; // 入射方向采样点概率密度
				double pdf_p; // 入射点位置采样点概率密度
				vec3 normali; // 入射点法线
				vec3 positioni; // 入射点坐标
				bool wo_front = rec.front_face;
				bool wi_front;
				tie(pdf_p, normali, positioni) = rec.mat_ptr->sample_positioni(normalo, positiono, bvh_root); // 获取入射点采样pdf，入射点法线，入射方向
				tie(pdf_w, wi, wi_front) = rec.mat_ptr->sample_wi(wo, normali, wo_front);
#ifdef test_mode
				std::cout << "depth = " << depth << '\n';
#endif
				sample_light_flag = false;
				vec3 brdf = rec.mat_ptr->bsdf(wo, normalo, positiono, wo_front, rec.uv, wi, normali, positioni, wi_front);
				
				ray new_ray(positioni + wi * 0.0001, wi); // 新的光线从入射点射出
				vec3 radiance_indirect;
				// 如果光线穿透，那么depth不减少
				if (wo_front == wi_front) {
					radiance_indirect = ray_color(new_ray, bvh_root, depth - 1, light_ptr_list) * brdf * dot(normalo, wi) / (pdf_w * pdf_p * P_RR);
				}
				else
				{
					radiance_indirect = ray_color(new_ray, bvh_root, depth, light_ptr_list) * brdf * dot(normalo, wi) / (pdf_w * pdf_p * P_RR);
				}
				radiance_indirect = clamp(radiance_indirect, 0, std::numeric_limits<double>::infinity());

				// 得到最终radiance
				vec3 radiance = rec.mat_ptr->get_radiance();
				if (include_direct_light) {
					return radiance + radiance_direct + radiance_indirect;
				}
				else {
					return radiance + radiance_direct + radiance_indirect;
				}
			}
			else
			{
				// 得到最终radiance
				vec3 radiance = rec.mat_ptr->get_radiance();
				return radiance + radiance_direct;
			}
		}
	}

	// 如果什么都没打中，则返回环境光
	return vec3(0, 0, 0);
}


mutex framebuffer_mutex;
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