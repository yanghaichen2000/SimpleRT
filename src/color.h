#pragma once
#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"

#include <string>
#include <fstream>
#include <sstream>
#include <vector>

using std::string;
using std::vector;
using std::stringstream;
using std::to_string;


// 将一个像素的颜色写入ppm文件
// pixel_color的范围为[0, 1] ^ 3
void write_color(std::fstream &out, color pixel_color, int samples_per_pixel) {
	auto r = pixel_color.x();
	auto g = pixel_color.y();
	auto b = pixel_color.z();

	// Divide the color by the number of samples.
	auto scale = 1.0 / samples_per_pixel;
	r = pow(r * scale, 1 / 2.2);
	g = pow(g * scale, 1 / 2.2);
	b = pow(b * scale, 1 / 2.2);

	// Write the translated [0,255] value of each color component.
	out << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
		<< static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
		<< static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';
}

// 将颜色[0, 1] ^ 3写入framebuffer
// 引用版
void write_color_to_framebuffer(vector<vec3> &framebuffer, color pixel_color, int samples_per_pixel, int index) {
	auto r = pixel_color.x();
	auto g = pixel_color.y();
	auto b = pixel_color.z();

	// Divide the color by the number of samples.
	auto scale = 1.0 / samples_per_pixel;
	r = pow(r * scale, 1 / 2.2);
	g = pow(g * scale, 1 / 2.2);
	b = pow(b * scale, 1 / 2.2);

	// Write the value of each color component.
	framebuffer[index] = vec3(r, g, b);
	
	//ssm << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
	//	<< static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
	//	<< static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';
}

// 将颜色[0, 1] ^ 3写入framebuffer
// 指针版
void write_color_to_framebuffer(vector<vec3> *framebuffer, color pixel_color, int samples_per_pixel, int index) {
	auto r = pixel_color.x();
	auto g = pixel_color.y();
	auto b = pixel_color.z();

	// Divide the color by the number of samples.
	auto scale = 1.0 / samples_per_pixel;
	r = pow(r * scale, 1 / 2.2);
	g = pow(g * scale, 1 / 2.2);
	b = pow(b * scale, 1 / 2.2);

	// Write the value of each color component.
	(*framebuffer)[index] = vec3(r, g, b);

	//ssm << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
	//	<< static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
	//	<< static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';
}

#endif