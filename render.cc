#include <fstream>
#include <iostream>
#include <limits>
#include <algorithm>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"


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

Vec3f barycentric(Vec2i A, Vec2i B, Vec2i C, Vec2i P)
{
	auto z = Vec3f(float(B.x - A.x), float(C.x - A.x), float(A.x - P.x)) ^ Vec3f(float(B.y - A.y), float(C.y - A.y), float(A.y - P.y));
	if (std::abs(z.w) < 1) 
		return Vec3f(-1, 1, 1);
	return Vec3f(1.0f - (z.u + z.v) / z.w, z.u / z.w, z.v / z.w);
}

void draw_triangle(const Vertex vx[3], float *bufferz, TGAImage &image, TGAImage &texture, const Vec3f light_dir)
{
	int iw = image.get_width(), ih = image.get_height();
	int tw = texture.get_width(), th = texture.get_height();
	Vec2i A(int((vx[0].v.x + 1)*iw / 2), int((vx[0].v.y + 1)*ih / 2));
	Vec2i B(int((vx[1].v.x + 1)*iw / 2), int((vx[1].v.y + 1)*ih / 2));
	Vec2i C(int((vx[2].v.x + 1)*iw / 2), int((vx[2].v.y + 1)*ih / 2));
	int min_x = std::min({ A.x, B.x, C.x });
	int min_y = std::min({ A.y, B.y, C.y });
	int max_x = std::min(std::max({ A.x, B.x, C.x }), iw-1);
	int max_y = std::min(std::max({ A.y, B.y, C.y }), ih-1);
	Vec2i P;
	for (P.x = min_x; P.x <= max_x; ++P.x) 
	{
		for (P.y = min_y; P.y <= max_y; ++P.y) 
		{
			auto bc = barycentric(A, B, C, P);
			if (bc.x < 0 || bc.y < 0 || bc.z < 0) 
				continue;
			auto z_idx = P.x + P.y * iw;
			auto z_itp = bc * Vec3f(vx[0].v.z, vx[1].v.z, vx[2].v.z);
			if (z_itp <= bufferz[z_idx])
				continue;
			bufferz[z_idx] = z_itp;
			auto vn_itp = vx[0].vn * bc.x + vx[1].vn * bc.y + vx[2].vn * bc.z;
			auto vt_itp = vx[0].vt * bc.x + vx[1].vt * bc.y + vx[2].vt * bc.z;
			auto text_color = texture.get(int(tw*vt_itp.x), int(th*vt_itp.y));
			float tensity = vn_itp * light_dir * -1;
			if (tensity > 0)
				image.set(P.x, P.y, text_color * tensity);
		}
	}
}

void draw_object(const std::string &obj_file, const std::string &texture_file, float *bufferz, const Vec3f light_dir, TGAImage &image)
{
	ObjModel model(obj_file);
	TGAImage texture(texture_file);

	for (const auto &face : model.faces) {
		Vertex vx[3];
		for (int i = 0; i < 3; ++i)
			vx[i] = face[i];
		draw_triangle(vx, bufferz, image, texture, light_dir);
	}	
}

int main(int argc, char *argv[])
{
	const int width = 1000, height = 1000;
	TGAFormat format = TGAFormat::RGB;
	TGAImage image(width, height, format);
	float *bufferz = new float[width * height];
	std::fill(bufferz, bufferz + width * height, -1 * std::numeric_limits<float>::max());

	image.clear(black);
	//draw_object("african_head.obj", "african_head_diffuse.tga", bufferz, Vec3f(0, 0, -1.f), image);
	draw_object("diablo3_pose.obj", "diablo3_pose_diffuse.tga", bufferz, Vec3f(0, 0, -1.f), image);
	
	delete[] bufferz;
	std::string out_file("output.tga");
	image.to_file(out_file);
	assert(std::cerr << "done! rendered into \"" << out_file << "\"(" << width << "x" << height
		<< "/" << format << ")" << std::endl);
	getchar();
}