#pragma once
#include <vector>
#include "model.h"
class H7Asset
{
public:
	std::vector<Model*> LOD_levels; // 0 = highest quality
	std::vector<std::string> LOD_dirs;
	std::vector<int> LOD_load_states;
	H7Asset(std::vector<std::string> LOD_dirs, bool load)
	{
		this->LOD_dirs = LOD_dirs;
		if (load)
		{
			for (string& dir : LOD_dirs)
			{
				LOD_levels.push_back(new Model(dir));
			}
		}
	}
	void Draw(Shader& shader, int LOD_index)
	{
		if (LOD_load_states[LOD_index] == 2)
		{
			LOD_levels[LOD_index]->loadTextures();
			for (Mesh& mesh : LOD_levels[LOD_index]->meshes)
			{
				mesh.setupMesh(LOD_levels[LOD_index]->textures_loaded);
			}


			LOD_load_states[LOD_index] = 3;
		}
		if (LOD_load_states[LOD_index] == 3)
		{
			LOD_levels[LOD_index]->Draw(shader);
		}
		else if (LOD_load_states[LOD_index] == 0)
		{
			std::cout << "else before" << std::endl;
			std::thread load_thread(&H7Asset::Load, this);
			LOD_load_states[LOD_index] = 1;
			load_thread.detach();

			std::cout << "else after" << std::endl;
		}
	}
	void Load(int LOD_id)
	{
		if (LOD_id < LOD_levels.size())
		{
			LOD_levels[LOD_id] = new Model(LOD_dirs[LOD_id]);
		}
		else
		{
			std::cout << "LOD level doesn't exist" << std::endl;
		}
	}
private:
	
};

