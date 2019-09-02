#pragma once


#include "boost/dynamic_bitset.hpp"
#include <vector>

namespace kahypar {

	template<class T>
	class ClearListSet {
	public:
		ClearListSet(size_t n) : contained_keys(), set(n)  {}
	public:

		void insert(const T e) {
			set.set(e);
			contained_keys.push_back(e);
		}

		void remove(const T e) {
			set.reset(e);
		}

		bool contains(const T e) const {
			return set[e];
		}

		const std::vector<T>& keys() const { return contained_keys; }
		std::vector<T>& keys() { return contained_keys; }

		template<typename F> void forKeys(F f) {
			for (T& e : contained_keys)
				if (contains(e))
					f(e);
		}

		void clear() {
			for (T& e : keys())
				remove(e);
			contained_keys.clear();
		}

	private:
		std::vector<T> contained_keys;
		boost::dynamic_bitset<> set;
	};


	template<class K, class V>
	class ClearListMap {
	public:
		ClearListMap(size_t size, V defaultValue) : defaultValue(defaultValue), contained_keys(), map(size, defaultValue) {}

		void insert(const K k, const V v) {
			contained_keys.push_back(k);
			map[k] = v;
		}

		void add(const K k, const V v) {
			if (!contains(k))
				insert(k,v);
			else
				map[k] += v;
		}

		void remove(const K k) {
			map[k] = defaultValue;
		}

		bool contains(const K k) const {
			return map[k] != defaultValue;
		}

		const V& operator[](const K k) const { return map[k]; }

		V& operator[](const K k) { return map[k]; }

		const std::vector<K>& keys() const { return contained_keys; }
		std::vector<K>& keys() { return contained_keys; }

		template<typename F> void forKeyValuePairs(F f) {
			for (K& k :keys())
				if (contains(k))
					f(k,map[k]);
		}

		void clear() {
			for (K& k : keys())
				remove(k);
			contained_keys.clear();
			assert( std::all_of( map.begin(), map.end(), [&](const V& v) { return v == defaultValue; } ) );
		}

		void merge(ClearListMap<K,V>& other) {
			for (K& k : other.keys())
				add(k, other[k]);
			other.clear();
		}

	private:
		V defaultValue;
		std::vector<K> contained_keys;
		std::vector<V> map;
	};

}