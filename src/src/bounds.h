#ifndef _BOUNDS_H_
#define _BOUNDS_H_
#include "vec3.h"
#include "algorithm"


class bounds3
{
public:
	vec3 pMin, pMax;

	// 默认构造函数，生成最反的bounds
	bounds3()
	{
		double min_num = std::numeric_limits<double>::lowest();
		double max_num = std::numeric_limits<double>::max();
		pMax = vec3(min_num, min_num, min_num);
		pMin = vec3(max_num, max_num, max_num);
	}

	// 使用一个坐标构造，生成大小为0的bounds
	bounds3(const vec3 &p) : pMin(p), pMax(p) {}

	// 使用两个坐标构造
	bounds3(const vec3 &p1, const vec3 &p2) :
		pMin(std::min(p1[0], p2[0]), std::min(p1[1], p2[1]), std::min(p1[2], p2[2])),
		pMax(std::max(p1[0], p2[0]), std::max(p1[1], p2[1]), std::max(p1[2], p2[2])) {}

	// 获取pMin或者pMax
	const vec3 &operator[](int i) const
	{
		if (i == 0) return pMin;
		else return pMax;
	}
	vec3 &operator[](int i)
	{
		if (i == 0) return pMin;
		else return pMax;
	}

	// 获取顶点坐标
	// corner \in [0, 7]
	vec3 Corner(int corner) const
	{
		return vec3((*this)[(corner & 1)][0],
						(*this)[(corner & 2) ? 1 : 0][1],
						(*this)[(corner & 4) ? 1 : 0][2]);
	}

	// 获取从pMin到pMax的向量
	vec3 Diagnal() const
	{
		return pMax - pMin;
	}

	// 求表面积
	double SurfaceArea() const
	{
		vec3 d = Diagnal();
		return 2 * (d[0] * d[1] + d[0] * d[2] + d[2] * d[1]);
	}

	// 求体积
	double Volume() const
	{
		vec3 d = Diagnal();
		return d[0] * d[1] * d[2];
	}

	// 获取最长边
	int MaximumExtent() const
	{
		vec3 d = Diagnal();
		if (d[0] > d[1] and d[0] > d[2])
			return 0;
		else if (d[1] > d[2])
			return 1;
		else
			return 2;
	}

	// 加权混合
	vec3 Lerp(const vec3 &t) const 
	{
		return vec3(
			pMin[0] * (1 - t[0]) + pMax[0] * t[0],
			pMin[1] * (1 - t[1]) + pMax[1] * t[1],
			pMin[2] * (1 - t[2]) + pMax[2] * t[2]
		);
	}

	// 获取点在bounds的相对坐标，相当于Lerp的逆操作
	vec3 Offset(const vec3 &p) const
	{
		vec3 t = p - pMin;
		if (pMax[0] > pMin[0])
			t[0] = t[0] / (pMax[0] - pMin[0]);
		if (pMax[1] > pMin[1])
			t[1] = t[1] / (pMax[1] - pMin[1]);
		if (pMax[0] > pMin[0])
			t[2] = t[2] / (pMax[2] - pMin[2]);
		return t;
	}

	/*
	// 求解bounds的外接球
	// 数据记录在传入的指针对应的位置
	void BoundingSphere(vec3 *center, double *radius) const
	{
		*center = (pMin + pMax) / 2;
		*radius = Inside(*center, *this) ? (*center - pMax).length() : 0;
	}
	*/

	// bounds与光线求交
	bool hit(const ray& r, double tmin, double tmax) const
	{
		for (int i = 0; i < 3; i++) // 依次对三个坐标方向进行计算
		{
			double invD = 1.0 / r.direction()[i];
			// 获取和两个平行平面的交点的t值
			double t0 = (pMin[i] - r.origin()[i]) * invD;
			double t1 = (pMax[i] - r.origin()[i]) * invD;
			// 确保 t0 < t1
			if (invD < 0.0) std::swap(t0, t1);

			// 更新tmax和tmin（与t0和t1中间的范围求交）
			tmin = (tmin > t0) ? tmin : t0;
			tmax = (tmax < t1) ? tmax : t1;

			// 如果求交后发现tmax和tmin大小关系相反，则表示没有交点
			if (tmax <= tmin) return false;
		}

		return true;
	}
};


// 将一个点并入bounds
bounds3 Union(const bounds3 &b, const vec3 &p)
{
	return bounds3(
		vec3(std::min(b.pMin[0], p[0]), std::min(b.pMin[1], p[1]), std::min(b.pMin[2], p[2])),
		vec3(std::max(b.pMax[0], p[0]), std::max(b.pMax[1], p[1]), std::max(b.pMax[2], p[2]))
	);
}

// 合并两个bounds
bounds3 Union(const bounds3 &b1, const bounds3 &b2)
{
	return bounds3(
		vec3(std::min(b1.pMin[0], b2.pMin[0]), std::min(b1.pMin[1], b2.pMin[1]), std::min(b1.pMin[2], b2.pMin[2])),
		vec3(std::max(b1.pMax[0], b2.pMax[0]), std::max(b1.pMax[1], b2.pMax[1]), std::max(b1.pMax[2], b2.pMax[2]))
	);
}

// 两个bounds求交
bounds3 Intersect(const bounds3 &b1, const bounds3 &b2)
{
	return bounds3(
		vec3(std::max(b1.pMin[0], b2.pMin[0]), std::max(b1.pMin[1], b2.pMin[1]), std::max(b1.pMin[2], b2.pMin[2])),
		vec3(std::min(b1.pMax[0], b2.pMax[0]), std::min(b1.pMax[1], b2.pMax[1]), std::min(b1.pMax[2], b2.pMax[2]))
	);
}

// 判断两个bounds是否有交集
bool Overlaps(const bounds3 &b1, const bounds3 &b2)
{
	bool tmp_1 = (b1.pMax[0] >= b2.pMin[0]) and (b1.pMin[0] <= b2.pMax[0]);
	bool tmp_2 = (b1.pMax[1] >= b2.pMin[1]) and (b1.pMin[1] <= b2.pMax[1]);
	bool tmp_3 = (b1.pMax[2] >= b2.pMin[2]) and (b1.pMin[2] <= b2.pMax[2]);

	return (tmp_1 && tmp_2 && tmp_3);
}

// 判断点是否在bounds内
bool Inside(const vec3 &p, const bounds3 &b)
{
	bool tmp_1 = (p[0] >= b.pMin[0]) and (p[0] <= b.pMax[0]);
	bool tmp_2 = (p[1] >= b.pMin[1]) and (p[1] <= b.pMax[1]);
	bool tmp_3 = (p[2] >= b.pMin[2]) and (p[2] <= b.pMax[2]);

	return (tmp_1 && tmp_2 && tmp_3);
}

// 判断点是否在bounds内（不含边界）
// 对于float该函数与Inside等效
bool InsideExclusive(const vec3 &p, const bounds3 &b)
{
	bool tmp_1 = (p[0] > b.pMin[0]) and (p[0] < b.pMax[0]);
	bool tmp_2 = (p[1] > b.pMin[1]) and (p[1] < b.pMax[1]);
	bool tmp_3 = (p[2] > b.pMin[2]) and (p[2] < b.pMax[2]);

	return (tmp_1 && tmp_2 && tmp_3);
}

// 将bounds向外扩展delta
inline bounds3 Expand(const bounds3 &b, double delta)
{
	return bounds3(b.pMin - vec3(delta, delta, delta), b.pMax + vec3(delta, delta, delta));
}








#endif