#pragma once

#include <atomic>

namespace kahypar {
namespace parallel {

template<typename T>
void fetch_add(std::atomic<T>& x, T y) {
	T cur_x = x.load();
	while(!x.compare_exchange_weak(cur_x, cur_x + y, std::memory_order_relaxed));
}

template<typename T>
void fetch_sub(std::atomic<T>& x, T y) {
	T cur_x = x.load();
	while(!x.compare_exchange_weak(cur_x, cur_x - y, std::memory_order_relaxed));
}

template<class T>
class AtomicWrapper : public std::atomic<T> {
public:
	void operator+=(T other) {
		fetch_add(*this, other);
	}

	void operator-=(T other) {
		fetch_sub(*this, other);
	}
};

}
}