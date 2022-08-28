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

const double P_RR = 1; // ����˹���̶ĸ���


// ����׷�ٺ���
// ��ȱ�ʾʣ��ɷ�������
color ray_color(const ray& r, const hittable& world, int depth) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	if (world.hit(r, 0, infinity, rec)) {
		// ȷ���Ƿ���Ҫ����
		bool scatter = random_double() < P_RR;
		//std::cout << "test\n";
		if (scatter)
		{
			// �����������ⷽ��
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
			// �����������ֱ�ӷ��ظò��ʵ�radiance
			return rec.mat_ptr->get_radiance();
		}
	}

	// ���ʲô��û���У��򷵻ػ�����
	vec3 unit_direction = unit_vector(r.direction());
	auto t = 0.5*(unit_direction.y() + 1.0);
	//return (1.0 - t)*color(0.309, 0.737, 1) + t * color(0.309, 0.737, 1);
	return (1.0 - t)*color(0, 0, 0) + t * color(0, 0, 0);
}


// ����׷�ٺ���
// ��ȱ�ʾʣ��ɷ�������
// ���ӶԹ�Դ��������
color ray_color(const ray& r, const hittable& world, int depth, vector<shared_ptr<light>> light_ptr_list) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	if (world.hit(r, 0, infinity, rec)) {
		// ���������Ϣ
		vec3 normal = unit_vector(rec.normal); // shading point����
		vec3 wo = unit_vector(-r.direction()); // ���䷽��

		// ����ֱ�ӹ�
		vec3 radiance_direct = vec3(0, 0, 0);

		for (shared_ptr<light> light_ptr : light_ptr_list)
		{
			// ������Դ
			vec3 position_hit = rec.p; // shading point����
			vec3 position_light; // ��Դ����������
			vec3 radiance_light; // ��Դ������radiance
			vec3 normal_light; // ��Դ�����㷨��
			double pdf_light = light_ptr->sample_p(position_light, radiance_light, normal_light); // ��Դ������pdf
			vec3 wi_light = unit_vector(position_light - position_hit); // ���䷽�򣨵���Դ�����㣩

			// ����
			ray ray_to_light(position_hit, wi_light);
			hit_record rec_check_block;
			if (world.hit(ray_to_light, 0.000001, infinity, rec_check_block) and rec_check_block.t < (position_light - position_hit).length())
			{
				radiance_direct += vec3(0, 0, 0); // ���ڵ�����ֱ�ӹ�Ϊ0
			}
			else
			{
				double distance_light_square = (position_light - position_hit).length_squared(); // shading point����Դ����������ƽ��
				vec3 brdf_light = rec.mat_ptr->brdf(wi_light, wo, normal, rec.uv);
				vec3 radiance_direct_delta = radiance_light * brdf_light * dot(normal, wi_light) * dot(normal_light, -wi_light) / (distance_light_square * pdf_light); // ֱ�ӹ�
				radiance_direct += clamp(radiance_direct_delta, 0, std::numeric_limits<double>::infinity()); // ʹradiance�Ǹ�������ӹ�Դ������������⣩
			}
		}

		// ȷ���Ƿ���Ҫ����
		bool scatter = random_double() < P_RR;
		if (scatter)
		{
			// �����������ⷽ���ȡ��ӹ���
			vec3 wi; // ���䷽�����������
			double pdf = rec.mat_ptr->sample_wi(wi, wo, normal);
			vec3 brdf = rec.mat_ptr->brdf(wi, wo, normal, rec.uv);
			ray new_ray(rec.p + wi * 0.0001, wi);
			vec3 radiance_indirect = ray_color(new_ray, world, depth - 1, light_ptr_list) * brdf * dot(normal, wi) / pdf / P_RR;
			radiance_indirect = clamp(radiance_indirect, 0, std::numeric_limits<double>::infinity());

			// �õ�����radiance
			vec3 radiance = rec.mat_ptr->get_radiance();
			return radiance + radiance_direct + radiance_indirect;
		}
		else
		{
			// �õ�����radiance
			vec3 radiance = rec.mat_ptr->get_radiance();
			return radiance + radiance_direct;
		}
	}

	// ���ʲô��û���У��򷵻ػ�����
	return vec3(0.1, 0.1, 0.1);
}

// ����׷�ٺ���
// ��ȱ�ʾʣ��ɷ�������
// ���ӶԹ�Դ��������
// ʹ��bvh
color ray_color(const ray& r, shared_ptr<hittable> &bvh_root, int depth, vector<shared_ptr<light>> light_ptr_list) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	if (bvh_root->hit(r, 0, infinity, rec)) {
		// ���������Ϣ
		vec3 normal = unit_vector(rec.normal); // shading point����
		vec3 wo = unit_vector(-r.direction()); // ���䷽��

		// ����ֱ�ӹ�
		vec3 radiance_direct = vec3(0, 0, 0);

		for (shared_ptr<light> light_ptr : light_ptr_list)
		{
			// ������Դ
			vec3 position_hit = rec.p; // shading point����
			vec3 position_light; // ��Դ����������
			vec3 radiance_light; // ��Դ������radiance
			vec3 normal_light; // ��Դ�����㷨��
			double pdf_light = light_ptr->sample_p(position_light, radiance_light, normal_light); // ��Դ������pdf
			vec3 wi_light = unit_vector(position_light - position_hit); // ���䷽�򣨵���Դ�����㣩

			// ����
			ray ray_to_light(position_hit, wi_light);
			hit_record rec_check_block;
			if (bvh_root->hit(ray_to_light, 0.000001, infinity, rec_check_block) and rec_check_block.t < (position_light - position_hit).length())
			{
				radiance_direct += vec3(0, 0, 0); // ���ڵ�����ֱ�ӹ�Ϊ0
			}
			else
			{
				double distance_light_square = (position_light - position_hit).length_squared(); // shading point����Դ����������ƽ��
				vec3 brdf_light = rec.mat_ptr->brdf(wi_light, wo, normal, rec.uv);
				vec3 radiance_direct_delta = radiance_light * brdf_light * dot(normal, wi_light) * dot(normal_light, -wi_light) / (distance_light_square * pdf_light); // ֱ�ӹ�
				radiance_direct += clamp(radiance_direct_delta, 0, std::numeric_limits<double>::infinity()); // ʹradiance�Ǹ�������ӹ�Դ������������⣩
			}
		}

		// ȷ���Ƿ���Ҫ�������������������ӹ�
		bool scatter = random_double() < P_RR;
		if (scatter)
		{
			// �����������ⷽ���ȡ��ӹ���
			vec3 wi; // ���䷽�����������
			double pdf = rec.mat_ptr->sample_wi(wi, wo, normal);
			vec3 brdf = rec.mat_ptr->brdf(wi, wo, normal, rec.uv);
			ray new_ray(rec.p + wi * 0.0001, wi);
			vec3 radiance_indirect = ray_color(new_ray, bvh_root, depth - 1, light_ptr_list) * brdf * dot(normal, wi) / pdf / P_RR;
			radiance_indirect = clamp(radiance_indirect, 0, std::numeric_limits<double>::infinity());

			// �õ�����radiance
			vec3 radiance = rec.mat_ptr->get_radiance();
			return radiance + radiance_direct + radiance_indirect;
		}
		else
		{
			// �õ�����radiance
			vec3 radiance = rec.mat_ptr->get_radiance();
			return radiance + radiance_direct;
		}
	}

	// ���ʲô��û���У��򷵻ػ�����
	return vec3(0.1, 0.1, 0.1);
}


// ����ɫд��framebuffer��
// ���ؽ�һ��һ��д��ͼ�񣬴������ң���������
// ����j�����������Ϊд��ʱu�𽥼�С��������������,u��yͬ��v��xͬ��
// i�ڵ���ʱÿ�ε�����ֵ���Բ�Ϊ1,ͨ��step��������
// bias����Ϊi��������㣨������
void render(const int &image_height, const int &image_width, const int &samples_per_pixel, const int &max_depth,
	const hittable_list &world, const camera &cam, vector<vec3> &framebuffer, const int &step, const int &bias)
{
	for (int j = image_height - 1; j >= 0; --j) {
		std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush; // ������ʾ
		for (int i = bias; i < image_width; i += step) {
			color pixel_color(0, 0, 0);
			// ��β�������ƽ��ֵ
			// ͬʱ��ô��Ҳʹ��ray�ķ������������ָ����������
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


// render()��ָ��汾��֧�ֶ��߳�
// ����world��framebuffer����ָ�봫�ݣ�������������ֵ����
mutex framebuffer_mutex;
void render_raw(int image_height, int image_width, int samples_per_pixel, int max_depth,
	hittable_list world, camera cam, vector<vec3> *framebuffer, vector<shared_ptr<light>> light_ptr_list, int step, int bias)
{
	for (int j = image_height - 1; j >= 0; --j) {
		std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush; // ������ʾ
		for (int i = bias; i < image_width; i += step) {
			color pixel_color(0, 0, 0);
			// ��β�������ƽ��ֵ
			// ͬʱ��ô��Ҳʹ��ray�ķ������������ָ����������
			for (int s = 0; s < samples_per_pixel; ++s) {
				auto u = (i + random_double()) / (image_width - 1);
				auto v = (j + random_double()) / (image_height - 1);
				ray r = cam.get_ray(u, v);
				//pixel_color += ray_color(r, world, max_depth);
				//pixel_color += ray_color(r, world, max_depth, light_ptr);
				pixel_color += clamp(ray_color(r, world, max_depth, light_ptr_list), 0, 1);
			}
			// ��֪��������û����
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
		std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush; // ������ʾ
		for (int i = bias; i < image_width; i += step) {
			color pixel_color(0, 0, 0);
			// ��β�������ƽ��ֵ
			// ͬʱ��ô��Ҳʹ��ray�ķ������������ָ����������
			for (int s = 0; s < samples_per_pixel; ++s) {
				auto u = (i + random_double()) / (image_width - 1);
				auto v = (j + random_double()) / (image_height - 1);
				ray r = cam.get_ray(u, v);
				//pixel_color += ray_color(r, world, max_depth);
				//pixel_color += ray_color(r, world, max_depth, light_ptr);
				pixel_color += clamp(ray_color(r, bvh_root, max_depth, light_ptr_list), 0, 1);
			}
			// ��֪��������û����
			framebuffer_mutex.lock();
			write_color_to_framebuffer(framebuffer, pixel_color, samples_per_pixel, (image_height - 1 - j) * image_width + i);
			framebuffer_mutex.unlock();
		}
	}
}

#endif