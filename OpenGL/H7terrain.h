#pragma once
#include <string>
#include "stb_image.h"
#include <iostream>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

template < int map_size >

class H7terrain
{
public:
	double *height_data = new double[map_size * map_size];//this will also be used for btHeightfieldTerrainShape
	//double height_data[map_size * map_size];
	int width = map_size, length = map_size;
	int channels = 1;
	int VBO_size = 0;
	GLuint tVBO, tVAO;

	H7terrain(const char terrain_heightmap[], float min_height, float max_height)
	{
		
		uint16_t* height_image_data = stbi_load_16(terrain_heightmap, &width, &length, &channels, 1);
		glGenBuffers(1, &tVBO);
		glBindBuffer(GL_ARRAY_BUFFER, tVBO);
		glBufferData(GL_ARRAY_BUFFER, pow(map_size - 1, 2) * sizeof(GLfloat) * 6 * 8, 0, GL_STATIC_DRAW);

		glGenVertexArrays(1, &tVAO);
		glBindVertexArray(tVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));

		GLuint index[3];
		float height_range = max_height - min_height;
		for (int i = 0; i < pow(map_size, 2); i++)
		{
			height_data[i] = float(*(height_image_data + i)) / 65535 * height_range;
		}
		GLfloat data[8];
		GLfloat face_data[24];
		
		for (int i = 0; i < pow(map_size, 2); i++)
		{
			
			if (i < pow(map_size, 2) - map_size && (i + 1) % map_size != 0)
			{
				get_vertdata(&face_data[0], i);
				get_vertdata(&face_data[8], i + map_size + 1);
				get_vertdata(&face_data[16], i + 1);
				get_facenormal(face_data);
				glBufferSubData(GL_ARRAY_BUFFER, VBO_size, sizeof(face_data), face_data);
				get_vertdata(&face_data[16], i + map_size);
				get_facenormal(face_data);
				glBufferSubData(GL_ARRAY_BUFFER, VBO_size + sizeof(face_data), sizeof(face_data), face_data);
				VBO_size += 2 * sizeof(face_data);
			}
		}
	}
private:
	void get_vertdata(GLfloat *coordinates, int ind)
	{
		coordinates[0] = float(ind % map_size) - map_size / 2 + 0.5;
		coordinates[1] = height_data[ind];
		coordinates[2] = -map_size / 2 + 0.5f + float(floor(ind / map_size));
		coordinates[6] = float(ind % map_size) / (map_size - 1);
		coordinates[7] = float(floor(ind / map_size)) / map_size;
	}
	void get_facenormal(GLfloat* face_data)
	{
		glm::vec3 AB(face_data[8] - face_data[0], face_data[9] - face_data[1], face_data[10] - face_data[2]);
		glm::vec3 AC(face_data[16] - face_data[0], face_data[17] - face_data[1], face_data[18] - face_data[2]);

		glm::vec3 normal = cross(AB, AC);
		if (normal.y < 0)
		{
			normal *= -1;
		}
		//normal *= -1;
		for (int j = 0; j < 3; j++)
		{
			face_data[j * 8 + 3] = normal.x;
			face_data[j * 8 + 4] = normal.y;
			face_data[j * 8 + 5] = normal.z;
		}
	}
};
