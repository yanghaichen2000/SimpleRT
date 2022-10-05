#pragma once
#ifndef BVH_NODE_H
#define BVH_NODE_H

#include "hittable.h"
#include "hittable_list.h"
#include "bounds.h"
#include "global.h"


class bvh_node : public hittable {
public:
	// �ܽڵ����
	static int node_num;
	// �ӽڵ�ָ��
	shared_ptr<hittable> left, right;
	// ��Χ��
	bounds3 box;

public:
	bvh_node() {};

	// ����hittableָ���б��ʼ��bvh_node
	// �����objects����ֱ������hittable_list��ͬ����Ա����
	// start��end�����˸�bvh_node��Ҫ������objects������Χ(������end)���Ӹ��ڵ㵽Ҷ�ӽ�������Χ�𽥱�С
	bvh_node(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end) {
		node_num++;
		
		// ȷ����Ҫ�ָ��������
		int axis = random_int_012();

		// �����Ӧ��ͼԪ�Ƚ�����������
		comparator_t comparator = box_x_compare;
		switch (axis) {
		case 0: comparator = box_x_compare; break;
		case 1: comparator = box_y_compare; break;
		case 2: comparator = box_z_compare; break;
		}

		// ��ǰbvh_node�а�����object����
		size_t object_span = end - start;

		// ��ʼ�����ӽڵ�
		if (object_span == 1) { // ��node��ֻ��һ��object���ӽڵ�Ϊ��ͬ��object
			left = right = objects[start];
		}
		else if (object_span == 2) { // ��node��ֻ������object���ֱ���䵽�����ӽڵ�
			if (comparator(objects[start], objects[start + 1])) {
				left = objects[start];
				right = objects[start + 1];
			}
			else {
				left = objects[start + 1];
				right = objects[start];
			}
		}
		else { // ��node���ж�������object���򴴽�����bvh_node��Ϊ�ӽڵ�

			// �Է�Χ�ڵ�object��������
			std::sort(objects.begin() + start, objects.begin() + end, comparator);

			// ����Χ�ٷֳɲ��֣��ֱ𴴽������ӽڵ�
			size_t mid = start + object_span / 2;
			left = make_shared<bvh_node>(objects, start, mid);
			right = make_shared<bvh_node>(objects, mid, end);
		}

		// �ýڵ��bounds�������ӽڵ�bounds�Ĳ�
		box = Union(left->bounds(), right->bounds());
	}

	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
		// �����node��bounds�����û�н��㣬ֱ�ӷ���false
		if (not box.hit(r, t_min, t_max))
			return false;

		// ����Ƿ������ӽڵ��н���
		bool hit_left = left->hit(r, t_min, t_max, rec);
		// ����Ƿ������ӽڵ��н��㣬��ʱ�޶�t����Ҫ�����ӽڵ�С
		bool hit_right = right->hit(r, t_min, hit_left ? rec.t : t_max, rec);

		return hit_left or hit_right;
	}

	virtual bounds3 bounds() const override {
		return box;
	}
};

int bvh_node::node_num = 0;

// ����bvh�������ظ��ڵ�
shared_ptr<hittable> generate_bvh(hittable_list &list) {
	shared_ptr<hittable> root = make_shared<bvh_node>(list.objects, 0, list.objects.size());
	return root;
}


#endif