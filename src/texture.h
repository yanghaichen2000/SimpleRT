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

// ��ɫtexture
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

// ͼ��texture
class color_map : public texture {
public:
	// ���ͼ��RGBֵ(0.0~1.0)�ľ���
	// ���Կռ�
	// �������ϣ���������
	vec3 **color_mat;
	// ���ͼ��alphaֵ(0.0~1.0)�ľ���
	// 0Ϊ��ȫ͸����1Ϊ��ȫ��͸��
	// ��color_mat��Ԫ�ض�Ӧ
	double **alpha_mat;
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
		image = cv::imread(file_name, -1); // �ڶ�������Ϊ-1����ʾ��ȡ͸��ͨ��

		// ��ȡͼ��ߴ�
		rows = image.rows;
		cols = image.cols;

		// �����洢�ռ�
		color_mat = new vec3*[rows];
		for (int i = 0; i < rows; i++) {
			color_mat[i] = new vec3[cols];
		}
		alpha_mat = new double*[rows];
		for (int i = 0; i < rows; i++) {
			alpha_mat[i] = new double[cols];
		}

		// ��¼����
		// �����Ϊ����������д���
		// ������ͨ��ͼƬ��������cv::Vec3b����������õ������ݻ����
		// ������ͨ��ͼƬ��������cv::Vec4b�����������޷���ȡalphaͨ��
		if (image.channels() == 3) {
			for (int i = 0; i < rows; i++) {
				for (int j = 0; j < cols; j++) {
					// opencv����ɫ�洢˳��ΪBGR��������Ҫ��תһ��˳��
					// ����ע��Ҫ��RGBֵת�������Կռ�
					color_mat[i][j][0] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[2]) / 255.0, 2.2);
					color_mat[i][j][1] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[1]) / 255.0, 2.2);
					color_mat[i][j][2] = pow(int(image.at<cv::Vec3b>(rows - i - 1, j)[0]) / 255.0, 2.2);
					alpha_mat[i][j] = 1; // 3ͨ����alpha��Ϊ1
				}
			}
		} 
		else if (image.channels() == 4) {
			for (int i = 0; i < rows; i++) {
				for (int j = 0; j < cols; j++) {
					// opencv����ɫ�洢˳��ΪBGR��������Ҫ��תһ��˳��
					// ����ע��Ҫ��RGBֵת�������Կռ�
					color_mat[i][j][0] = pow(int(image.at<cv::Vec4b>(rows - i - 1, j)[2]) / 255.0, 2.2);
					color_mat[i][j][1] = pow(int(image.at<cv::Vec4b>(rows - i - 1, j)[1]) / 255.0, 2.2);
					color_mat[i][j][2] = pow(int(image.at<cv::Vec4b>(rows - i - 1, j)[0]) / 255.0, 2.2);
					alpha_mat[i][j] = int(image.at<cv::Vec4b>(rows - i - 1, j)[3]) / 255.0;
				}
			}
		}
		else { // ͨ��������3Ҳ����4
			std::cout << "error in texture.h: color_map::color_map(): unsupported image channels = " << image.channels() << "\n";
			std::cout << "file name : " << file_name << '\n';
			exit(-1);
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
		int row_0 = static_cast<int>(std::floor(row));
		int row_1 = row_0 + 1;
		int col_0 = static_cast<int>(std::floor(col));
		int col_1 = col_0 + 1;

		// �����ϲ�ֵ
		vec3 RGB_left = color_mat[row_1][col_0] * (row - row_0) + color_mat[row_0][col_0] * (row_1 - row);
		vec3 RGB_right = color_mat[row_1][col_1] * (row - row_0) + color_mat[row_0][col_1] * (row_1 - row);

		// �����ϲ�ֵ
		vec3 RGB = RGB_right * (col - col_0) + RGB_left * (col_1 - col);

		return RGB;
	}

	double get_alpha(const vec3 &uv) const override {

		// Ԥ����uv���꣬ʹ�䷶Χ��[0,1)
		vec3 real_uv = uv * scale;
		real_uv[0] = real_uv[0] - std::floor(real_uv[0]);
		real_uv[1] = real_uv[1] - std::floor(real_uv[1]);

		// ��uvֵӳ�䵽��������
		// ӳ����ֵ��ΧΪ[0, cols - 1]��[0, rows - 1]
		double row = real_uv[1] * (rows - 1);
		double col = real_uv[0] * (cols - 1);

		// ��������ĸ����ص�����
		int row_0 = static_cast<int>(std::floor(row));
		int row_1 = row_0 + 1;
		int col_0 = static_cast<int>(std::floor(col));
		int col_1 = col_0 + 1;

		// �����ϲ�ֵ
		double alpha_left = alpha_mat[row_1][col_0] * (row - row_0) + alpha_mat[row_0][col_0] * (row_1 - row);
		double alpha_right = alpha_mat[row_1][col_1] * (row - row_0) + alpha_mat[row_0][col_1] * (row_1 - row);

		// �����ϲ�ֵ
		double alpha = alpha_right * (col - col_0) + alpha_left * (col_1 - col);
		
		return alpha;
	}

	~color_map() {
		// �ֶ��ͷſռ�
		for (int i = 0; i < rows; i++) {
			delete[] color_mat[i];
			delete[] alpha_mat[i];
		}
		delete[] color_mat;
		delete[] alpha_mat;
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
		int row_0 = static_cast<int>(std::floor(row));
		int row_1 = row_0 + 1;
		int col_0 = static_cast<int>(std::floor(col));
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

	double get_alpha(const vec3 &uv) const override {
		return 1;
	}

	~normal_map() {
		// �ֶ��ͷſռ�
		for (int i = 0; i < rows; i++) {
			delete[] normal_mat[i];
		}
		delete[] normal_mat;
	}
};

// �û���ͼ
class displacement_map : public texture {
public:
	// ���λ�����ľ���
	// �������ϣ���������
	double** displacement_mat;
	// ͼ������������
	int rows, cols;
	// �Ŵ���
	double scale;
	// ǿ��
	double strength;

public:
	// scaleԽ������Խ�ܼ�
	displacement_map(const char file_name[], double scale_init = 1, double strength_init = 0.02) {
		scale = scale_init;
		strength = strength_init;

		// ��ȡͼ��
		cv::Mat image;
		image = cv::imread(file_name, -1); // �ڶ�������Ϊ-1����ʾ��ȡ͸��ͨ��
		  
		// ��ȡͼ��ߴ�
		rows = image.rows;
		cols = image.cols;

		// �����洢�ռ�
		displacement_mat = new double *[rows];
		for (int i = 0; i < rows; i++) {
			displacement_mat[i] = new double[cols];
		}

		// ��¼����
		// �����Ϊ����������д���
		// ������ͨ��ͼƬ��������cv::Vec3b����������õ������ݻ����
		// ������ͨ��ͼƬ��������cv::Vec4b�����������޷���ȡalphaͨ��
		if (image.channels() == 3) {
			for (int i = 0; i < rows; i++) {
				for (int j = 0; j < cols; j++) {
					// opencv����ɫ�洢˳��ΪBGR��������Ҫ��תһ��˳��
					// ����ע��Ҫ��RGBֵת�������Կռ�
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
					// opencv����ɫ�洢˳��ΪBGR��������Ҫ��תһ��˳��
					// ����ע��Ҫ��RGBֵת�������Կռ�
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
					// opencv����ɫ�洢˳��ΪBGR��������Ҫ��תһ��˳��
					// ����ע��Ҫ��RGBֵת�������Կռ�
					displacement_mat[i][j] = int(image.at<cv::uint8_t>(rows - i - 1, j)) / 255.0 * 2 - 1;
				}
			}
		}
		else { // ͨ��������3Ҳ����4
			std::cout << "error in texture.h: color_map::color_map(): unsupported image channels = " << image.channels() << "\n";
			std::cout << "file name : " << file_name << '\n';
			exit(-1);
		}
	}

	virtual vec3 get_value(const vec3& uv) const override
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
		int row_0 = static_cast<int>(std::floor(row));
		int row_1 = row_0 + 1;
		int col_0 = static_cast<int>(std::floor(col));
		int col_1 = col_0 + 1;

		// �����ϲ�ֵ
		double displacement_left = displacement_mat[row_1][col_0] * (row - row_0) + displacement_mat[row_0][col_0] * (row_1 - row);
		double displacement_right = displacement_mat[row_1][col_1] * (row - row_0) + displacement_mat[row_0][col_1] * (row_1 - row);

		// �����ϲ�ֵ
		double displacement_value = displacement_right * (col - col_0) + displacement_left * (col_1 - col);
		displacement_value *= strength;

		return vec3(displacement_value, displacement_value, displacement_value);
	}

	double get_alpha(const vec3& uv) const override {

		return 1;
	}

	~displacement_map() {
		// �ֶ��ͷſռ�
		for (int i = 0; i < rows; i++) {
			delete[] displacement_mat[i];
		}
		delete[] displacement_mat;
	}
};

#endif