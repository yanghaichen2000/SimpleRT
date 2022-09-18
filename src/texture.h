#pragma once
#ifndef TEXTURE_H
#define TEXTURE_H

#include "vec3.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>


class texture {
public:
	virtual vec3 get_value(const vec3 &uv) const = 0;
	virtual double get_alpha(const vec3 &uv) const = 0;
};

// 纯色texture
class simple_color_texture : public texture {
public:
	vec3 color;
	double alpha;

public:
	simple_color_texture(const vec3 &color_init, double alpha_init = 1)
	{
		color = clamp(color_init, 0, 1);
		alpha = alpha_init;
	}

	simple_color_texture(int R, int G, int B, double alpha_init = 1)
	{
		color = clamp(vec3(pow(R / 255.0, 2.2), pow(G / 255.0, 2.2), pow(B / 255.0, 2.2)), 0, 1);
		alpha = alpha_init;
	}

	virtual vec3 get_value(const vec3 &uv) const override
	{
		return color;
	}

	double get_alpha(const vec3 &uv) const override
	{
		return alpha;
	}
};

// 图像texture
class color_map : public texture {
public:
	// 存放图像RGB值(0.0~1.0)的矩阵
	// 线性空间
	// 从下往上，从左往右
	vec3 **color_mat;
	// 存放图像alpha值(0.0~1.0)的矩阵
	// 0为完全透明，1为完全不透明
	// 与color_mat中元素对应
	double **alpha_mat;
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
		image = cv::imread(file_name, -1); // 第二个参数为-1，表示读取透明通道

		// 获取图像尺寸
		rows = image.rows;
		cols = image.cols;

		// 创建存储空间
		color_mat = new vec3*[rows];
		for (int i = 0; i < rows; i++) {
			color_mat[i] = new vec3[cols];
		}
		alpha_mat = new double*[rows];
		for (int i = 0; i < rows; i++) {
			alpha_mat[i] = new double[cols];
		}

		// 记录数据
		// 这里分为两种情况进行处理：
		// 对于三通道图片，必须用cv::Vec3b解析，否则得到的数据会打乱
		// 对于四通道图片，必须用cv::Vec4b解析，否则无法获取alpha通道
		if (image.channels() == 3) {
			for (int i = 0; i < rows; i++) {
				for (int j = 0; j < cols; j++) {
					// opencv中颜色存储顺序为BGR，所以需要反转一下顺序
					// 这里注意要将RGB值转换到线性空间
					color_mat[i][j][0] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[2]) / 255.0, 2.2);
					color_mat[i][j][1] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[1]) / 255.0, 2.2);
					color_mat[i][j][2] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[0]) / 255.0, 2.2);
					alpha_mat[i][j] = 1; // 3通道，alpha设为1
				}
			}
		} 
		else if (image.channels() == 4) {
			for (int i = 0; i < rows; i++) {
				for (int j = 0; j < cols; j++) {
					// opencv中颜色存储顺序为BGR，所以需要反转一下顺序
					// 这里注意要将RGB值转换到线性空间
					color_mat[i][j][0] = pow(int(image.at<cv::Vec4b>(rows - i - 1, j)[2]) / 255.0, 2.2);
					color_mat[i][j][1] = pow(int(image.at<cv::Vec4b>(rows - i - 1, j)[1]) / 255.0, 2.2);
					color_mat[i][j][2] = pow(int(image.at<cv::Vec4b>(rows - i - 1, j)[0]) / 255.0, 2.2);
					alpha_mat[i][j] = int(image.at<cv::Vec4b>(rows - i - 1, j)[3]) / 255.0;
				}
			}
		}
		else { // 通道数不是3也不是4
			std::cout << "error in texture.h: color_map::color_map(): unsupported image channels = " << image.channels() << "\n";
			std::cout << "file name : " << file_name << '\n';
			exit(-1);
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
		int row_0 = static_cast<int>(std::floor(row));
		int row_1 = row_0 + 1;
		int col_0 = static_cast<int>(std::floor(col));
		int col_1 = col_0 + 1;

		// 在列上插值
		vec3 RGB_left = color_mat[row_1][col_0] * (row - row_0) + color_mat[row_0][col_0] * (row_1 - row);
		vec3 RGB_right = color_mat[row_1][col_1] * (row - row_0) + color_mat[row_0][col_1] * (row_1 - row);

		// 在行上插值
		vec3 RGB = RGB_right * (col - col_0) + RGB_left * (col_1 - col);

		return RGB;
	}

	double get_alpha(const vec3 &uv) const override {

		// 预处理uv坐标，使其范围在[0,1)
		vec3 real_uv = uv * scale;
		real_uv[0] = real_uv[0] - std::floor(real_uv[0]);
		real_uv[1] = real_uv[1] - std::floor(real_uv[1]);

		// 将uv值映射到像素坐标
		// 映射后的值范围为[0, cols - 1]与[0, rows - 1]
		double row = real_uv[1] * (rows - 1);
		double col = real_uv[0] * (cols - 1);

		// 获得相邻四个像素的坐标
		int row_0 = static_cast<int>(std::floor(row));
		int row_1 = row_0 + 1;
		int col_0 = static_cast<int>(std::floor(col));
		int col_1 = col_0 + 1;

		// 在列上插值
		double alpha_left = alpha_mat[row_1][col_0] * (row - row_0) + alpha_mat[row_0][col_0] * (row_1 - row);
		double alpha_right = alpha_mat[row_1][col_1] * (row - row_0) + alpha_mat[row_0][col_1] * (row_1 - row);

		// 在行上插值
		double alpha = alpha_right * (col - col_0) + alpha_left * (col_1 - col);
		
		return alpha;
	}

	~color_map() {
		// 手动释放空间
		for (int i = 0; i < rows; i++) {
			delete[] color_mat[i];
			delete[] alpha_mat[i];
		}
		delete[] color_mat;
		delete[] alpha_mat;
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
		int row_0 = static_cast<int>(std::floor(row));
		int row_1 = row_0 + 1;
		int col_0 = static_cast<int>(std::floor(col));
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

	double get_alpha(const vec3 &uv) const override {
		return 1;
	}

	~normal_map() {
		// 手动释放空间
		for (int i = 0; i < rows; i++) {
			delete[] normal_mat[i];
		}
		delete[] normal_mat;
	}
};

// 置换贴图
class displacement_map : public texture {
public:
	// 存放位移量的矩阵
	// 从下往上，从左往右
	double** displacement_mat;
	// 图像行数和列数
	int rows, cols;
	// 放大倍数
	double scale;
	// 强度
	double strength;

public:
	// scale越大，纹理越密集
	displacement_map(const char file_name[], double scale_init = 1, double strength_init = 0.02) {
		scale = scale_init;
		strength = strength_init;

		// 读取图像
		cv::Mat image;
		image = cv::imread(file_name, -1); // 第二个参数为-1，表示读取透明通道
		  
		// 获取图像尺寸
		rows = image.rows;
		cols = image.cols;

		// 创建存储空间
		displacement_mat = new double *[rows];
		for (int i = 0; i < rows; i++) {
			displacement_mat[i] = new double[cols];
		}

		// 记录数据
		// 这里分为两种情况进行处理：
		// 对于三通道图片，必须用cv::Vec3b解析，否则得到的数据会打乱
		// 对于四通道图片，必须用cv::Vec4b解析，否则无法获取alpha通道
		if (image.channels() == 3) {
			for (int i = 0; i < rows; i++) {
				for (int j = 0; j < cols; j++) {
					// opencv中颜色存储顺序为BGR，所以需要反转一下顺序
					// 这里注意要将RGB值转换到线性空间
					displacement_mat[i][j] = 0;
					displacement_mat[i][j] += int(image.at<cv::Vec3b>(rows - i - 1, j)[2]) / 255.0 * 2 - 1;
					displacement_mat[i][j] += int(image.at<cv::Vec3b>(rows - i - 1, j)[1]) / 255.0 * 2 - 1;
					displacement_mat[i][j] += int(image.at<cv::Vec3b>(rows - i - 1, j)[0]) / 255.0 * 2 - 1;
					displacement_mat[i][j] /= 3.0;
				}
			}
		}
		else if (image.channels() == 4) {
			for (int i = 0; i < rows; i++) {
				for (int j = 0; j < cols; j++) {
					// opencv中颜色存储顺序为BGR，所以需要反转一下顺序
					// 这里注意要将RGB值转换到线性空间
					displacement_mat[i][j] = 0;
					displacement_mat[i][j] += int(image.at<cv::Vec4b>(rows - i - 1, j)[2]) / 255.0 * 2 - 1;
					displacement_mat[i][j] += int(image.at<cv::Vec4b>(rows - i - 1, j)[1]) / 255.0 * 2 - 1;
					displacement_mat[i][j] += int(image.at<cv::Vec4b>(rows - i - 1, j)[0]) / 255.0 * 2 - 1;
					displacement_mat[i][j] /= 3.0;
				}
			}
		}
		else if (image.channels() == 1) {
			for (int i = 0; i < rows; i++) {
				for (int j = 0; j < cols; j++) {
					// opencv中颜色存储顺序为BGR，所以需要反转一下顺序
					// 这里注意要将RGB值转换到线性空间
					displacement_mat[i][j] = int(image.at<cv::uint8_t>(rows - i - 1, j)) / 255.0 * 2 - 1;
				}
			}
		}
		else { // 通道数不是3也不是4
			std::cout << "error in texture.h: color_map::color_map(): unsupported image channels = " << image.channels() << "\n";
			std::cout << "file name : " << file_name << '\n';
			exit(-1);
		}
	}

	virtual vec3 get_value(const vec3& uv) const override
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
		int row_0 = static_cast<int>(std::floor(row));
		int row_1 = row_0 + 1;
		int col_0 = static_cast<int>(std::floor(col));
		int col_1 = col_0 + 1;

		// 在列上插值
		double displacement_left = displacement_mat[row_1][col_0] * (row - row_0) + displacement_mat[row_0][col_0] * (row_1 - row);
		double displacement_right = displacement_mat[row_1][col_1] * (row - row_0) + displacement_mat[row_0][col_1] * (row_1 - row);

		// 在行上插值
		double displacement_value = displacement_right * (col - col_0) + displacement_left * (col_1 - col);
		displacement_value *= strength;

		return vec3(displacement_value, displacement_value, displacement_value);
	}

	double get_alpha(const vec3& uv) const override {

		return 1;
	}

	~displacement_map() {
		// 手动释放空间
		for (int i = 0; i < rows; i++) {
			delete[] displacement_mat[i];
		}
		delete[] displacement_mat;
	}
};

#endif