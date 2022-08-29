#pragma once
#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <array>
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "texture.h"
using std::array;


class triangle : public hittable {
public:
	vec3 vertex[3]; // ��������
	vec3 uv[3]; // ����ռ����꣬ʹ��xy����Ϊuv
	shared_ptr<material> mat_ptr; // material
	vec3 vertex_normal[3]; // ���㷨��
	vec3 tangent; // ���ߡ�ʵ�ַ�����ͼʱ��Ҫ�õ�����Ϣ

public:
	// ʹ�����������λ�úͷ����Լ�һ��material��ʼ��
	triangle(const vec3 &vertex_1, const vec3 &vertex_2, const vec3 &vertex_3, 
		const shared_ptr<material> &mat_ptr_all, 
		const vec3 &uv_1, const vec3 &uv_2, const vec3 &uv_3, 
		const vec3 &vertex_normal_1, const vec3 &vertex_normal_2, const vec3 &vertex_normal_3) {
		
		// ���û�������
		vertex[0] = vertex_1;
		vertex[1] = vertex_2;
		vertex[2] = vertex_3;

		mat_ptr = mat_ptr_all;

		uv[0] = uv_1;
		uv[1] = uv_2;
		uv[2] = uv_3;
		
		// ֱ�Ӽ�¼����Ķ��㷨��
		vertex_normal[0] = vertex_normal_1;
		vertex_normal[1] = vertex_normal_2;
		vertex_normal[2] = vertex_normal_3;

		// ��������
		// ԭ����Բο�zhuanlan.zhihu.com/p/414940275
		vec3 AB = vertex[1] - vertex[0];
		vec3 AC = vertex[2] - vertex[0];
		double du1 = uv[2][0] - uv[0][0];
		double du2 = uv[1][0] - uv[0][0];
		double dv1 = uv[2][1] - uv[0][1];
		double dv2 = uv[1][1] - uv[0][1];
		// �õ����ߣ�δ��������
		tangent = (dv1 * AB - dv2 * AC) / (du2 * dv1 - du1 * dv2);
		tangent = unit_vector(tangent);
	}

	// ʹ�����������λ�ã��޷��ߣ��Լ�һ��material��ʼ��
	triangle(const vec3 &vertex_1, const vec3 &vertex_2, const vec3 &vertex_3,
		const shared_ptr<material> &mat_ptr_all) {

		// ���û�������
		vertex[0] = vertex_1;
		vertex[1] = vertex_2;
		vertex[2] = vertex_3;

		mat_ptr = mat_ptr_all;

		// ��֧��uv
		uv[0] = vec3(0, 0, 0);
		uv[1] = vec3(0, 0, 0);
		uv[2] = vec3(0, 0, 0);

		// ���ݶ���λ�ü��㷨�ߣ�����ϵ��
		vertex_normal[0] = unit_vector(cross(vertex[1] - vertex[0], vertex[2] - vertex[0]));
		vertex_normal[1] = vertex_normal[0];
		vertex_normal[2] = vertex_normal[0];

		// �ù��캯����֧��uv�������ò��˷�����ͼ�����Բ���Ҫ��������
		tangent = vec3(0, 0, 0);
	}

	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
		// ����t�Լ��������꣬ͬʱ�жϹ������������Ƿ��ཻ
		// ����0,1,2��Ȩ�طֱ�Ϊ(1-b1-b2),b1,b2
		vec3 E1 = vertex[1] - vertex[0];
		vec3 E2 = vertex[2] - vertex[0];
		vec3 S = r.orig - vertex[0];
		vec3 S1 = cross(r.dir, E2);
		vec3 S2 = cross(S, E1);
		double S1E1_inv = 1 / dot(S1, E1);
		double t = dot(S2, E2) * S1E1_inv;
		if (t < t_min or t > t_max) return false; // t���趨��Χ֮������Ϊû�н���
		double b1 = dot(S1, S) * S1E1_inv;
		double b2 = dot(S2, r.dir) * S1E1_inv;
		if (b1 < 0 or b2 < 0 or b1 + b2 > 1) return false; // ��������������Ҳ�޽���

		// �����������������������
		double w0 = 1 - b1 - b2;
		double w1 = std::move(b1);
		double w2 = std::move(b2);

		// ��¼����
		rec.t = t;

		// ���㽻������
		rec.p = r.orig + t * r.dir;

		// ����uv
		rec.uv = w0 * uv[0] + w1 * uv[1] + w2 * uv[2];

		// ���㷨�ߣ�
		// ����ʹ��ƽ����ɫ�����ݶ��㷨�߲�ֵ�õ�shading point����
		vec3 normal = w0 * vertex_normal[0] + w1 * vertex_normal[1] + w2 * vertex_normal[2];
		normal = unit_vector(normal);

		// ������ڷ�����ͼ��ʹ�÷�����ͼ�Է��߽��д���
		if (mat_ptr->get_normal_map_ptr() != nullptr) {
			// ���ݳ�Ա����tangent�����������ҵ�λ�����߻�����
			vec3 basis_t = unit_vector(tangent - dot(tangent, normal) * normal);
			// ���㸱���߻�����
			vec3 basis_b = cross(basis_t, normal);
			// �ӷ�����ͼ��ȡ���߿ռ�����
			vec3 tangent_coord = mat_ptr->get_normal_map_ptr()->get_value(rec.uv);
			// ���㷨��
			normal = basis_t * tangent_coord[0] + basis_b * tangent_coord[1] + normal * tangent_coord[2];
		}

		rec.set_face_normal(r, normal);

		// ����texture�е�material�������޸�
		rec.mat_ptr = mat_ptr;

		return true;
	}

	virtual bounds3 bounds() const override {
		bounds3 ret(vertex[0], vertex[1]);
		ret = Union(ret, vertex[2]);

		// ��Ϊ�����ο��ܺ�����ƽ��ƽ�У����Խ���Χ������һ�㣬�������׵����󽻽���ض�Ϊfalse
		return Expand(ret, 0.00000001);
	}
};

#endif