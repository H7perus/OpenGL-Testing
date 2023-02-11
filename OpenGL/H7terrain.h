#pragma once
#include <string>
#include "stb_image.h"
#include <iostream>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include "shader.h"
#include <chrono>
#include <vector>


//I need to implement configs for the maps as a replacement for hardcoding, like a JSON for example. I will continue to hardcode for a bit as I learn more.
class H7terrain
{
public:
	double *height_data;//this will also be used for btHeightfieldTerrainShape
	int map_size;
	int width, length;
	int channels = 1;
	int EBO_size = 0;
	GLuint tVBO, tVAO, tEBO;
	Shader* tShader;
	float u_per_px;

	glm::dvec3 *cameraPosPtr;
	unsigned int textureCount = 0;
	unsigned int decalCount = 0;
	std::vector<unsigned int> textures;
	unsigned int ground_texture, ground_texture2, water_texture, splat_map, airfield_decal; //this will be changed in the future, just having one texture is a pretty bad approach
	uint16_t* height_image_data;
	unsigned char *color_data2, *color_data3;
	H7terrain(const int map_size_in, const char terrain_heightmap[], float min_height, float max_height, const float u_per_px_constructor, Shader *in_Shader)
	{
		tShader = in_Shader;
		map_size = map_size_in;
		width = map_size, length = map_size;
		height_data = new double[map_size * map_size];
		u_per_px = u_per_px_constructor;
		height_image_data = stbi_load_16(terrain_heightmap, &width, &length, &channels, 1);
		glGenBuffers(1, &tVBO);
		glBindBuffer(GL_ARRAY_BUFFER, tVBO);
		glBufferData(GL_ARRAY_BUFFER, pow(map_size - 1, 2) * sizeof(GLfloat) * 6 * 8, NULL, GL_STATIC_DRAW);

		glGenBuffers(1, &tEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, pow(map_size - 1, 2) * 6 * sizeof(GLuint), NULL, GL_STATIC_DRAW);

		glGenVertexArrays(1, &tVAO);
		glBindVertexArray(tVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));


		GLfloat data[8];
		GLuint index[3];
		float height_range = max_height - min_height;
		auto time_start = std::chrono::steady_clock::now();
		auto time_heightstart = std::chrono::steady_clock::now();
		for (int i = 0; i < pow(map_size, 2); i++)
		{
			height_data[i] = float(*(height_image_data + i)) / 65535 * height_range;

			if (i < pow(map_size, 2) - map_size && (i + 1) % map_size != 0)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBO);
				index[0] = i;
				index[1] = i + map_size + 1;
				index[2] = i + 1;
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, EBO_size, sizeof(index), index);
				index[2] = i + map_size;
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, EBO_size + 3 * sizeof(GLuint), sizeof(index), index);
				EBO_size += 2 * sizeof(index);
			}
			//glGetBufferSubData(GL_ARRAY_BUFFER, index[1] * sizeof(data), sizeof(data), data);
			//std::cout << data[0] << " " << data[1] << " " << data[2] << std::endl;
		}
		auto time_height = std::chrono::steady_clock::now() - time_heightstart;
		std::cout << "total time for map height generation: " << (float)time_height.count() / 1000000000 << std::endl;

		//std::cout << pow(map_size, 2) - map_size << std::endl;
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBO);
		glGetBufferSubData(GL_ARRAY_BUFFER, 8 * 4, sizeof(data), data);
		//std::cout << data[3] << " " << data[4] << " " << data[5] << std::endl;

		auto time_generationstart = std::chrono::steady_clock::now();
		for (int i = 0; i < pow(map_size, 2); i++)
		{
			get_vertdata(data, i);
			get_vertnormal(data, i);
			glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(data), sizeof(data), data);
		}
		auto time_gen = std::chrono::steady_clock::now() - time_heightstart;
		std::cout << "total time for map mesh generation: " << (float)time_gen.count() / 1000000000 << std::endl;
		std::cout << "total time: " << (float)(time_gen + time_height).count() / 1000000000 << std::endl;
		GLfloat face_data[24];
	};

	void load_color()
	{
		tShader->use(); //I am an idiot, forgot this and was chasing texture assign bugs
		add_texture("../assets/france_testmap/colormap.png", "main_albedo");
		add_texture("../assets/france_testmap/forestground_albedo.png", "subtex_land");
		add_texture("../assets/france_testmap/water1.png", "subtex_water");

		glGenTextures(1, &splat_map);
		glBindTexture(GL_TEXTURE_2D, splat_map);
		if (height_image_data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, width, length, 0, GL_RED, GL_UNSIGNED_SHORT, height_image_data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture (splat)" << std::endl;
		}
		stbi_image_free(color_data2);
		tShader->setInt("splatMap", textures.size());
		textures.push_back(splat_map);
		stbi_image_free(height_image_data);

		add_decal("../assets/france_testmap/airfield_decal.png");

		tShader->setInt("subtex_land", 1);
	}
	void add_decal(const char decal_albedo_path[])
	{
		unsigned char* color_data;
		unsigned int texture_int;
		int dheight, dwidth;
		glGenTextures(1, &texture_int);
		glBindTexture(GL_TEXTURE_2D, texture_int);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		stbi_set_flip_vertically_on_load(true);
		color_data = stbi_load(decal_albedo_path, &dwidth, &dheight, &channels, 0);
		if (color_data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dwidth, dheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, color_data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(color_data);
		
		tShader->setInt("decals[" + std::to_string(decalCount++) + "].albedo", textures.size());
		textures.insert(textures.end(), texture_int);
	}
	void add_texture(const char texture_path[], const char target[])
	{
		unsigned int texture_int;
		unsigned char* color_data;
		int c_channels;
		int llength, lwidth;
		color_data = stbi_load(texture_path, &lwidth, &llength, &c_channels, 0);
		glGenTextures(1, &texture_int);
		glBindTexture(GL_TEXTURE_2D, texture_int); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (color_data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, lwidth, llength, 0, GL_RGB, GL_UNSIGNED_BYTE, color_data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture: " << target << std::endl;
		}
		tShader->setInt(target, textures.size());
		textures.insert(textures.end(), texture_int);
		stbi_image_free(color_data);
	}
	void draw()
	{
		tShader->use();
		for (int i = 0; i < textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, textures[i]);
		}
		glm::mat4 modelmat = glm::translate(glm::dmat4(1.0), glm::dvec3(0.0f, -2.0f, 0.0f) -*cameraPosPtr);
		tShader->setMat4("model", modelmat);



		glBindBuffer(GL_ARRAY_BUFFER, tVBO);
		glBindVertexArray(tVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBO);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//glDrawArrays(GL_TRIANGLES, 0, pow(terrain_size - 1, 2) * 24 - 5);
		glDrawElements(GL_TRIANGLES, pow(width - 1, 2) * 6, GL_UNSIGNED_INT, 0);
	}
	void set_cameraPosPtr(glm::dvec3* cameraPos)
	{
		cameraPosPtr = cameraPos;
	}
private:
	void get_vertdata(GLfloat *coordinates, const int &ind)
	{
		coordinates[0] = float(ind % map_size) * u_per_px - (map_size * u_per_px) / 2 + 0.5 * u_per_px;
		coordinates[1] = height_data[ind];
		coordinates[2] = -(map_size * u_per_px) / 2 +(0.5f * u_per_px) + float(floor(ind / map_size)) * u_per_px;
		coordinates[6] = float(ind % map_size) / (map_size - 1);
		coordinates[7] = float(floor(ind / map_size)) / map_size;
		//get_vertnormal(coordinates, ind);
	}
	void get_vertlocation(GLfloat* coordinates, const int &ind)
	{
		coordinates[0] = float(ind % map_size) * u_per_px - (map_size * u_per_px) / 2 + 0.5 * u_per_px;
		coordinates[1] = height_data[ind];
		coordinates[2] = -(map_size * u_per_px) / 2 + (0.5f * u_per_px) + float(floor(ind / map_size)) * u_per_px;
	}
	void gen_facenormal(GLfloat* face_data)
	{
		glm::vec3 AB(face_data[8] - face_data[0], face_data[9] - face_data[1], face_data[10] - face_data[2]);
		glm::vec3 AC(face_data[16] - face_data[0], face_data[17] - face_data[1], face_data[18] - face_data[2]);

		glm::vec3 normal = cross(AB, AC);
		if (normal.y < 0)
		{
			normal *= -1;
		}
		for (int j = 0; j < 3; j++)
		{
			face_data[j * 8 + 3] = normal.x;
			face_data[j * 8 + 4] = normal.y;
			face_data[j * 8 + 5] = normal.z;
		}
	}
	glm::vec3* get_facenormal(GLfloat* face_data)
	{
		glm::vec3 AB(face_data[3] - face_data[0], face_data[4] - face_data[1], face_data[5] - face_data[2]);
		glm::vec3 AC(face_data[6] - face_data[0], face_data[7] - face_data[1], face_data[8] - face_data[2]);
		//std::cout << AB.x << " " << AB.y << " " << AB.z << std::endl;

		glm::vec3 normal = glm::normalize(cross(AB, AC));
		if (normal.y < 0)
		{
			normal *= -1;
		}
		return &normal;
	}
	void get_vertnormal(GLfloat* vert_data, const int &ind) //this goes through all the faces connected to a vertex clockwise, starting at the bottom right.
	{
		glm::vec3 normal(0);
		GLfloat face_points[9];
		get_vertlocation(face_points, ind);
		int casecount = 0;
		for (int i = 0; i < 6; i++)
		{
			switch (i)
			{
			case 0:
				if (ind % map_size < map_size - 1 && ind < pow(map_size, 2) - map_size)
				{
					get_vertlocation(&face_points[3], ind + map_size);
					get_vertlocation(&face_points[6], ind + 1);
					normal += *get_facenormal(face_points);
					casecount++;
				}
				break;
			case 1:
				if (ind % map_size < map_size - 1 && ind >= map_size)
				{
					get_vertlocation(&face_points[3], ind + 1);
					get_vertlocation(&face_points[6], ind - map_size + 1);
					normal += *get_facenormal(face_points);
					casecount++;
				}
				break;
			case 2:
				if (ind % map_size < map_size - 1 && ind >= map_size)
				{
					get_vertlocation(&face_points[3], ind - map_size);
					get_vertlocation(&face_points[6], ind - map_size + 1);
					normal += *get_facenormal(face_points);
					casecount++;
				}
				break;
			case 3:
				if (ind % map_size > 0 && ind >= map_size)
				{
					get_vertlocation(&face_points[3], ind - 1);
					get_vertlocation(&face_points[6], ind - map_size);
					normal += *get_facenormal(face_points);
					casecount++;
				}
				break;
			case 4:
				if (ind % map_size > 0 && ind <= pow(map_size, 2) - map_size)
				{
					get_vertlocation(&face_points[3], ind - 1);
					get_vertlocation(&face_points[6], ind + map_size - 1);
					normal += *get_facenormal(face_points);
					casecount++;
				}
				break;
			case 5:
				if (ind % map_size > 0 && ind <= pow(map_size, 2) - map_size)
				{
					get_vertlocation(&face_points[3], ind + map_size);
					get_vertlocation(&face_points[6], ind + map_size - 1);
					normal += *get_facenormal(face_points);
					casecount++;
				}
			}
			
		}
		vert_data[3] = glm::normalize(normal).x;
		vert_data[4] = glm::normalize(normal).y;
		vert_data[5] = glm::normalize(normal).z;
		//std::cout << "vert normal: " << vert_data[3] << " " << vert_data[4] << " " << vert_data[5] << std::endl;
	}
};
