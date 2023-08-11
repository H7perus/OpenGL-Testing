//The Asset management class. The intent is to control things like asset loading/unloading and instancing.

#pragma once
#include <vector>
#include "H7Asset.h"





struct draw_action 
{
	int asset_index;
	int lod_index;
	glm::mat4 transform;
	draw_action(int asset_index, int lod_index, glm::mat4 transform)
	{
		this->asset_index = asset_index;
		this->lod_index = lod_index;
		this->transform = transform;
	}
};

struct draw_action_position 
{
	int asset_index;
	int lod_index;
	glm::dvec4 pos_rot;
	draw_action_position(int asset_index, int lod_index, glm::dvec3 position, double rotation)
	{
		this->asset_index = asset_index;
		this->lod_index = lod_index;
		this->pos_rot = glm::dvec4(position, 1.0);
		this->pos_rot.w = rotation;
	}
};


static bool compare_calls(const draw_action& draw_action_a, const draw_action& draw_action_b)
{
	if (draw_action_a.asset_index < draw_action_b.asset_index)
		return true;
	if (draw_action_a.asset_index == draw_action_b.asset_index && draw_action_a.lod_index < draw_action_b.lod_index)
		return true;
	return false;
}
static bool compare_calls_position(const draw_action_position& draw_action_a, const draw_action_position& draw_action_b)
{
	if (draw_action_a.asset_index < draw_action_b.asset_index)
		return true;
	if (draw_action_a.asset_index == draw_action_b.asset_index && draw_action_a.lod_index < draw_action_b.lod_index)
		return true;
	return false;
}

class H7AssetManager
{
public:
	std::vector<H7Asset*> Assets;
	std::vector<draw_action> current_draw_actions;
	std::vector<draw_action_position> current_draw_actions_position;

	int get_asset_id(std::string name)
	{
		for (int i = 0; i < Assets.size(); i++)
		{
			std::cout << Assets.at(i)->name << " vs " << name << std::endl;
			if (Assets.at(i)->name == name)
			{
				return i;
			}
		}
		std::cout << "returning -1" << std::endl;
		return -1;
	}
	int add_asset(std::string name, std::vector<std::string> LOD_dirs)
	{
		int asset_id = get_asset_id(name);
		if (asset_id == -1)
		{
			std::cout << "asset ID -1, thats pretty bad ngl" << std::endl;
			Assets.push_back(new H7Asset(name, LOD_dirs, true));
			asset_id = Assets.size() - 1;
		}

		return asset_id;
	}
	int add_asset(Shader& shader, std::string name, std::vector<std::string> LOD_dirs)
	{
		int asset_id = get_asset_id(name);
		if (asset_id == -1)
		{
			std::cout << "asset ID -1, thats pretty bad ngl" << std::endl;
			Assets.push_back(new H7Asset(shader, name, LOD_dirs, true));
			asset_id = Assets.size() - 1;
		}

		return asset_id;
	}
	void submitDrawAction(glm::mat4& transform, int& asset_index, int& lod_index)
	{
		current_draw_actions.push_back(draw_action(asset_index, lod_index, transform));
	}
	void submitDrawActionPosition(glm::dvec3& position, double& rotation, int& asset_index, int& lod_index)
	{
		current_draw_actions_position.push_back(draw_action_position(asset_index, lod_index, position, rotation));
	}
	void drawInstanced()
	{
		/*if (current_draw_actions.size() == 0)
			return;*/
		std::vector<std::vector<glm::mat4>> instanceList;
		std::vector<glm::mat4> instanceMats;
		std::sort(current_draw_actions.begin(), current_draw_actions.end(), compare_calls);
		//std::cout << "woohoo wa wawa, this should only be called once-2" << std::endl;
		int prev_asset_index = -1;
		int prev_lod_index = -1;
		draw_action draw_action = current_draw_actions.at(0);
		for (int i = 0; i <= current_draw_actions.size(); i++)
		{

			//std::cout << "woohoo wa wawa, this should only be called once-1" << std::endl;
			if (i != current_draw_actions.size())
				draw_action = current_draw_actions.at(i);
			if (i == 0)
			{
				prev_asset_index = draw_action.asset_index;
				prev_lod_index = draw_action.lod_index;
			}
			if (((prev_asset_index != draw_action.asset_index || prev_lod_index != draw_action.lod_index) || i == current_draw_actions.size()) && instanceMats.size())
			{


				unsigned int buffer;
				glGenBuffers(1, &buffer);
				glBindBuffer(GL_ARRAY_BUFFER, buffer);
				glBufferData(GL_ARRAY_BUFFER, instanceMats.size() * sizeof(glm::mat4), instanceMats.data(), GL_STATIC_DRAW);
				Model model = ref(Assets.at(prev_asset_index)->LOD_levels.at(prev_lod_index)->model);
				Shader shader = ref(*Assets.at(prev_asset_index)->LOD_levels.at(prev_lod_index)->shader);
				shader.use();
				shader.setInt("Instanced", 1);
				//std::cout << "woohoo wa wawa, this should only be called once" << std::endl;
				for (unsigned int i = 0; i < model.meshes.size(); i++)
				{
					//std::cout << "woohoo wa wawa, this should only be called once1" << std::endl;
					unsigned int VAO = model.meshes[i].VAO;
					glBindVertexArray(VAO);
					// vertex attributes
					std::size_t vec4Size = sizeof(glm::vec4);
					glEnableVertexAttribArray(3);
					glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
					glEnableVertexAttribArray(4);
					glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
					glEnableVertexAttribArray(5);
					glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
					glEnableVertexAttribArray(6);
					glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

					glVertexAttribDivisor(5, 1);
					glVertexAttribDivisor(6, 1);
					glVertexAttribDivisor(7, 1);
					glVertexAttribDivisor(8, 1);

					shared_ptr<LOD_level> LOD_level = Assets.at(prev_asset_index)->LOD_levels.at(prev_lod_index);
					if (Assets.at(prev_asset_index)->check_if_loaded_else_load(prev_lod_index))
					{
						model.meshes[i].setTextures(shader);
						//std::cout << shader.ID << " " << model.meshes[i].indices.size() << " " << instanceMats.size() << std::endl;
						glDrawElementsInstanced(GL_TRIANGLES, model.meshes[i].indices.size(), GL_UNSIGNED_INT, 0, instanceMats.size());
					}
					glBindVertexArray(0);
				}
				prev_asset_index = draw_action.asset_index;
				prev_lod_index = draw_action.lod_index;
				glDeleteBuffers(1, &buffer);
				instanceMats.clear();
				shader.setBool("Instanced", 0);
			}
			instanceMats.push_back(draw_action.transform);

		}
		current_draw_actions.clear();
	}
	void drawInstancedPositions(bool clear_data = false)
	{
		if (current_draw_actions_position.size() == 0)
			return;
		std::vector<glm::dvec4> instancePosRots;
		std::sort(current_draw_actions_position.begin(), current_draw_actions_position.end(), compare_calls_position);
		int prev_asset_index = -1;
		int prev_lod_index = -1;
		draw_action_position draw_action = current_draw_actions_position.at(0);
		auto stuffStart = std::chrono::steady_clock::now();
		for (int i = 0; i <= current_draw_actions_position.size(); i++)
		{
			//std::cout << "woohoo wa wawa, this should only be called once-1" << std::endl;
			if (i != current_draw_actions_position.size())
				draw_action = current_draw_actions_position.at(i);
			if (i == 0)
			{
				prev_asset_index = draw_action.asset_index;
				prev_lod_index = draw_action.lod_index;
			}
			if (((prev_asset_index != draw_action.asset_index || prev_lod_index != draw_action.lod_index) || i == current_draw_actions_position.size()) && instancePosRots.size())
			{
				
				unsigned int buffer;
				
				glGenBuffers(1, &buffer);
				glBindBuffer(GL_ARRAY_BUFFER, buffer);
				glBufferData(GL_ARRAY_BUFFER, instancePosRots.size() * sizeof(glm::dvec4), instancePosRots.data(), GL_STATIC_DRAW);
				

				Model &model = ref(Assets.at(prev_asset_index)->LOD_levels.at(prev_lod_index)->model);
				Shader &shader = ref(*Assets.at(prev_asset_index)->LOD_levels.at(prev_lod_index)->shader);
				shader.use();
				shader.setInt("Instanced", 2);
				for (unsigned int i = 0; i < model.meshes.size(); i++)
				{
					unsigned int VAO = model.meshes[i].VAO;
					glBindVertexArray(VAO);
					// vertex attributes
					int vec4Size = sizeof(glm::vec4);
					glEnableVertexAttribArray(5);
					glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
					glEnableVertexAttribArray(6);
					glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
					glEnableVertexAttribArray(7);
					glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
					glEnableVertexAttribArray(8);
					glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

					glVertexAttribDivisor(5, 1);
					glVertexAttribDivisor(6, 1);
					glVertexAttribDivisor(7, 1);
					glVertexAttribDivisor(8, 1);

					glEnableVertexAttribArray(6);
					glVertexAttribLPointer(6, 4, GL_DOUBLE, sizeof(glm::dvec4), (void*)0);

					shared_ptr<LOD_level> LOD_level = Assets.at(prev_asset_index)->LOD_levels.at(prev_lod_index);
					if (Assets.at(prev_asset_index)->check_if_loaded_else_load(prev_lod_index))
					{
						
						model.meshes[i].setTextures(shader);
						glDrawElementsInstanced(GL_TRIANGLES, model.meshes[i].indices.size(), GL_UNSIGNED_INT, 0, instancePosRots.size());
						
					}
					glBindVertexArray(0);
				}
				prev_asset_index = draw_action.asset_index;
				prev_lod_index = draw_action.lod_index;
				glDeleteBuffers(1, &buffer);
				instancePosRots.clear();
				shader.setInt("Instanced", 0);
				
			}
			instancePosRots.push_back(draw_action.pos_rot);
		}
		auto stuffTime = std::chrono::steady_clock::now() - stuffStart;

		std::cout << "time for treestuff: " << stuffTime.count() / 1000000.f << std::endl;
		current_draw_actions_position.clear();
	}
	void drawInstancedLegacy() //SLOW but whatever
	{
		for (draw_action draw_action : current_draw_actions)
		{
			Assets[draw_action.asset_index]->Draw(draw_action.lod_index, draw_action.transform);
		}
		current_draw_actions.clear();
	}
};

