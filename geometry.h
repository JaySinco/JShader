#pragma once

#include <iostream>
#include <cmath>

template<typename T>
struct Vec2 {
	union {
		struct { T u, v; };
		struct { T x, y; };
		T raw[2];
	};
	Vec2(): u(0), v(0) {}
	Vec2(T _u, T _v) : u(_u), v(_v) {}
	Vec2<T> operator+(const Vec2<T> &r) const { return Vec2<T>(u + r.u, v + r.v); }
	Vec2<T> operator-(const Vec2<T> &r) const { return Vec2<T>(u - r.u, v - r.v); }
	Vec2<T> operator*(float f)          const { return Vec2<T>(u*f, v*f); }
};

template<typename T> std::ostream &operator<<(std::ostream &out, Vec2<T>& v2) {
	out << "Vec2(" << v2.x << ", " << v2.y << ")";
	return out;
}

template<typename T>
struct Vec3 {
	union {
		struct { T u, v, w; };
		struct { T x, y, z; };
		T raw[3];
	};
	Vec3() : x(0), y(0), z(0) {}
	Vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
	Vec3<T> operator^(const Vec3<T> &r) const { return Vec3<T>(y*r.z - z * r.y, z*r.x - x * r.z, x*r.y - y * r.x); }
	Vec3<T> operator+(const Vec3<T> &r) const { return Vec3<T>(x + r.x, y + r.y, z + r.z); }
	Vec3<T> operator-(const Vec3<T> &r) const { return Vec3<T>(x - r.x, y - r.y, z - r.z); }
	Vec3<T> operator*(float f)          const { return Vec3<T>(x*f, y*f, z*f); }
	T       operator*(const Vec3<T> &r) const { return x * r.x + y * r.y + z * r.z; }
	float norm() const { return std::sqrt(x*x + y * y + z * z); }
	Vec3<T> &normalize(T n = 1) { *this = (*this)*(n / norm()); return *this; }
};

template<typename T> std::ostream &operator<<(std::ostream &out, Vec3<T>& v3) {
	out << "Vec3(" << v3.x << ", " << v3.y << ", " << v3.z << ")";
	return out;
}

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;
using Vec3f = Vec3<float>;
using Vec3i = Vec3<int>;
