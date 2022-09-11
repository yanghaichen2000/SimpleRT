#pragma once
#ifndef MEDIUM_H
#define MEDIUM_H

#include <memory>
using std::shared_ptr;

class medium {
public:
	// ÕÛÉäÂÊ
	double n = 1;

public:
	medium(double n_init = 1) {
		n = n_init;
	}
};

shared_ptr<medium> default_medium_ptr = make_shared<medium>(1);

#endif