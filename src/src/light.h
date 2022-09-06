#pragma once
#ifndef LIGHT_H
#define LIGHT_H

#include "vec3.h"
#include "global.h"

class light {
public:
	// 采样一个光源上的点，坐标存放到p中
	// 返回采样点在光源上的pdf
	virtual double sample_p(vec3 &p, vec3 &light_radiance, vec3 &light_normal) const = 0;
};


class circle_light : public light {
public:
	vec3 center;
	vec3 normal;
	double radius;
	vec3 radiance;
	vec3 b1, b2; // 切向基向量
	double pdf;

public:
	circle_light(const vec3 &center_init, const vec3 &normal_init, double radius_init, const vec3 &radiance_init) {
		center = center_init;
		normal = unit_vector(normal_init);
		radius = radius_init;
		radiance = radiance_init;
		build_basis(normal, b1, b2);
		pdf = 1 / (pi * radius * radius);
	}

	// 在圆上均匀采样
	virtual double sample_p(vec3 &p, vec3 &light_radiance, vec3 &light_normal) const override
	{
		double r = sqrt(random_double()) * radius;
		double phi = pi2 * random_double();
		p = center + b1 * r * sin(phi) + b2 * r * cos(phi);
		light_radiance = radiance;
		light_normal = normal;

		return pdf;
	}
};


class triangle_light : public light {
public:
	vec3 vertex[3];
	vec3 normal;
	vec3 radiance;
	double pdf;

public:
	triangle_light(const vec3 &vertex_1, const vec3 &vertex_2, const vec3 &vertex_3, const vec3 &radiance_init) {
		
		vertex[0] = vertex_1;
		vertex[1] = vertex_2;
		vertex[2] = vertex_3;
		vec3 tmp_vec = cross(vertex[1] - vertex[0], vertex[2] - vertex[0]);
		pdf = 1 / (tmp_vec.length() * 0.5);
		normal = unit_vector(tmp_vec);
		radiance = radiance_init;
	}

	// 在三角形上均匀采样
	virtual double sample_p(vec3 &p, vec3 &light_radiance, vec3 &light_normal) const override {
		
		// 先在平行四边形内采样，然后调整外部的点
		double x = random_double();
		double y = random_double();

		if (x + y > 1) {
			x = 1 - x;
			y = 1 - y;
		}

		p = vertex[0] + x * (vertex[1] - vertex[0]) + y * (vertex[2] - vertex[0]);
		light_radiance = radiance;
		light_normal = normal;

		return pdf;
	}
};


#endif