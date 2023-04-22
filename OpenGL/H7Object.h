#pragma once
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include "shader.h"
#include "model.h"
#include "H7Asset.h"
#include "H7AssetManager.h"
#include <thread>
#include "Bullet3.24/btBulletDynamicsCommon.h"

class H7Object
{
public:
	glm::mat4 transformMat = glm::mat4(1.0); //this includes the modifier, the modifier will be used whenever the transform is modified.
	glm::mat4 modifierMat = glm::mat4(1.0);
	H7AssetManager* asset_manager_target;
	int asset_id;
	H7Object();
	H7Object(H7AssetManager &asset_manager, std::string name, std::vector<std::string> LOD_dirs, bool load)
	{
		asset_manager_target = &asset_manager;
		asset_id = asset_manager.add_asset(name, LOD_dirs);
	}
	H7Object(H7AssetManager& asset_manager, Shader &shader, std::string name, std::vector<std::string> LOD_dirs, bool load)
	{
		asset_manager_target = &asset_manager;
		asset_id = asset_manager.add_asset(shader, name, LOD_dirs);
	}
	H7Object(H7AssetManager &asset_manager, std::string name)
	{
		asset_manager_target = &asset_manager;
		asset_id = asset_manager.get_asset_id(name);
	}
	void setModifier(glm::mat4 setMat)
	{
		modifierMat = setMat;
	}
	void setTransform(glm::mat4 setMat)
	{
		transformMat = setMat;
	}
	void setTransform(glm::dmat4 setMat)
	{
		transformMat = setMat;
	}
	void setTransform(btTransform btTransform, glm::dvec3 cameraPos) //I am hoping to make this unnecessary in the future with the use of dmat4 and in shader conversions.
	{
		glm::dmat4 matPointer = glm::dmat4(1.0);
		btTransform.getOpenGLMatrix((btScalar*)&matPointer);
		transformMat =  glm::mat4(glm::translate(glm::dmat4(1.0), -cameraPos) * matPointer) * modifierMat;
	}
	void drawAction(int LOD_index)
	{
		asset_manager_target->submitDrawAction(transformMat, asset_id, LOD_index);
	}
	void draw(int LOD_index)
	{
		asset_manager_target->Assets[asset_id]->Draw(LOD_index, transformMat);
	}
	void drawShadow(Shader& shader, int LOD_index)
	{
		shader.setMat4("model", transformMat);
		asset_manager_target->Assets.at(asset_id)->Draw(shader, 0);
	}
};

