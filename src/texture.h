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

// ��ɫtexture
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

// ͼ��texture
class color_map : public texture {
public:
	// ���ͼ��RGBֵ(0.0~1.0)�ľ���
	// ���Կռ�
	// �������ϣ���������
	vec3 **color_mat;
	// ͼ������������
	int rows, cols; 
	// �Ŵ���
	double scale = 1;

public:
	// scaleԽ������Խ�ܼ�
	color_map(const char file_name[], double scale_init = 1) {
		scale = scale_init;
		
		// ��ȡͼ��
		cv::Mat image;
		image = cv::imread(file_name);

		// ��ȡͼ��ߴ�
		rows = image.rows;
		cols = image.cols;

		// �����洢�ռ�
		color_mat = new vec3*[rows];
		for (int i = 0; i < rows; i++) {
			color_mat[i] = new vec3[cols];
		}

		// ��¼����
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				// opencv����ɫ�洢˳��ΪBGR��������Ҫ��תһ��˳��
				// ����ע��Ҫ��RGBֵת�������Կռ�
				color_mat[i][j][0] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[2]) / 255.0, 2.2);
				color_mat[i][j][1] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[1]) / 255.0, 2.2);
				color_mat[i][j][2] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[0]) / 255.0, 2.2);
			}
		}
	}

	virtual vec3 get_value(const vec3 &uv) const override
	{
		// Ԥ����uv���꣬ʹ�䷶Χ��[0,1)
		vec3 real_uv = uv * scale;
		real_uv[0] = real_uv[0] - std::floor(real_uv[0]);
		real_uv[1] = real_uv[1] - std::floor(real_uv[1]);

		// ��uvֵӳ�䵽��������
		// ӳ����ֵ��ΧΪ[0, cols - 1]��[0, rows - 1]
		double row = real_uv[1] * (rows - 1);
		double col = real_uv[0] * (cols - 1);

		// ��������ĸ����ص�����
		int row_0 = std::floor(row);
		int row_1 = row_0 + 1;
		int col_0 = std::floor(col);
		int col_1 = col_0 + 1;

		// �����ϲ�ֵ
		vec3 RGB_left = color_mat[row_1][col_0] * (row - row_0) + color_mat[row_0][col_0] * (row_1 - row);
		vec3 RGB_right = color_mat[row_1][col_1] * (row - row_0) + color_mat[row_0][col_1] * (row_1 - row);

		// �����ϲ�ֵ
		vec3 RGB = RGB_right * (col - col_0) + RGB_left * (col_1 - col);

		return RGB;
	}

	~color_map() {
		// �ֶ��ͷſռ�
		for (int i = 0; i < rows; i++) {
			delete[] color_mat[i];
		}
		delete[] color_mat;
	}
};


// ������ͼ
class normal_map : public texture {
public:
	// ���ͼ��RGBֵ(0.0~1.0)�ľ���
	// ���Կռ�
	// �������ϣ���������
	vec3 **normal_mat;
	// ͼ������������
	int rows, cols;
	// �Ŵ���
	double scale = 1;

public:
	// scaleԽ������Խ�ܼ�
	normal_map(const char file_name[], double scale_init = 1) {
		scale = scale_init;
		
		// ��ȡͼ��
		cv::Mat image;
		image = cv::imread(file_name);

		// ��ȡͼ��ߴ�
		rows = image.rows;
		cols = image.cols;

		// �����洢�ռ�
		normal_mat = new vec3*[rows];
		for (int i = 0; i < rows; i++) {
			normal_mat[i] = new vec3[cols];
		}

		// ��¼����
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				// opencv����ɫ�洢˳��ΪBGR��������Ҫ��תһ��˳��
				// �����ݴ�[0, 255]ӳ�䵽[-1, 1]
				// TODO:Ŀǰ��ӳ�䷽���������㵥λ��������֮�������Ҫ�޸�
				normal_mat[i][j][0] = int(image.at<cv::Vec3b>(rows - i - 1, j)[2]) / 255.0 * 2 - 1; // T����
				normal_mat[i][j][1] = int(image.at<cv::Vec3b>(rows - i - 1, j)[1]) / 255.0 * 2 - 1; // B����
				normal_mat[i][j][2] = int(image.at<cv::Vec3b>(rows - i - 1, j)[0]) / 255.0 * 2 - 1; // N����
				// ��λ��
				normal_mat[i][j] = unit_vector(normal_mat[i][j]);
			}
		}
	}

	virtual vec3 get_value(const vec3 &uv) const override
	{
		// Ԥ����uv���꣬ʹ�䷶Χ��[0,1)
		vec3 real_uv = uv * scale;
		real_uv[0] = real_uv[0] - std::floor(real_uv[0]);
		real_uv[1] = real_uv[1] - std::floor(real_uv[1]);

		// ��uvֵӳ�䵽��������
		// ӳ����ֵ��ΧΪ[0, cols - 1]��[0, rows - 1]
		double row = real_uv[1] * (rows - 1);
		double col = real_uv[0] * (cols - 1);

		// ��������ĸ����ص�����
		int row_0 = std::floor(row);
		int row_1 = row_0 + 1;
		int col_0 = std::floor(col);
		int col_1 = col_0 + 1;

		// �����ϲ�ֵ
		vec3 RGB_left = normal_mat[row_1][col_0] * (row - row_0) + normal_mat[row_0][col_0] * (row_1 - row);
		vec3 RGB_right = normal_mat[row_1][col_1] * (row - row_0) + normal_mat[row_0][col_1] * (row_1 - row);

		// �����ϲ�ֵ
		vec3 RGB = RGB_right * (col - col_0) + RGB_left * (col_1 - col);

		// ��λ��
		RGB = unit_vector(RGB);

		return RGB;
	}

	~normal_map() {
		// �ֶ��ͷſռ�
		for (int i = 0; i < rows; i++) {
			delete[] normal_mat[i];
		}
		delete[] normal_mat;
	}
};

#endif