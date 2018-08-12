#include <fstream>
#include <iostream>
#include <limits>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const int width = 800, height = 800;
const TGAColor white = TGAColor(255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0);
const TGAColor green = TGAColor(0, 255, 0);
const TGAColor blue = TGAColor(0, 0, 255);

void draw_line(Vec2i v1, Vec2i v2, TGAImage &image, TGAColor color)
{
	int x1 = v1.x, y1 = v1.y;
	int x2 = v2.x, y2 = v2.y;
	bool steep = false;
	if (std::abs(x1 - x2) < std::abs(y1 - y2)) {
		std::swap(x1, y1);
		std::swap(x2, y2);
		steep = true;
	}
	if (x1 > x2) { 
		std::swap(x1, x2);
		std::swap(y1, y2);
	}
	for (int x = x1; x != x2; ++x)
	{
		float t = (x - x1) / (float)(x2 - x1);
		int y = int(y1 * (1. - t) + y2 * t);
		if (steep) {
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
		}
	}
}

void draw_triangle(Vec3f v1, Vec3f v2, Vec3f v3, float *bufferz, TGAImage &image, TGAColor color)
{
	if (v1.y == v2.y && v1.y == v3.y) return;
	if (v1.y > v2.y) std::swap(v1, v2);
	if (v1.y > v3.y) std::swap(v1, v3);
	if (v2.y > v3.y) std::swap(v2, v3);
	auto total_dy = int(v3.y - v1.y);
	for (int i = 0; i < total_dy; ++i)
	{
		bool part2 = (i > v2.y - v1.y) || (v2.y == v1.y);
		auto segment_dy = int(part2 ? (v3.y - v2.y) : (v2.y - v1.y));
		float alpha = i / float(total_dy);
		float beta = (i - (part2 ? v2.y - v1.y : 0)) / float(segment_dy);
		Vec3f A = v1 + (v3 - v1)*alpha;
		Vec3f B = part2 ? (v2 + (v3 - v2)*beta) : (v1 + (v2 - v1)*beta);
		if (A.x > B.x) std::swap(A, B);
		for (auto j = int(A.x); j <= int(B.x); ++j) {
			float z = (j - A.x) * (B.z - A.z) / (B.x - A.x) + A.z;
			int idx = int(j + (v1.y + i) * width);
			if (z >= bufferz[idx]) 
			{
				bufferz[idx] = z;
				image.set(j, int(v1.y) + i, color);
			}
		}
	}
}


Vec3f world2screen(Vec3f v) {
	auto x = int((v.x + 1.) * width / 2 + .5);
	auto y = int((v.y + 1.) * height / 2. + .5);
	return Vec3f(float(x), float(y), v.z);
}

int main(int argc, char *argv[])
{
	auto image = TGAImage(width, height, TGAFormat::GRAYSCALE);
	ObjModel model("african_head.obj");
	Vec3f light_dir(0, 0, -1);
	float *bufferz = new float[width*height];
	std::fill(bufferz, bufferz+width*height, -1 * std::numeric_limits<float>::max());

	for (const auto &face : model.faces)
	{
		Vec3f pv[3];
		for (int i = 0; i < 3; ++i)
			pv[i] = world2screen(face[i].v);
		Vec3f n = (face[2].v - face[0].v) ^ (face[1].v - face[0].v);
		n.normalize();
		float intensity = n * light_dir;
		if (intensity >= 0)
		{
			auto gray = unsigned char(intensity * 255);
			draw_triangle(pv[0], pv[1], pv[2], bufferz, image, TGAColor(gray));
		}
	}

	image.to_file("output.tga");
	delete[] bufferz;
	std::cout << "done!" << std::endl;
	getchar();
}