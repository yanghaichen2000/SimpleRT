#pragma once
#ifndef RAY_H
#define RAY_H

#include "vec3.h"
#include "medium.h"

class ray {
public:
	point3 orig; // ���
	vec3 dir; // ���򣬲�һ���ǵ�λ����
	medium med; // ����

public:
	ray() {}
	ray(const point3& origin, const vec3& direction, const medium& med_init = medium(1))
	{
		orig = origin;
		dir = direction;
		med = med_init;      
	}

	point3 origin() const { return orig; }
	vec3 direction() const { return dir; }
	
	// �������t���������
	point3 at(double t) const {
		return orig + t * dir;
	}


};

#endif