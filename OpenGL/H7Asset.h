#pragma once
#include <vector>
#include <thread>
#include "model.h"


struct LOD_level {
	Model model = Model("");
	Shader* shader;
	std::string directory;
	float min_distance;
	int load_state;
};

class H7Asset
{
public:
	string name;
	std::vector<shared_ptr<LOD_level>> LOD_levels;

	H7Asset(std::string name, std::vector<std::string> LOD_dirs, bool load)
	{
		this->name = name;
		for (int i = 0; i < LOD_dirs.size(); i++)
		{
			LOD_levels.push_back(std::move(make_unique<LOD_level>()));
			LOD_levels[i]->directory = LOD_dirs[i];
			LOD_levels[i]->load_state = 1;

			if (load)
				Load(i);
		}
		

		for (int i = 0; i < LOD_levels.size(); i++)
		{
			Load(i);
		}
	}
	H7Asset(Shader &shader, std::string name, std::vector<std::string> LOD_dirs, bool load)
	{
		this->name = name;
		for (int i = 0; i < LOD_dirs.size(); i++)
		{
			LOD_levels.push_back(std::move(make_unique<LOD_level>()));
			LOD_levels[i]->directory = LOD_dirs[i];
			LOD_levels[i]->load_state = 1;
			LOD_levels[i]->shader = &shader;
			if (load)
				Load(i);
		}
		for (int i = 0; i < LOD_levels.size(); i++)
		{
			Load(i);
		}
	}
	void Draw(int LOD_index, glm::mat4 transformMat)
	{
		shared_ptr<LOD_level> LOD_level = LOD_levels[LOD_index];
		if (LOD_level->load_state == 2)
		{
			LOD_level->model.loadTextures();
			for (Mesh& mesh : LOD_level->model.meshes)
			{
				mesh.setupMesh(LOD_level->model.textures_loaded);
			}
			LOD_levels[LOD_index]->load_state = 3;
		}
		if (LOD_levels[LOD_index]->load_state == 3)
		{
			LOD_level->shader->setMat4("model", transformMat);
			LOD_level->model.Draw(*LOD_level->shader);
		}
		else if (LOD_levels[LOD_index]->load_state == 0)
		{
			std::cout << "else before" << std::endl;
			std::thread load_thread(&H7Asset::Load, this, LOD_index);
			LOD_levels[LOD_index]->load_state = 1;
			load_thread.detach();
			std::cout << "else after" << std::endl;
		}
	}
	void Draw(Shader &shader, int LOD_index)
	{
		shared_ptr<LOD_level> LOD_level = LOD_levels[LOD_index];
		if (this->check_if_loaded_else_load(LOD_index))
		{
			LOD_level->model.Draw(shader);
		}
	}
	bool check_if_loaded_else_load(int LOD_index)//goofy aaahhh function name
	{
		shared_ptr<LOD_level> LOD_level = LOD_levels[LOD_index];
		if (LOD_level->load_state == 2)
		{
			LOD_level->model.loadTextures();
			for (Mesh& mesh : LOD_level->model.meshes)
			{
				mesh.setupMesh(LOD_level->model.textures_loaded);
			}
			LOD_levels[LOD_index]->load_state = 3;
		}
		if (LOD_levels[LOD_index]->load_state == 3)
		{
			return true;
		}
		else if (LOD_levels[LOD_index]->load_state == 0)
		{
			std::thread load_thread(&H7Asset::Load, this, LOD_index);
			LOD_levels[LOD_index]->load_state = 1;
			load_thread.detach();
			return false;
		}
	}
	void Load(int LOD_index)
	{
		if (LOD_index < LOD_levels.size())
		{
			LOD_levels[LOD_index]->model = Model(LOD_levels[LOD_index]->directory);
		}
		else
		{
			std::cout << "LOD level doesn't exist" << std::endl;
		}
		LOD_levels[LOD_index]->load_state = 2;
	}
	void Unload(int LOD_index) //don't use this rn, no need with the 3 assets I am using.
	{
		//LOD_levels[LOD_index].reset(); //unloading right now may be regarded as a memory leak. Not gonna fix it now, just don't touch it!
		//LOD_levels[LOD_index]->load_state = 0;
	}
private:
	
};

