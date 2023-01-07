#pragma once
#include <glm/glm/glm.hpp>
class physicsObject
{
public:
	glm::vec3 pos;
	glm::vec3 rotation;
	glm::vec3 rotational_vel;
	glm::vec3 vel;
	float mass;
};

