#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include "global.h"

class camera {

private:
	point3 origin;
	point3 lower_left_corner;
	vec3 horizontal;
	vec3 vertical;


public:
	camera(double aspect_ratio = 16.0 / 9.0) {
		//auto aspect_ratio = 16.0 / 9.0; // ��� / �߶�
		auto viewport_height = 2.0;
		auto viewport_width = aspect_ratio * viewport_height;
		auto focal_length = 2;

		origin = point3(0, 0, 0);
		horizontal = vec3(viewport_width, 0.0, 0.0);
		vertical = vec3(0.0, viewport_height, 0.0);
		lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);
	}

	// ������Ļ�������ɹ���
	// uvȡֵ��ΧΪ[0, 1] ^ 2
	ray get_ray(double u, double v) const {
		vec3 dir = lower_left_corner + u * horizontal + v * vertical - origin;
		dir = unit_vector(dir); //����λ��
		//return ray(origin, dir);
		return ray(lower_left_corner + u * horizontal + v * vertical, dir); // ��Ļ�ϵĵ���Ϊ���ߵ����
	}
};
#endif