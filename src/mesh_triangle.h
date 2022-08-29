#pragma once
#ifndef MESH_TRIANGLE_H
#define MESH_TRIANGLE_H

#include "material.h"
#include "triangle.h"
#include "OBJ_Loader.h"

using std::vector;

class mesh_triangle {
public:
	virtual vector<shared_ptr<triangle>> unpack() const = 0;
	virtual void set_scale(const vec3 &vec) = 0;
	virtual void set_rotate(const vec3 &vec) = 0;
	virtual void set_translate(const vec3 &vec) = 0;
};


// ��������
class cuboid : public mesh_triangle {
public:
	// x, y, z��ĳ���
	double length[3];
	// materialָ��
	shared_ptr<material> mat_ptr;

	// �任�����Ⱥ�˳������
	vec3 scale_vec;
	vec3 rotate_vec; // �ֱ�Ϊ��x,y,z����ת�Ļ���
	vec3 translate_vec;

public:
	cuboid(double length_x, double length_y, double length_z, shared_ptr<material> mat_ptr_init) {
		length[0] = length_x;
		length[1] = length_y;
		length[2] = length_z;
		mat_ptr = mat_ptr_init;
		scale_vec = vec3(1, 1, 1);
		rotate_vec = vec3(0, 0, 0);
		translate_vec = vec3(0, 0, 0);
	}

	void set_scale(const vec3 &vec) override {
		scale_vec = vec;
	}

	void set_rotate(const vec3 &vec) override {
		rotate_vec = vec;
	}

	void set_translate(const vec3 &vec) override {
		translate_vec = vec;
	}

	virtual vector<shared_ptr<triangle>> unpack() const override {
		vec3 vertex_1 = vec3(-0.5 * length[0], -0.5 * length[1], -0.5 * length[2]).scale(scale_vec).rotate(rotate_vec).translate(translate_vec);
		vec3 vertex_2 = vec3(0.5 * length[0], -0.5 * length[1], -0.5 * length[2]).scale(scale_vec).rotate(rotate_vec).translate(translate_vec);
		vec3 vertex_3 = vec3(0.5 * length[0], -0.5 * length[1], 0.5 * length[2]).scale(scale_vec).rotate(rotate_vec).translate(translate_vec);
		vec3 vertex_4 = vec3(-0.5 * length[0], -0.5 * length[1], 0.5 * length[2]).scale(scale_vec).rotate(rotate_vec).translate(translate_vec);
		vec3 vertex_5 = vec3(-0.5 * length[0], 0.5 * length[1], -0.5 * length[2]).scale(scale_vec).rotate(rotate_vec).translate(translate_vec);
		vec3 vertex_6 = vec3(0.5 * length[0], 0.5 * length[1], -0.5 * length[2]).scale(scale_vec).rotate(rotate_vec).translate(translate_vec);
		vec3 vertex_7 = vec3(0.5 * length[0], 0.5 * length[1], 0.5 * length[2]).scale(scale_vec).rotate(rotate_vec).translate(translate_vec);
		vec3 vertex_8 = vec3(-0.5 * length[0], 0.5 * length[1], 0.5 * length[2]).scale(scale_vec).rotate(rotate_vec).translate(translate_vec);

		vector<shared_ptr<triangle>> triangle_ptr_list;
		triangle_ptr_list.resize(12);

		triangle_ptr_list[0] = make_shared<triangle>(vertex_1, vertex_2, vertex_3, mat_ptr);
		triangle_ptr_list[1] = make_shared<triangle>(vertex_1, vertex_3, vertex_4, mat_ptr);
		triangle_ptr_list[2] = make_shared<triangle>(vertex_2, vertex_5, vertex_6, mat_ptr);
		triangle_ptr_list[3] = make_shared<triangle>(vertex_1, vertex_5, vertex_2, mat_ptr);
		triangle_ptr_list[4] = make_shared<triangle>(vertex_1, vertex_4, vertex_8, mat_ptr);
		triangle_ptr_list[5] = make_shared<triangle>(vertex_1, vertex_8, vertex_5, mat_ptr);
		triangle_ptr_list[6] = make_shared<triangle>(vertex_7, vertex_3, vertex_2, mat_ptr);
		triangle_ptr_list[7] = make_shared<triangle>(vertex_6, vertex_7, vertex_2, mat_ptr);
		triangle_ptr_list[8] = make_shared<triangle>(vertex_8, vertex_6, vertex_5, mat_ptr);
		triangle_ptr_list[9] = make_shared<triangle>(vertex_8, vertex_7, vertex_6, mat_ptr);
		triangle_ptr_list[10] = make_shared<triangle>(vertex_4, vertex_3, vertex_8, mat_ptr);
		triangle_ptr_list[11] = make_shared<triangle>(vertex_3, vertex_7, vertex_8, mat_ptr);

		return triangle_ptr_list;
	}
};


// ��.obj�ļ��ж�ȡ
// ��ȡ����λ�á�uv�ͷ���
// ������������Զ�blender������б任
class simple_obj_mesh : public mesh_triangle
{
public:
	// materialָ��
	shared_ptr<material> mat_ptr;

	// �任�����Ⱥ�˳������
	vec3 scale_vec;
	vec3 rotate_vec; // �ֱ�Ϊ��x,y,z����ת�Ļ���
	vec3 translate_vec;

	// OBJLoader�Ķ�ȡ���
	vector<objl::Mesh> mesh_list;
	

public:
	simple_obj_mesh(const char file_name[], shared_ptr<material> mat_ptr_init)
	{
		objl::Loader loader;
		loader.LoadFile(file_name);

		// ��ȡmesh�б�
		// һ��mesh��Ӧblender��һ���������������а���һϵ�ж����������Ϣ
		mesh_list = loader.LoadedMeshes;

		// ���û�����Ա����
		mat_ptr = mat_ptr_init;
		scale_vec = vec3(1, 1, 1);
		rotate_vec = vec3(0, 0, 0);
		translate_vec = vec3(0, 0, 0);
	}

	void set_scale(const vec3 &vec) override {
		scale_vec = vec;
	}

	void set_rotate(const vec3 &vec) override {
		rotate_vec = vec;
	}

	void set_translate(const vec3 &vec) override {
		translate_vec = vec;
	}

	virtual vector<shared_ptr<triangle>> unpack() const override {
		// ��ʼ������ֵ
		vector<shared_ptr<triangle>> triangle_ptr_list;
		
		// ���������θ���
		int num_triangle = 0;
		for (objl::Mesh mesh : mesh_list) {
			num_triangle += mesh.Indices.size() / 3;
		}
		// Ԥ���趨triangle_ptr_list����
		triangle_ptr_list.resize(num_triangle);
		
		// ��ÿ��mesh�ֱ���ж�ȡ
		int triangle_ptr_index = 0;
		for (objl::Mesh mesh : mesh_list) {
			
			// ���ȼ�¼������Ϣ
			// ������λ��ת��Ϊvec3��ʽ�������position_list��
			vector<vec3> position_list;
			// ������uv����ת��Ϊvec3��ʽ�������uv_list��
			vector<vec3> uv_list;
			// �����㷨��ת��Ϊvec3��ʽ�������normal_list��
			vector<vec3> normal_list;

			// Ԥ���趨position_list��uv_list��normal_list�ĳ���
			int vertex_num = mesh.Vertices.size();
			position_list.resize(vertex_num);
			uv_list.resize(vertex_num);
			normal_list.resize(vertex_num);
			
			for (int index = 0; index < vertex_num; index++) {
				// ��¼����λ��ʱ�������Զ���blender��Ӧ����ϵ��������任
				// blender�е�y��Ӧ�����x
				// blender�е�z��Ӧ�����y
				// blender�е�x��Ӧ�����z
				// ����ֱ��ʹ������ӳ��û�еõ���ȷ�Ľ��������ʵ�ʴ���ʹ����һ�������hack����ʱ��֪��ԭ��
				//position_list[index] = vec3(mesh.Vertices[index].Position.Y, mesh.Vertices[index].Position.Z, mesh.Vertices[index].Position.X); // ������ȷ
				position_list[index] = vec3(-mesh.Vertices[index].Position.Z, mesh.Vertices[index].Position.Y, mesh.Vertices[index].Position.X); // ʵ����ȷ
				// Ӧ��mesh_triangle�Դ��ı任
				position_list[index].scale(scale_vec).rotate(rotate_vec).translate(translate_vec);

				// ��normalʹ����ͬ��hack
				normal_list[index] = vec3(-mesh.Vertices[index].Normal.Z, mesh.Vertices[index].Normal.Y, mesh.Vertices[index].Normal.X);
				// Ӧ��mesh_triangle�Դ��ı任����ʹ��translate����scale�任�Ƕ���λ��scale�任���棩
				normal_list[index].scale(vec3(1 / scale_vec[0], 1 / scale_vec[1], 1 / scale_vec[2])).rotate(rotate_vec);

				// ��¼uv
				uv_list[index] = vec3(mesh.Vertices[index].TextureCoordinate.X, mesh.Vertices[index].TextureCoordinate.Y, 0);
			}
			
			// ���������Σ�������triangle_ptr_list
			int index_num = mesh.Indices.size();
			// ÿ����indexһ���Ӧһ��������
			//std::cout << triangle_ptr_list.size() << ' ' << position_list.size() << '\n';
			for (int index = 0; index < index_num ; index += 3) {
				//std::cout << triangle_ptr_index << ' ' << index << '\n';
				triangle_ptr_list[triangle_ptr_index] = make_shared<triangle>(
					position_list[mesh.Indices[index]],
					position_list[mesh.Indices[index + 1]], 
					position_list[mesh.Indices[index + 2]],
					mat_ptr,
					uv_list[mesh.Indices[index]],
					uv_list[mesh.Indices[index + 1]],
					uv_list[mesh.Indices[index + 2]],
					normal_list[mesh.Indices[index]],
					normal_list[mesh.Indices[index + 1]],
					normal_list[mesh.Indices[index + 2]]);
				triangle_ptr_index++;
			}
		}

		return triangle_ptr_list;
	}
};

#endif