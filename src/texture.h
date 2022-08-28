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

public:
	color_map(const char file_name[]) {
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
				color_mat[i][j][0] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[2]) / 256.0, 2.2);
				color_mat[i][j][1] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[1]) / 256.0, 2.2);
				color_mat[i][j][2] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[0]) / 256.0, 2.2);
			}
		}
	}

	virtual vec3 get_value(const vec3 &uv) const override
	{
		// Ԥ����uv���꣬ʹ�䷶Χ��[0,1)
		vec3 real_uv(0, 0, 0);
		real_uv[0] = uv[0] - std::floor(uv[0]);
		real_uv[1] = uv[1] - std::floor(uv[1]);

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

#endif