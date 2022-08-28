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

public:
	color_map(const char file_name[]) {
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
				color_mat[i][j][0] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[2]) / 256.0, 2.2);
				color_mat[i][j][1] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[1]) / 256.0, 2.2);
				color_mat[i][j][2] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[0]) / 256.0, 2.2);
			}
		}
	}

	virtual vec3 get_value(const vec3 &uv) const override
	{
		// 预处理uv坐标，使其范围在[0,1)
		vec3 real_uv(0, 0, 0);
		real_uv[0] = uv[0] - std::floor(uv[0]);
		real_uv[1] = uv[1] - std::floor(uv[1]);

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

#endif