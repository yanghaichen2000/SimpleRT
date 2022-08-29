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


// 长方体类
class cuboid : public mesh_triangle {
public:
	// x, y, z轴的长度
	double length[3];
	// material指针
	shared_ptr<material> mat_ptr;

	// 变换，按先后顺序排列
	vec3 scale_vec;
	vec3 rotate_vec; // 分别为绕x,y,z轴旋转的弧度
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


// 从.obj文件中读取
// 读取顶点位置、uv和法线
// 顶点坐标根据自定blender坐标进行变换
class simple_obj_mesh : public mesh_triangle
{
public:
	// material指针
	shared_ptr<material> mat_ptr;

	// 变换，按先后顺序排列
	vec3 scale_vec;
	vec3 rotate_vec; // 分别为绕x,y,z轴旋转的弧度
	vec3 translate_vec;

	// OBJLoader的读取结果
	vector<objl::Mesh> mesh_list;
	

public:
	simple_obj_mesh(const char file_name[], shared_ptr<material> mat_ptr_init)
	{
		objl::Loader loader;
		loader.LoadFile(file_name);

		// 获取mesh列表
		// 一个mesh对应blender中一个独立的网格，其中包含一系列顶点和索引信息
		mesh_list = loader.LoadedMeshes;

		// 设置基本成员变量
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
		// 初始化返回值
		vector<shared_ptr<triangle>> triangle_ptr_list;
		
		// 计算三角形个数
		int num_triangle = 0;
		for (objl::Mesh mesh : mesh_list) {
			num_triangle += mesh.Indices.size() / 3;
		}
		// 预先设定triangle_ptr_list长度
		triangle_ptr_list.resize(num_triangle);
		
		// 对每个mesh分别进行读取
		int triangle_ptr_index = 0;
		for (objl::Mesh mesh : mesh_list) {
			
			// 首先记录顶点信息
			// 将顶点位置转换为vec3格式，存放在position_list中
			vector<vec3> position_list;
			// 将顶点uv坐标转换为vec3格式，存放在uv_list中
			vector<vec3> uv_list;
			// 将顶点法线转换为vec3格式，存放在normal_list中
			vector<vec3> normal_list;

			// 预先设定position_list，uv_list和normal_list的长度
			int vertex_num = mesh.Vertices.size();
			position_list.resize(vertex_num);
			uv_list.resize(vertex_num);
			normal_list.resize(vertex_num);
			
			for (int index = 0; index < vertex_num; index++) {
				// 记录顶点位置时，根据自定的blender对应坐标系进行坐标变换
				// blender中的y对应这里的x
				// blender中的z对应这里的y
				// blender中的x对应这里的z
				// 但是直接使用上述映射没有得到正确的结果，所以实际代码使用了一个逆天的hack，暂时不知道原因
				//position_list[index] = vec3(mesh.Vertices[index].Position.Y, mesh.Vertices[index].Position.Z, mesh.Vertices[index].Position.X); // 理论正确
				position_list[index] = vec3(-mesh.Vertices[index].Position.Z, mesh.Vertices[index].Position.Y, mesh.Vertices[index].Position.X); // 实际正确
				// 应用mesh_triangle自带的变换
				position_list[index].scale(scale_vec).rotate(rotate_vec).translate(translate_vec);

				// 对normal使用相同的hack
				normal_list[index] = vec3(-mesh.Vertices[index].Normal.Z, mesh.Vertices[index].Normal.Y, mesh.Vertices[index].Normal.X);
				// 应用mesh_triangle自带的变换（不使用translate，且scale变换是顶点位置scale变换的逆）
				normal_list[index].scale(vec3(1 / scale_vec[0], 1 / scale_vec[1], 1 / scale_vec[2])).rotate(rotate_vec);

				// 记录uv
				uv_list[index] = vec3(mesh.Vertices[index].TextureCoordinate.X, mesh.Vertices[index].TextureCoordinate.Y, 0);
			}
			
			// 生成三角形，并放入triangle_ptr_list
			int index_num = mesh.Indices.size();
			// 每三个index一组对应一个三角形
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