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
	int texture_id;
	vec3 vertex[3]; // ��������
	vec3 uv[3]; // �����ռ����꣬ʹ��xy����Ϊuv
	shared_ptr<material> mat_ptr; // material
	vec3 normal; // �����������ķ���

public:
	// ʹ�����������һ��material��ʼ��
	// ͬʱҲ���Դ������������uv
	triangle(const vec3 &vertex_1, const vec3 &vertex_2, const vec3 &vertex_3, const shared_ptr<material> &mat_ptr_all, 
		const vec3 &uv_1 = vec3(), const vec3 &uv_2 = vec3(), const vec3 &uv_3 = vec3(), int texture_id_init = -1) {
		
		// ���û�������
		
		vertex[0] = vertex_1;
		vertex[1] = vertex_2;
		vertex[2] = vertex_3;
		mat_ptr = mat_ptr_all;
		uv[0] = uv_1;
		uv[1] = uv_2;
		uv[2] = uv_3;
		texture_id = texture_id_init;

		// ���㸨������
		
		// ���ߣ�����ϵ
		normal = unit_vector(cross(vertex[1] - vertex[0], vertex[2] - vertex[0]));
	}

	// �������û�б�const����Ϊ��map[]������ͻ��ʵ���ϸú���������ı��Ա����
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

		// ���㷨��
		rec.set_face_normal(r, normal);

		// ����texture�е�material�������޸�
		rec.mat_ptr = mat_ptr;

		// �����Լ���ָ��
		rec.texture_id = texture_id;

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