#pragma once

#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>

#include "geometry.h"


struct ObjModel {
	struct Vertex { Vec3f v, vt, vn; };
	using Face = std::vector<Vertex>;
	std::vector<Face> faces;

	explicit ObjModel(const std::string &filename)
	{	
		std::ifstream in(filename);
		assert(in.good());
		std::vector<Vec3f> raw_v;
		std::vector<Vec3f> raw_vt;
		std::vector<Vec3f> raw_vn;
 		std::string line;
		while (std::getline(in, line)) 
		{
			std::istringstream iss(line);
			char trash;
			if (line.substr(0, 2) == "v ") 
			{
				iss >> trash;
				Vec3f v;
				for (int i = 0; i<3; i++) 
					iss >> v.raw[i];
				raw_v.push_back(v);
			}
			else if (line.substr(0, 2) == "vn")
			{
				iss >> trash >> trash;
				Vec3f vn;
				for (int i = 0; i < 3; i++)
					iss >> vn.raw[i];
				raw_vn.push_back(vn);
			}
			else if (line.substr(0, 2) == "vt")
			{
				iss >> trash >> trash;
				Vec3f vt;
				for (int i = 0; i<3; i++)
					iss >> vt.raw[i];
				raw_vt.push_back(vt);
			}
			else if (line.substr(0, 2) == "f ") 
			{
				Face f;
				int vi, vti, vni;
				iss >> trash;
				while (iss >> vi >> trash >> vti >> trash >> vni) 
				{
					Vertex vx;
					vx.v = raw_v[vi-1];
					vx.vn = raw_vn[vni-1];
					vx.vt = raw_vt[vti-1];
					f.push_back(vx);
				}
				faces.push_back(f);
			}
		}
		assert(std::cerr << "model loaded from \"" << filename <<  "\" (#v" << raw_v.size()
			<< " #vn" << raw_vn.size()  << " #vt" << raw_vt.size() << " #f" << faces.size() << ")" << std::endl);
	}
};
