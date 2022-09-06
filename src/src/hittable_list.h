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

// ����bvh_nodeʱ��Ҫʹ�õıȽϺ���
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

// ����comparator_t������һ������ָ�룬������������������������һ��
typedef bool(*comparator_t)(const shared_ptr<hittable>, const shared_ptr<hittable>);


// ���hittable������б�
class hittable_list : public hittable {
public:
	// ����object��ָ������vector��
	std::vector<shared_ptr<hittable>> objects;

public:
	hittable_list() {}
	hittable_list(shared_ptr<hittable> object) { add(object); }

	void clear() { objects.clear(); }

	// ����һ��hittable����object
	void add(shared_ptr<hittable> object) { 
		objects.push_back(object);
	}


	// ����һ��mesh_triangle
	void add(shared_ptr<mesh_triangle> mesh_ptr) {
		vector<shared_ptr<triangle>> triangle_ptr_list = mesh_ptr->unpack();
		// ���б����������������hittable_list��
		for (shared_ptr<hittable> triangle_ptr : triangle_ptr_list) {
			add(triangle_ptr);
		}
	}


	// ��ȡ������hittable_list����Ľ���
	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
		hit_record temp_rec;
		bool hit_anything = false;
		auto closest_so_far = t_max;

		for (const auto& object : objects) {
			if (object->hit(r, t_min, closest_so_far, temp_rec)) {
				hit_anything = true;
				// ͨ����t_max����Ϊ��ǰ�����t���ҵ���С��t
				closest_so_far = temp_rec.t;
				rec = temp_rec;
			}
		}

		return hit_anything;
	}

	virtual bounds3 bounds() const override {
		bounds3 box = objects[0]->bounds();
		int len = objects.size();

	    // �ӵڶ���Ԫ�ؿ�ʼ���Ϻϲ���Χ��
		bounds3 box_tmp;
		for (int i = 1; i < len; i++) {
			box_tmp = objects[i]->bounds();
			box = Union(box, box_tmp);
		}

		return box;
	}
};



#endif