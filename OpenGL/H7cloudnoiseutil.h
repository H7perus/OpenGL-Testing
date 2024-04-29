#pragma once
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtx/rotate_vector.hpp>
#include <random>
#include <thread>
#include <filesystem>
#include <fstream>
#include "glad/glad.h"
#include "GLFW/glfw3.h"

void threaded_worley_noise(float* noiseData, int frequency, glm::vec3* points, int point_amount, int size, int start_height, int end_height, int mode, float persistence)
{


	for (int x = start_height; x < end_height; x++)
		for (int y = 0; y < size; y++)
			for (int z = 0; z < size; z++)
			{
				glm::vec3 position(glm::vec3((x + 0.5) / size, (y + 0.5) / size, (z + 0.5) / size));
				glm::vec3 closest = points[0];
				float prev_length = glm::length(position - closest);
				for (int i = 1; i < point_amount * 27; i++)
				{
					if (glm::length(position - points[i]) < prev_length)
					{
						closest = points[i];
						prev_length = glm::length(position - points[i]);
					}
				}

				noiseData[x + y * size + z * size * size] += ((1 - glm::clamp(prev_length * frequency, 0.0f, 1.0f)) * (1 + mode) - mode) * persistence;
			}
}
void worley_noise_generator(float* noiseData, int frequency, int tex_size, float persistence, int seed = 0, int mode = 0)
{
	std::mt19937 gen(seed);
	std::uniform_real_distribution<> dis(0.0, 1.0);

	int point_amount = pow(frequency, 3);

	glm::vec3* points = new glm::vec3[point_amount * 27];

	for (int i = 0; i < point_amount; i++)
	{
		glm::vec3 local_point = glm::vec3(dis(gen), dis(gen), dis(gen));

		points[27 * i + 0] = local_point + glm::vec3(0, 0, 0);
		points[27 * i + 1] = local_point + glm::vec3(-1, 0, 0);
		points[27 * i + 2] = local_point + glm::vec3(1, 0, 0);
		points[27 * i + 3] = local_point + glm::vec3(0, 0, 1);
		points[27 * i + 4] = local_point + glm::vec3(0, 0, -1);
		points[27 * i + 5] = local_point + glm::vec3(1, 0, 1);
		points[27 * i + 6] = local_point + glm::vec3(-1, 0, 1);
		points[27 * i + 7] = local_point + glm::vec3(1, 0, -1);
		points[27 * i + 8] = local_point + glm::vec3(-1, 0, -1);

		points[27 * i + 9] = local_point + glm::vec3(0, 1, 0);
		points[27 * i + 10] = local_point + glm::vec3(-1, 1, 0);
		points[27 * i + 11] = local_point + glm::vec3(1, 1, 0);
		points[27 * i + 12] = local_point + glm::vec3(0, 1, 1);
		points[27 * i + 13] = local_point + glm::vec3(0, 1, -1);
		points[27 * i + 14] = local_point + glm::vec3(1, 1, 1);
		points[27 * i + 15] = local_point + glm::vec3(-1, 1, 1);
		points[27 * i + 16] = local_point + glm::vec3(1, 1, -1);
		points[27 * i + 17] = local_point + glm::vec3(-1, 1, -1);

		points[27 * i + 18] = local_point + glm::vec3(0, -1, 0);
		points[27 * i + 19] = local_point + glm::vec3(-1, -1, 0);
		points[27 * i + 20] = local_point + glm::vec3(1, -1, 0);
		points[27 * i + 21] = local_point + glm::vec3(0, -1, 1);
		points[27 * i + 22] = local_point + glm::vec3(0, -1, -1);
		points[27 * i + 23] = local_point + glm::vec3(1, -1, 1);
		points[27 * i + 24] = local_point + glm::vec3(-1, -1, 1);
		points[27 * i + 25] = local_point + glm::vec3(1, -1, -1);
		points[27 * i + 26] = local_point + glm::vec3(-1, -1, -1);
	}


	std::thread thread0 = std::thread(threaded_worley_noise, noiseData, frequency, points, point_amount, tex_size, 0, tex_size / 4, mode, persistence);
	std::thread thread1 = std::thread(threaded_worley_noise, noiseData, frequency, points, point_amount, tex_size, tex_size / 4, tex_size / 2, mode, persistence);
	std::thread thread2 = std::thread(threaded_worley_noise, noiseData, frequency, points, point_amount, tex_size, tex_size / 2, tex_size / 4 * 3, mode, persistence);
	std::thread thread3 = std::thread(threaded_worley_noise, noiseData, frequency, points, point_amount, tex_size, tex_size / 4 * 3, tex_size, mode, persistence);
	thread0.join();
	thread1.join();
	thread2.join();
	thread3.join();
}
void three_ch_worley_from_to_file(std::string file_name, float* noiseData, int frequency, int tex_size, float persistence, int seed = 0, int mode = 0)
{

	//noiseData = new GLfloat[tex_size * tex_size * tex_size * 3]();
	if (std::filesystem::exists(file_name))
	{
		std::ifstream file(file_name, std::ios::binary | std::ios::ate);

		std::streampos fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		file.read((char*)noiseData, fileSize);

		file.close();
	}
	else
	{
		for (int i = 1; i < 4; i++)
		{
			GLfloat* tempdetailWorleyNoiseData = new GLfloat[tex_size * tex_size * tex_size]();

			worley_noise_generator(tempdetailWorleyNoiseData, frequency * i, tex_size, persistence, seed, mode);

			for (int j = 0; j < tex_size * tex_size * tex_size; j++)
			{
				noiseData[j * 3 + (i - 1)] = tempdetailWorleyNoiseData[j];
			}
		}
		std::ofstream out(file_name, std::ios::binary);
		out.write((char*)(noiseData), pow(tex_size, 3) * 3 * 4);
		out.close();
	}
}


void thread_perlin_noise(std::vector<std::vector<std::vector<glm::vec3>>>& cornerVectors, float* noiseData, int perlin_size, int perlin_tex_size, int start_height, int end_height, float persistence)
{
	for (int x = 0; x < perlin_tex_size; x++)
		for (int y = 0; y < perlin_tex_size; y++)
			for (int z = start_height; z < end_height; z++)
			{
				glm::ivec3 box_coords = (glm::ivec3(x, y, z) + glm::ivec3(perlin_tex_size / (2 * perlin_size))) * perlin_size / perlin_tex_size;
				//FIX THIS,ASAP
				glm::vec3 point_coords = (glm::vec3(x, y, z) + glm::vec3(perlin_tex_size / (2 * perlin_size))) / float(perlin_tex_size / perlin_size) - glm::vec3(box_coords);

				box_coords.x = box_coords.x % perlin_size;
				box_coords.y = box_coords.y % perlin_size;
				box_coords.z = box_coords.z % perlin_size;

				float c000 = glm::dot(point_coords - glm::vec3(0, 0, 0), cornerVectors[(box_coords.x)][(box_coords.y)][box_coords.z]);

				float c001 = glm::dot(point_coords - glm::vec3(0, 0, 1), cornerVectors[(box_coords.x)][(box_coords.y)][(box_coords.z + 1) % perlin_size]);
				float c010 = glm::dot(point_coords - glm::vec3(0, 1, 0), cornerVectors[(box_coords.x)][(box_coords.y + 1) % perlin_size][box_coords.z]);
				float c011 = glm::dot(point_coords - glm::vec3(0, 1, 1), cornerVectors[(box_coords.x)][(box_coords.y + 1) % perlin_size][(box_coords.z + 1) % perlin_size]);
				float c110 = glm::dot(point_coords - glm::vec3(1, 1, 0), cornerVectors[(box_coords.x + 1) % perlin_size][(box_coords.y + 1) % perlin_size][box_coords.z]);
				float c111 = glm::dot(point_coords - glm::vec3(1, 1, 1), cornerVectors[(box_coords.x + 1) % perlin_size][(box_coords.y + 1) % perlin_size][(box_coords.z + 1) % perlin_size]);
				float c100 = glm::dot(point_coords - glm::vec3(1, 0, 0), cornerVectors[(box_coords.x + 1) % perlin_size][(box_coords.y)][box_coords.z]);
				float c101 = glm::dot(point_coords - glm::vec3(1, 0, 1), cornerVectors[(box_coords.x + 1) % perlin_size][(box_coords.y)][(box_coords.z + 1) % perlin_size]);

				float c00 = c000 + glm::smoothstep(0.0f, 1.0f, point_coords.z) * (c001 - c000);
				float c01 = c010 + glm::smoothstep(0.0f, 1.0f, point_coords.z) * (c011 - c010);
				float c11 = c110 + glm::smoothstep(0.0f, 1.0f, point_coords.z) * (c111 - c110);
				float c10 = c100 + glm::smoothstep(0.0f, 1.0f, point_coords.z) * (c101 - c100);

				float c0 = c00 + glm::smoothstep(0.0f, 1.0f, point_coords.y) * (c01 - c00);
				float c1 = c10 + glm::smoothstep(0.0f, 1.0f, point_coords.y) * (c11 - c10);

				float c = c0 + glm::smoothstep(0.0f, 1.0f, point_coords.x) * (c1 - c0);
				noiseData[x + y * perlin_tex_size + z * perlin_tex_size * perlin_tex_size] += persistence * glm::clamp(c / 0.7071, -1.0, 1.0);
			}
}

void perlin_noise_generator(float* noiseData, int perlin_size, int perlin_tex_size, float persistence, int seed = 0)
{
	std::mt19937 gen(seed);
	std::uniform_real_distribution<> dis(0.0, 1.0);

	std::vector<std::vector<std::vector<glm::vec3>>> cornerVectors(perlin_size, std::vector<std::vector<glm::vec3>>(perlin_size, std::vector<glm::vec3>(perlin_size, glm::vec3(0))));

	for (auto& vec1 : cornerVectors)
		for (auto& vec2 : vec1)
			for (glm::vec3& cornerVector : vec2)
			{
				cornerVector = glm::normalize(glm::vec3(dis(gen) - 0.5, dis(gen) - 0.5, dis(gen) - 0.5));

				cornerVector = glm::rotate(glm::vec3(0, 1, 0), float(asin(dis(gen))), glm::vec3(0, 0, 1));
				cornerVector = glm::rotate(cornerVector, float(dis(gen) * acos(-1)), glm::vec3(0, 1, 0));
				cornerVector = glm::normalize(cornerVector - glm::vec3(0.0, 0.5, 0.0));
			}

	std::thread thread0 = std::thread(thread_perlin_noise, std::ref(cornerVectors), noiseData, perlin_size, perlin_tex_size, 0, perlin_tex_size / 4, persistence);
	std::thread thread1 = std::thread(thread_perlin_noise, std::ref(cornerVectors), noiseData, perlin_size, perlin_tex_size, perlin_tex_size / 4, perlin_tex_size / 2, persistence);
	std::thread thread2 = std::thread(thread_perlin_noise, std::ref(cornerVectors), noiseData, perlin_size, perlin_tex_size, perlin_tex_size / 2, perlin_tex_size / 4 * 3, persistence);
	std::thread thread3 = std::thread(thread_perlin_noise, std::ref(cornerVectors), noiseData, perlin_size, perlin_tex_size, perlin_tex_size / 4 * 3, perlin_tex_size, persistence);
	thread0.join();
	thread1.join();
	thread2.join();
	thread3.join();
}

void perlinworley_from_to_file(std::string file_name, float* noiseData, int tex_size, int seed = 0)
{
	int perlin_worley_tex_size = tex_size;
	

	if (std::filesystem::exists(file_name))
	{

		std::ifstream file(file_name, std::ios::binary | std::ios::ate);
		std::streampos fileSize = file.tellg();
		file.seekg(0, std::ios::beg);
		file.read((char*)noiseData, fileSize);
		file.close();


	}
	else
	{
		float total_amplitude = (1.0 + 0.5 + 0.25) + (1.0 + 0.5 + 0.25);
		//perlin_noise_generator(perlinWorleyNoiseData, 16, perlin_worley_tex_size, 1.0f / total_amplitude);
		perlin_noise_generator(noiseData, 32, perlin_worley_tex_size, 0.5f / total_amplitude, 0 + seed);
		perlin_noise_generator(noiseData, 64, perlin_worley_tex_size, 0.25f / total_amplitude, 1 + seed);
		worley_noise_generator(noiseData, 6, perlin_worley_tex_size, 1.0 / total_amplitude, 0, 0 + seed);
		worley_noise_generator(noiseData, 12, perlin_worley_tex_size, 0.5 / total_amplitude, 0, 1 + seed);
		worley_noise_generator(noiseData, 24, perlin_worley_tex_size, 0.5 / total_amplitude, 0, 2 + seed); //take a look at this later
		std::ofstream out(file_name, std::ios::binary);

		out.write((char*)(noiseData), tex_size * tex_size * tex_size * sizeof(GLfloat));

		out.close();
	}
}
