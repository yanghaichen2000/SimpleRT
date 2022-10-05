#pragma once
#ifndef BVH_NODE_H
#define BVH_NODE_H

#include "hittable.h"
#include "hittable_list.h"
#include "bounds.h"
#include "global.h"


class bvh_node : public hittable {
public:
	// 总节点个数
	static int node_num;
	// 子节点指针
	shared_ptr<hittable> left, right;
	// 包围盒
	bounds3 box;

public:
	bvh_node() {};

	// 根据hittable指针列表初始化bvh_node
	// 这里的objects可以直接输入hittable_list的同名成员变量
	// start和end给出了该bvh_node需要包含的objects索引范围(不包含end)，从根节点到叶子结点这个范围逐渐变小
	bvh_node(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end) {
		node_num++;
		
		// 确定需要分割的坐标轴
		int axis = random_int_012();

		// 分配对应的图元比较器用于排序
		comparator_t comparator = box_x_compare;
		switch (axis) {
		case 0: comparator = box_x_compare; break;
		case 1: comparator = box_y_compare; break;
		case 2: comparator = box_z_compare; break;
		}

		// 当前bvh_node中包含的object个数
		size_t object_span = end - start;

		// 开始构建子节点
		if (object_span == 1) { // 该node中只有一个object，子节点为相同的object
			left = right = objects[start];
		}
		else if (object_span == 2) { // 该node中只有两个object，分别分配到左右子节点
			if (comparator(objects[start], objects[start + 1])) {
				left = objects[start];
				right = objects[start + 1];
			}
			else {
				left = objects[start + 1];
				right = objects[start];
			}
		}
		else { // 该node中有多于两个object，则创建两个bvh_node作为子节点

			// 对范围内的object进行排序
			std::sort(objects.begin() + start, objects.begin() + end, comparator);

			// 将范围再分成部分，分别创建左右子节点
			size_t mid = start + object_span / 2;
			left = make_shared<bvh_node>(objects, start, mid);
			right = make_shared<bvh_node>(objects, mid, end);
		}

		// 该节点的bounds是左右子节点bounds的并
		box = Union(left->bounds(), right->bounds());
	}

	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
		// 如果该node的bounds与光线没有交点，直接返回false
		if (not box.hit(r, t_min, t_max))
			return false;

		// 检查是否与左子节点有交点
		bool hit_left = left->hit(r, t_min, t_max, rec);
		// 检查是否与右子节点有交点，此时限定t必须要比左子节点小
		bool hit_right = right->hit(r, t_min, hit_left ? rec.t : t_max, rec);

		return hit_left or hit_right;
	}

	virtual bounds3 bounds() const override {
		return box;
	}
};

int bvh_node::node_num = 0;

// 生成bvh，并返回根节点
shared_ptr<hittable> generate_bvh(hittable_list &list) {
	shared_ptr<hittable> root = make_shared<bvh_node>(list.objects, 0, list.objects.size());
	return root;
}


#endif