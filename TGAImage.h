#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#pragma pack(push, 1)
struct TGAHeader
{
	unsigned char  IdLen;
	unsigned char  ColorMapTyp;
	unsigned char  ImageTyp;
	unsigned short ColorMapFstIdx;
	unsigned short ColorMapLen;
	unsigned char  ColorMapSize;
	unsigned short XOrigin;
	unsigned short YOrigin;
	unsigned short Width;
	unsigned short Height;
	unsigned char  PixelDepth;
	unsigned char  ImgDesc;
};
#pragma pack(pop)

enum TGAFormat {
	GRAYSCALE = 1, RGB = 3, RGBA = 4
};

struct TGAColor
{
	union {
		struct {
			unsigned char b, g, r, a;
		};
		unsigned char raw[4];
		unsigned int val;
	};

	TGAFormat bytespp;

	TGAColor() = delete;

	explicit TGAColor(TGAFormat f) :
		val(0), bytespp(f) {}

	explicit TGAColor(unsigned char gray) :
		val(gray), bytespp(TGAFormat::GRAYSCALE) {}

	TGAColor(unsigned char R, unsigned char G, unsigned char B) :
		r(R), g(G), b(B), a(0), bytespp(TGAFormat::RGB) {}

	TGAColor(unsigned char R, unsigned char G, unsigned char B, unsigned char A) :
		r(R), g(G), b(B), a(A), bytespp(TGAFormat::RGBA) {}

	TGAColor(int v, TGAFormat bpp) : val(v), bytespp(bpp) {}
};

inline std::ostream &operator<<(std::ostream &out, const TGAColor &c)
{
	switch (c.bytespp)
	{
	case TGAFormat::GRAYSCALE:
		out << "TGAColor(GRAY=" << int(c.b) << ")";
		break;
	case TGAFormat::RGB:
		out << "TGAColor(R=" << int(c.r) << ", G=" << int(c.g) << ", B=" << int(c.b) << ")";
		break;
	case TGAFormat::RGBA:
		out << "TGAColor(R=" << int(c.r) << ", G=" << int(c.g) << ", B=" << int(c.b) << ", A=" << int(c.a) << ")";
		break;
	default:
		out << "TGAColor(UNKNOW TGAFormat)";
		break;
	}
	return out;
}

class TGAImage
{
public:

	TGAImage() = delete;

	TGAImage(int w, int h, TGAFormat bpp) : data(nullptr), width(w), height(h), bytespp(bpp) {
		size_t sz = width * height * bytespp;
		data = new unsigned char[sz];
		memset(data, 0, sz);
	};

	TGAImage(const TGAImage &rhs) : width(rhs.width), height(rhs.height), bytespp(rhs.bytespp)
	{
		size_t sz = width * height * bytespp;
		data = new unsigned char[sz];
		std::memcpy(data, rhs.data, sz);
	}

	TGAImage &operator=(const TGAImage &) = delete;

	TGAColor get(int x, int y) {
		assert(data && x >= 0 && y >= 0 && x < width && y < height);
		TGAColor c(bytespp);
		std::memcpy(&c.val, data + (x + y * width)*bytespp, bytespp);
		return c;
	}

	void set(int x, int y, TGAColor c) {
		if(!(data && x >= 0 && y >= 0 && x < width && y < height && c.bytespp == bytespp)) return;
		std::memcpy(data + (x + y * width)*bytespp, &c.val, bytespp);
	}

	void to_file(const std::string &filename, bool rle=true)
	{
		unsigned char developer_area_ref[4] = { 0, 0, 0, 0 };
		unsigned char extension_area_ref[4] = { 0, 0, 0, 0 };
		unsigned char footer[18] = { 'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0' };
		std::ofstream out;
		out.open(filename, std::ios::binary);
		assert(out.is_open());
		TGAHeader header;
		memset(&header, 0, sizeof(header));
		header.PixelDepth = bytespp << 3;
		header.Width = width;
		header.Height = height;
		header.ImageTyp = (bytespp == TGAFormat::GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
		header.ImgDesc = 0x20;
		out.write((char*)&header, sizeof(header));
		flip_vertically();
		if (!rle) {
			out.write((char *)data, width*height*bytespp);
		}
		else {
			unload_rle_data(out);
		}
		out.write((char *)developer_area_ref, sizeof(developer_area_ref));
		out.write((char *)extension_area_ref, sizeof(extension_area_ref));
		out.write((char *)footer, sizeof(footer));
		assert(out.good());
		out.close();
	}

	~TGAImage()
	{
		delete[] data;
	}

private:
	unsigned char *data;
	int width;
	int height;
	TGAFormat bytespp;

	void unload_rle_data(std::ofstream &out)
	{
		const unsigned char max_chunk_length = 128;
		unsigned long npixels = width * height;
		unsigned long curpix = 0;
		while (curpix<npixels) {
			unsigned long chunkstart = curpix * bytespp;
			unsigned long curbyte = curpix * bytespp;
			unsigned char run_length = 1;
			bool raw = true;
			while (curpix + run_length<npixels && run_length<max_chunk_length) {
				bool succ_eq = true;
				for (int t = 0; succ_eq && t<bytespp; t++) {
					succ_eq = (data[curbyte + t] == data[curbyte + t + bytespp]);
				}
				curbyte += bytespp;
				if (1 == run_length) {
					raw = !succ_eq;
				}
				if (raw && succ_eq) {
					run_length--;
					break;
				}
				if (!raw && !succ_eq) {
					break;
				}
				run_length++;
			}
			curpix += run_length;
			out.put(raw ? run_length - 1 : run_length + 127);
			out.write((char *)(data + chunkstart), (raw ? run_length * bytespp : bytespp));
			assert(out.good());
		}
	}

	void flip_vertically()
	{
		if (!data) return;
		unsigned long bytes_per_line = width * bytespp;
		unsigned char *line = new unsigned char[bytes_per_line];
		int half = height >> 1;
		for (int j = 0; j<half; j++) {
			unsigned long l1 = j * bytes_per_line;
			unsigned long l2 = (height - 1 - j)*bytes_per_line;
			std::memmove((void *)line, (void *)(data + l1), bytes_per_line);
			std::memmove((void *)(data + l1), (void *)(data + l2), bytes_per_line);
			std::memmove((void *)(data + l2), (void *)line, bytes_per_line);
		}
		delete[] line;
	}
};