#pragma once
#include <string>
#include "stb_image.h"
#include <iostream>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include "Bullet3.24/btBulletDynamicsCommon.h"
#include "Bullet3.24/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "Bullet3.24/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include "Bullet3.24/BulletCollision/CollisionShapes/btTriangleShape.h"

#include "shader.h"
#include <chrono>
#include <vector>
#include <cmath>


struct H7TerrainSun
{
	glm::vec3 ambient = glm::vec3(1.0);
	glm::vec3 diffuse = glm::vec3(1.0);
	glm::vec3 direction = glm::vec3(1.0);
};

//I need to implement configs for the maps as a replacement for hardcoding, like a JSON for example. I will continue to hardcode for a bit as I learn more.
class H7TerrainTile
{
public:
	double *height_data;//this will also be used for btHeightfieldTerrainShape
	int map_size;
	int width, length;

	glm::dvec2 position_offset;

	int channels = 1;
	int EBO_size = 0;
	GLuint tVBO, tVAO, tEBOLOD1, tEBO;
	std::vector<GLfloat> tVBOcontent, tVAOcontent; //this is used to facilitate more performant threading on map load
	std::vector<GLuint> tEBOLOD1content, tEBOcontent; //this is used to facilitate more performant threading on map load
	Shader* tShader;
	float u_per_px;

	unsigned int textureCount = 0;
	unsigned int decalCount = 0;
	std::vector<unsigned int> textures;
	std::vector<glm::dvec3> treelocations;
	btRigidBody* physics_body;

	unsigned int ground_texture, ground_texture2, water_texture, splat_map, airfield_decal; //this will be changed in the future, just having one texture is a pretty bad approach
	uint16_t* height_image_data;
	unsigned char *color_data2, *color_data3;


	H7TerrainTile(const int map_size_in, std::string terrain_heightmap, float min_height, float max_height, const float u_per_px_constructor, glm::dvec2 pos_offset, Shader* in_Shader);
	void finalize_load(); //imagine if I could actually do stuff in OpenGL in another thread easily.
	void load_color(std::string terrain_albedomap);
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
	void Draw(int LOD_level)
	{
		tShader->use();
		for (int i = 0; i < textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, textures[i]);
		}
		
		if (LOD_level == 1)
		{
			glBindBuffer(GL_ARRAY_BUFFER, tVBO);
			glBindVertexArray(tVAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBOLOD1);
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			//glDrawArrays(GL_TRIANGLES, 0, pow(terrain_size - 1, 2) * 24 - 5);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		else
		{
			glBindBuffer(GL_ARRAY_BUFFER, tVBO);
			glBindVertexArray(tVAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBO);
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			//glDrawArrays(GL_TRIANGLES, 0, pow(terrain_size - 1, 2) * 24 - 5);
			glDrawElements(GL_TRIANGLES, pow(width - 1, 2) * 6, GL_UNSIGNED_INT, 0); // pow(width - 1, 2) * 6
		}
		
	}

	void generate_tree_locations(const char texture_path[])
	{
		int tree_density = 100;
		int t_width, t_height, t_channels;
		stbi_set_flip_vertically_on_load(false);
		uint8_t* treedata = (uint8_t*)(stbi_load(texture_path, &t_width, &t_height, &t_channels, 1));
		srand(1);
		float actual_mapsize = (map_size * u_per_px - u_per_px);
		float actual_px_width = actual_mapsize / t_width;
		float actual_px_height = actual_mapsize / t_height;
		
		for(int x = 0; x < t_width; x++)
			for (int y = 0; y < t_height; y++)
			{
				int tree_amount = treedata[y * t_width + x] * tree_density / 256;

				for (int i = 0; i < tree_amount; i++)
				{
					double x_coord = -actual_mapsize / 2 + x * actual_px_width +(rand() % 1000) / 1000.0 * actual_px_width;
					double y_coord = -actual_mapsize / 2 + y * actual_px_height +(rand() % 1000) / 1000.0 * actual_px_height;
					treelocations.push_back(glm::dvec3(x_coord, 10.0f, y_coord));
				}
			}


		stbi_image_free(treedata);
	};
private:
	void get_vertdata(GLfloat *coordinates, const int &ind, float min_height, float max_height)
	{
		//float range = max_height - min_height;
		coordinates[0] = float(ind % map_size) * u_per_px - (map_size * u_per_px) / 2 + 0.5 * u_per_px;
		coordinates[1] = float(height_data[ind]);
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
