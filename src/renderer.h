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
// ���ӶԹ�Դ��������
// ʹ��bvh
// ֧��͸������
color ray_color(const ray& r, shared_ptr<hittable> &bvh_root, int depth, vector<shared_ptr<light>> light_ptr_list) {
	hit_record rec;

	if (depth <= 0)
		return color(0, 0, 0);

	if (bvh_root->hit(r, 0, infinity, rec)) {
		// ��ȡ͸����
		double alpha = rec.mat_ptr->get_color_map_ptr()->get_alpha(rec.uv);

		// ��������Ǵ�͸���ǲ���͸������ȡ����͸����
		if (random_double() > alpha) { // ��͸
			// ֱ������һ�����߼�����ǰ����
			ray new_ray(rec.p + r.dir * 0.00001, r.dir, r.med);
			// �����º�����depthֵ�������
			if (random_double() > 0.9)
				return ray_color(new_ray, bvh_root, depth - 1, light_ptr_list);
			else
				return ray_color(new_ray, bvh_root, depth, light_ptr_list);
		}
		else {
			// ���������Ϣ
			vec3 normalo = unit_vector(rec.normal); // ����㷨��
			vec3 wo = unit_vector(-r.direction()); // ���䷽��
			vec3 positiono = rec.p; // ���������

			// ����ֱ�ӹ�
			vec3 radiance_direct = vec3(0, 0, 0);
			bool include_direct_light = false;
			if (rec.mat_ptr->sample_light()) {
				for (shared_ptr<light> light_ptr : light_ptr_list)
				{
					// ��ȡ��Դ����Ϣ
					vec3 position_light; // ��Դ����������
					vec3 radiance_light; // ��Դ������radiance
					vec3 normal_light; // ��Դ�����㷨��
					double pdf_light = light_ptr->sample_p(position_light, radiance_light, normal_light); // ��Դ������pdf

					// ������������
					double pdf_p; // ��������pdf
					vec3 normali; // ����㷨��
					vec3 positioni; // ���������
					tie(pdf_p, normali, positioni) = rec.mat_ptr->sample_positioni(normalo, positiono, bvh_root); // ��ȡ��������pdf������㷨�ߣ����䷽��

					// �õ����䷽�򣨵���Դ�����㣩
					vec3 wi_light = unit_vector(position_light - positioni);

					// �����Դ�Ĺ���
					ray ray_to_light(positioni, wi_light);
					hit_record rec_check_block;

					if (bvh_root->hit(ray_to_light, 0.000001, infinity, rec_check_block) and rec_check_block.t < (position_light - positioni).length())
					{
						radiance_direct += vec3(0, 0, 0); // ���ڵ�����ֱ�ӹ�Ϊ0
						include_direct_light = true;
					}
					else
					{
						double distance_light_square = (position_light - positioni).length_squared(); // shading point����Դ����������ƽ��
						bool wo_front = rec.front_face;
						bool wi_front = dot(normali, wi_light) > 0 ? wo_front : not wo_front;
						sample_light_flag = true;
						vec3 brdf_light = rec.mat_ptr->bsdf(wo, normalo, positiono, wo_front, rec.uv, wi_light, normali, positioni, wi_front); // ���㵽��Դ��bsdf
						vec3 radiance_direct_delta = radiance_light * brdf_light * dot(normalo, wi_light) * dot(normal_light, -wi_light) / (distance_light_square * pdf_light * pdf_p); // ֱ�ӹ�
						radiance_direct += clamp(radiance_direct_delta, 0, std::numeric_limits<double>::infinity()); // ʹradiance�Ǹ�������ӹ�Դ������������⣩
					}
				}
			}

			// ȷ���Ƿ���Ҫ�������������������ӹ�
			bool scatter = random_double() < P_RR;
			if (scatter)
			{
				// �����������ⷽ���ȡ��ӹ���
				vec3 wi; // ���䷽�����������
				double pdf_w; // ���䷽�����������ܶ�
				double pdf_p; // �����λ�ò���������ܶ�
				vec3 normali; // ����㷨��
				vec3 positioni; // ���������
				bool wo_front = rec.front_face;
				bool wi_front;
				tie(pdf_p, normali, positioni) = rec.mat_ptr->sample_positioni(normalo, positiono, bvh_root); // ��ȡ��������pdf������㷨�ߣ����䷽��
				tie(pdf_w, wi, wi_front) = rec.mat_ptr->sample_wi(wo, normali, wo_front);
#ifdef test_mode
				std::cout << "depth = " << depth << '\n';
#endif
				sample_light_flag = false;
				vec3 brdf = rec.mat_ptr->bsdf(wo, normalo, positiono, wo_front, rec.uv, wi, normali, positioni, wi_front);
				
				ray new_ray(positioni + wi * 0.0001, wi); // �µĹ��ߴ���������
				vec3 radiance_indirect;
				// ������ߴ�͸����ôdepth������
				if (wo_front == wi_front) {
					radiance_indirect = ray_color(new_ray, bvh_root, depth - 1, light_ptr_list) * brdf * dot(normalo, wi) / (pdf_w * pdf_p * P_RR);
				}
				else
				{
					radiance_indirect = ray_color(new_ray, bvh_root, depth, light_ptr_list) * brdf * dot(normalo, wi) / (pdf_w * pdf_p * P_RR);
				}
				radiance_indirect = clamp(radiance_indirect, 0, std::numeric_limits<double>::infinity());

				// �õ�����radiance
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
				// �õ�����radiance
				vec3 radiance = rec.mat_ptr->get_radiance();
				return radiance + radiance_direct;
			}
		}
	}

	// ���ʲô��û���У��򷵻ػ�����
	return vec3(0, 0, 0);
}


mutex framebuffer_mutex;
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