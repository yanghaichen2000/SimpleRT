#pragma once
#ifndef RAY_H
#define RAY_H

#include "vec3.h"
#include "medium.h"

class ray {
public:
	point3 orig; // 起点
	vec3 dir; // 方向，不一定是单位向量
	medium med; // 介质

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
	
	// 距离起点t距离的坐标
	point3 at(double t) const {
		return orig + t * dir;
	}


};

#endif