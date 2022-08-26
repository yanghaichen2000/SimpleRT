#pragma once
#ifndef TEXTURE_H
#define TEXTURE_H

#include "vec3.h"


class texture {
public:
	virtual vec3 get_value(const vec3 &uv) const = 0;
};

// ´¿É«texture
// ±àºÅÎª0
class simple_color_texture : public texture {
public:
	vec3 color;

public:
	simple_color_texture(const vec3 &color_init)
	{
		color = clamp(color_init, 0, 1);
	}

	virtual vec3 get_value(const vec3 &uv) const override
	{
		return color;
	}
};

// Í¼Ïñtexture
// ±àºÅÎª1


#endif