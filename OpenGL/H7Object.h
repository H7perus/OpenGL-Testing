#pragma once
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include "shader.h"
#include "model.h"

class H7Object
{
public:
	glm::mat4 transformMat = glm::mat4(1.0);
	Model* model;
	H7Object();
	H7Object(const char model_path[]) 
	{
		model = new Model(model_path);
	}
	
	void setTransform(glm::mat4 setMat)
	{
		transformMat = setMat;
	}
	void setTransform(glm::dmat4 setMat)
	{
		transformMat = setMat;
	}
	void Draw(Shader& shader)
	{
		shader.setMat4("model", transformMat);
		model->Draw(shader);
	}
};

