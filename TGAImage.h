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

std::ostream &operator<<(std::ostream &out, TGAFormat& tf) {
	switch (tf) {
		case GRAYSCALE: out << "GRAYSCALE"; break;
		case RGB:       out << "RGB"; break;
		case RGBA:      out << "RGBA"; break;
		default:        out << "UNKNOW"; break;
	}
	return out;
}

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

	TGAColor operator*(float rate) const
	{
		TGAColor temp = *this;
		for (auto &i : temp.raw)
			i = unsigned char(i*rate);
		return temp;
	}

	TGAColor operator+(const TGAColor rhs) const
	{
		assert(bytespp == rhs.bytespp);
		TGAColor temp = *this;
		for (int i = 0; i < int(bytespp); ++i)
			temp.raw[i] += rhs.raw[i];
		return temp;
	}
};


const TGAColor white = TGAColor(255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0);
const TGAColor green = TGAColor(0, 255, 0);
const TGAColor blue = TGAColor(0, 0, 255);
const TGAColor black = TGAColor(0, 0, 0);
const TGAColor yellow = TGAColor(247,238, 214);
const TGAColor gray = TGAColor(144, 144, 144);

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

	TGAImage(int w, int h, TGAFormat bpp) : data(nullptr), width(w), height(h), bytespp(bpp) 
	{
		size_t sz = width * height * bytespp;
		data = new unsigned char[sz];
		memset(data, 0, sz);
	};

	TGAImage(const std::string &filename) :data(nullptr)
	{
		from_file(filename);
	}

	TGAImage(const TGAImage &rhs) : width(rhs.width), height(rhs.height), bytespp(rhs.bytespp)
	{
		size_t sz = width * height * bytespp;
		data = new unsigned char[sz];
		std::memcpy(data, rhs.data, sz);
	}

	TGAImage &operator=(const TGAImage &rhs)
	{
		if (this != &rhs) {
			if (data) delete[] data;
			width = rhs.width;
			height = rhs.height;
			bytespp = rhs.bytespp;
			size_t sz = width * height * bytespp;
			data = new unsigned char[sz];
			std::memcpy(data, rhs.data, sz);
		}
		return *this;
	};

	int get_width() const
	{
		return width;
	}

	int get_height() const
	{
		return height;
	}

	TGAColor get(int x, int y) const 
	{
		TGAColor c(bytespp);
		if (data && x >= 0 && y >= 0 && x < width && y < height)
			std::memcpy(&c.val, data + (x + y * width)*bytespp, bytespp);
		else
			std::cerr << "illege access: " << x << ", " << y << "\n";
		return c;
	}

	void set(int x, int y, TGAColor c) 
	{
		assert(c.bytespp == bytespp);
		if(!(data && x >= 0 && y >= 0 && x < width && y < height)) return;
		std::memcpy(data + (x + y * width)*bytespp, &c.val, bytespp);
	}

	void from_file(const std::string &filename) {
		if (data) delete[] data;
		data = NULL;
		std::ifstream in(filename, std::ios::binary);
		assert(in.is_open());
		TGAHeader header;
		in.read((char *)&header, sizeof(header));
		width = header.Width;
		height = header.Height;
		bytespp = TGAFormat(header.PixelDepth >> 3);
		assert(width > 0 && height > 0 && (bytespp == GRAYSCALE || bytespp == RGB || bytespp == RGBA));
		unsigned long nbytes = bytespp * width * height;
		data = new unsigned char[nbytes];
		if (3 == header.ImageTyp || 2 == header.ImageTyp)
			in.read((char *)data, nbytes);
		else if (10 == header.ImageTyp || 11 == header.ImageTyp)
			load_rle_data(in);
		else
			assert(!(std::cerr << "unknown file format " << (int)header.ImageTyp << "\n"));
		if (header.ImgDesc & 0x20) {
			flip_vertically();
		}
		if (header.ImgDesc & 0x10) {
			flip_horizontally();
		}
		assert(in.good());
		assert(std::cerr << "image loaded from \"" << filename <<  "\"("
			<< width << "x" << height << "/" << bytespp << ")\n");
		in.close();
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

	void clear(TGAColor c)
	{
		for (int i = 0; i < width; ++i)
			for (int j = 0; j < height; ++j)
				set(i, j, c);
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

	void load_rle_data(std::ifstream &in) {
		unsigned long pixelcount = width * height;
		unsigned long currentpixel = 0;
		unsigned long currentbyte = 0;
		TGAColor colorbuffer(bytespp);
		do {
			unsigned char chunkheader = 0;
			chunkheader = in.get();
			if (chunkheader<128) {
				chunkheader++;
				for (int i = 0; i<chunkheader; i++) {
					in.read((char *)colorbuffer.raw, bytespp);
					for (int t = 0; t<bytespp; t++)
						data[currentbyte++] = colorbuffer.raw[t];
					currentpixel++;
					assert(currentpixel <= pixelcount);
				}
			}
			else {
				chunkheader -= 127;
				in.read((char *)colorbuffer.raw, bytespp);
				for (int i = 0; i<chunkheader; i++) {
					for (int t = 0; t<bytespp; t++)
						data[currentbyte++] = colorbuffer.raw[t];
					currentpixel++;
				}
			}
		} while (currentpixel < pixelcount);
		assert(in.good());
	}

	void flip_horizontally() {
		if (!data) return;
		int half = width >> 1;
		for (int i = 0; i<half; i++) {
			for (int j = 0; j<height; j++) {
				TGAColor c1 = get(i, j);
				TGAColor c2 = get(width - 1 - i, j);
				set(i, j, c2);
				set(width - 1 - i, j, c1);
			}
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