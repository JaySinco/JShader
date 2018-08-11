#include <fstream>
#include <iostream>

#include "tgaimage.h"
#include "model.h"

const int width = 800, height = 800;
const TGAColor white = TGAColor(255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0);

void draw_line(int x1, int y1, int x2, int y2, TGAImage &image, TGAColor color)
{
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

int main(int argc, char *argv[])
{
	auto image = TGAImage(width, height, TGAFormat::RGB);
	ObjModel model("african_head.obj");
	for (const auto &face : model.faces)
	{
		int n = face.size();
		for (int i = 0; i < n; ++i)
		{
			auto v1 = face[i];
			auto v2 = face[(i + 1) % n];
			auto v1x = int((v1.v.x + 1) / 2 * width);
			auto v1y = int((v1.v.y + 1) / 2 * height);
			auto v2x = int((v2.v.x + 1) / 2 * width);
			auto v2y = int((v2.v.y + 1) / 2 * height);
			draw_line(v1x, v1y, v2x, v2y, image, white);
		}
	}
	image.to_file("output.tga");
	std::cout << "done!" << std::endl;
	getchar();
}