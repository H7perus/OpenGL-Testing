#pragma once
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include "shader.h"
#include "model.h"
#include "H7Asset.h"
#include <thread>

class H7Object
{
public:
	glm::mat4 transformMat = glm::mat4(1.0);
	H7Asset *asset;
	H7Object();
	H7Object(std::vector<std::string> LOD_dirs, bool load)
	{
		asset = new H7Asset(LOD_dirs, load);
	}
	
	void setTransform(glm::mat4 setMat)
	{
		transformMat = setMat;
	}
	void setTransform(glm::dmat4 setMat)
	{
		transformMat = setMat;
	}
	void Draw(Shader& shader, int LOD_index)
	{
		if (load_state == 2)
		{
			model->loadTextures();
			for (Mesh& mesh : model->meshes)
			{
				mesh.setupMesh(model->textures_loaded);
				//std::cout << "size of textures: " << mesh.textures.size() << std::endl;
				//if(mesh.textures.size() > 0)
					//std::cout << "texture 0: " << mesh.textures[0].path << std::endl;
			}
			

			load_state = 3;
		}
		if (load_state == 3)
		{
			shader.setMat4("model", transformMat);
			model->Draw(shader);
		}
		else if(load_state == 0)
		{
			std::cout << "else before" << std::endl;
			std::thread load_thread(&H7Object::Load, this);
			load_state = 1;
			load_thread.detach();
			
			std::cout << "else after" << std::endl;
		}
	}
	void Load()
	{
		model = new Model(model_path);
		load_state = 2;
	}
	void Unload()
	{
		if (load_state != (0 || 1))
		{
			delete model;
			load_state = 0;
		}
	}
};

