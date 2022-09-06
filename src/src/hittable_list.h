#pragma once
#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"

#include <memory>
#include <vector>
#include <algorithm>
#include "mesh_triangle.h"

using std::shared_ptr;
using std::make_shared;

// 构建bvh_node时需要使用的比较函数
inline bool box_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis) {
	bounds3 box_a = a->bounds();
	bounds3 box_b = b->bounds();

	return box_a.pMin[axis] < box_b.pMin[axis];
}

bool box_x_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
	return box_compare(a, b, 0);
}

bool box_y_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
	return box_compare(a, b, 1);
}

bool box_z_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
	return box_compare(a, b, 2);
}

// 定义comparator_t，这是一个函数指针，可以是上面三个函数中任意一个
typedef bool(*comparator_t)(const shared_ptr<hittable>, const shared_ptr<hittable>);


// 存放hittable对象的列表
class hittable_list : public hittable {
public:
	// 所有object的指针存放在vector中
	std::vector<shared_ptr<hittable>> objects;

public:
	hittable_list() {}
	hittable_list(shared_ptr<hittable> object) { add(object); }

	void clear() { objects.clear(); }

	// 增加一个hittable类型object
	void add(shared_ptr<hittable> object) { 
		objects.push_back(object);
	}


	// 增加一个mesh_triangle
	void add(shared_ptr<mesh_triangle> mesh_ptr) {
		vector<shared_ptr<triangle>> triangle_ptr_list = mesh_ptr->unpack();
		// 将列表中三角形逐个加入hittable_list中
		for (shared_ptr<hittable> triangle_ptr : triangle_ptr_list) {
			add(triangle_ptr);
		}
	}


	// 获取光线与hittable_list最近的交点
	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
		hit_record temp_rec;
		bool hit_anything = false;
		auto closest_so_far = t_max;

		for (const auto& object : objects) {
			if (object->hit(r, t_min, closest_so_far, temp_rec)) {
				hit_anything = true;
				// 通过将t_max设置为当前交点的t来找到最小的t
				closest_so_far = temp_rec.t;
				rec = temp_rec;
			}
		}

		return hit_anything;
	}

	virtual bounds3 bounds() const override {
		bounds3 box = objects[0]->bounds();
		int len = objects.size();

	    // 从第二个元素开始不断合并包围盒
		bounds3 box_tmp;
		for (int i = 1; i < len; i++) {
			box_tmp = objects[i]->bounds();
			box = Union(box, box_tmp);
		}

		return box;
	}
};



#endif