#pragma once
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include "shader.h"

class skyRendering
{
public:
	unsigned int VAO, VBO;
	float skyarray[18] = {  -1.0, -1.0, 0.1,
							 1.0, -1.0, 0.1,
							-1.0,  1.0, 0.1,
							 1.0, -1.0, 0.1,
							 1.0,  1.0, 0.1,
							-1.0,  1.0, 0.1 };

	Shader shader = Shader("skyShader.vert", "skyShader.frag");
	skyRendering()
	{
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyarray), skyarray, GL_STATIC_DRAW);
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	}
	void Draw() 
	{
		shader.use();
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBindVertexArray(VAO);

		glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glEnable(GL_DEPTH_TEST);
	}
	void updateSunDir(glm::vec3 sunDir)
	{
		shader.use();
		shader.setVec3("sunDir", sunDir);
	}
};

