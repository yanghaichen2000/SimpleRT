#pragma once
#ifndef MATERIAL_SAMPLES
#define MATERIAL_SAMPLES

#include "material.h"


class material_samples {
public:
	material_samples() {}

	static shared_ptr<phong_material> diffuse_white() {
		return make_shared<phong_material>(vec3(1.0, 1.0, 1.0));
	}

	static shared_ptr<phong_material> diffuse_red() {
		return make_shared<phong_material>(vec3(1.0, 0.0, 0.0));
	}

	static shared_ptr<phong_material> diffuse_green() {
		return make_shared<phong_material>(vec3(0.0, 1.0, 0.0));
	}

	static shared_ptr<phong_material> diffuse_blue() {
		return make_shared<phong_material>(vec3(0.0, 0.0, 1.0));
	}

	static shared_ptr<ggx_metal_material> metal_aluminum(double roughness) {
		return make_shared<ggx_metal_material>(roughness, vec3(0.939, 0.914, 0.822));
	}

	static shared_ptr<ggx_metal_material> metal_gold(double roughness) {
		return make_shared<ggx_metal_material>(roughness, vec3(1, 0.582, 0.0956));
	}

	static shared_ptr<ggx_metal_material> metal_copper(double roughness) {
		return make_shared<ggx_metal_material>(roughness, vec3(0.904, 0.372, 0.256));
	}

};



#endif