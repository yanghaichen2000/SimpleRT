#pragma once
#ifndef TEXTURE_H
#define TEXTURE_H

#include "vec3.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>


class texture {
public:
	virtual vec3 get_value(const vec3 &uv) const = 0;
};

// 纯色texture
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

// 图像texture
class color_map : public texture {
public:
	// 存放图像RGB值(0.0~1.0)的矩阵
	// 线性空间
	// 从下往上，从左往右
	vec3 **color_mat;
	// 图像行数和列数
	int rows, cols; 
	// 放大倍数
	double scale = 1;

public:
	// scale越大，纹理越密集
	color_map(const char file_name[], double scale_init = 1) {
		scale = scale_init;
		
		// 读取图像
		cv::Mat image;
		image = cv::imread(file_name);

		// 获取图像尺寸
		rows = image.rows;
		cols = image.cols;

		// 创建存储空间
		color_mat = new vec3*[rows];
		for (int i = 0; i < rows; i++) {
			color_mat[i] = new vec3[cols];
		}

		// 记录数据
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				// opencv中颜色存储顺序为BGR，所以需要反转一下顺序
				// 这里注意要将RGB值转换到线性空间
				color_mat[i][j][0] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[2]) / 255.0, 2.2);
				color_mat[i][j][1] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[1]) / 255.0, 2.2);
				color_mat[i][j][2] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[0]) / 255.0, 2.2);
			}
		}
	}

	virtual vec3 get_value(const vec3 &uv) const override
	{
		// 预处理uv坐标，使其范围在[0,1)
		vec3 real_uv = uv * scale;
		real_uv[0] = real_uv[0] - std::floor(real_uv[0]);
		real_uv[1] = real_uv[1] - std::floor(real_uv[1]);

		// 将uv值映射到像素坐标
		// 映射后的值范围为[0, cols - 1]与[0, rows - 1]
		double row = real_uv[1] * (rows - 1);
		double col = real_uv[0] * (cols - 1);

		// 获得相邻四个像素的坐标
		int row_0 = std::floor(row);
		int row_1 = row_0 + 1;
		int col_0 = std::floor(col);
		int col_1 = col_0 + 1;

		// 在列上插值
		vec3 RGB_left = color_mat[row_1][col_0] * (row - row_0) + color_mat[row_0][col_0] * (row_1 - row);
		vec3 RGB_right = color_mat[row_1][col_1] * (row - row_0) + color_mat[row_0][col_1] * (row_1 - row);

		// 在行上插值
		vec3 RGB = RGB_right * (col - col_0) + RGB_left * (col_1 - col);

		return RGB;
	}

	~color_map() {
		// 手动释放空间
		for (int i = 0; i < rows; i++) {
			delete[] color_mat[i];
		}
		delete[] color_mat;
	}
};


// 法线贴图
class normal_map : public texture {
public:
	// 存放图像RGB值(0.0~1.0)的矩阵
	// 线性空间
	// 从下往上，从左往右
	vec3 **normal_mat;
	// 图像行数和列数
	int rows, cols;
	// 放大倍数
	double scale = 1;

public:
	// scale越大，纹理越密集
	normal_map(const char file_name[], double scale_init = 1) {
		scale = scale_init;
		
		// 读取图像
		cv::Mat image;
		image = cv::imread(file_name);

		// 获取图像尺寸
		rows = image.rows;
		cols = image.cols;

		// 创建存储空间
		normal_mat = new vec3*[rows];
		for (int i = 0; i < rows; i++) {
			normal_mat[i] = new vec3[cols];
		}

		// 记录数据
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				// opencv中颜色存储顺序为BGR，所以需要反转一下顺序
				// 将数据从[0, 255]映射到[-1, 1]
				// TODO:目前的映射方法并不满足单位化条件，之后可能需要修复
				normal_mat[i][j][0] = int(image.at<cv::Vec3b>(rows - i - 1, j)[2]) / 255.0 * 2 - 1; // T分量
				normal_mat[i][j][1] = int(image.at<cv::Vec3b>(rows - i - 1, j)[1]) / 255.0 * 2 - 1; // B分量
				normal_mat[i][j][2] = int(image.at<cv::Vec3b>(rows - i - 1, j)[0]) / 255.0 * 2 - 1; // N分量
				// 单位化
				normal_mat[i][j] = unit_vector(normal_mat[i][j]);
			}
		}
	}

	virtual vec3 get_value(const vec3 &uv) const override
	{
		// 预处理uv坐标，使其范围在[0,1)
		vec3 real_uv = uv * scale;
		real_uv[0] = real_uv[0] - std::floor(real_uv[0]);
		real_uv[1] = real_uv[1] - std::floor(real_uv[1]);

		// 将uv值映射到像素坐标
		// 映射后的值范围为[0, cols - 1]与[0, rows - 1]
		double row = real_uv[1] * (rows - 1);
		double col = real_uv[0] * (cols - 1);

		// 获得相邻四个像素的坐标
		int row_0 = std::floor(row);
		int row_1 = row_0 + 1;
		int col_0 = std::floor(col);
		int col_1 = col_0 + 1;

		// 在列上插值
		vec3 RGB_left = normal_mat[row_1][col_0] * (row - row_0) + normal_mat[row_0][col_0] * (row_1 - row);
		vec3 RGB_right = normal_mat[row_1][col_1] * (row - row_0) + normal_mat[row_0][col_1] * (row_1 - row);

		// 在行上插值
		vec3 RGB = RGB_right * (col - col_0) + RGB_left * (col_1 - col);

		// 单位化
		RGB = unit_vector(RGB);

		return RGB;
	}

	~normal_map() {
		// 手动释放空间
		for (int i = 0; i < rows; i++) {
			delete[] normal_mat[i];
		}
		delete[] normal_mat;
	}
};

#endif