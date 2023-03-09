#pragma once
#include "glad/glad.h"
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include "H7Object.h"


class shadowMap
{
public:
	Shader* shadowDepthShader;
	unsigned int SHADOW_WIDTH, SHADOW_HEIGHT;
	unsigned int depthMapFBO;
	unsigned int depthMap;
	glm::vec3 lightPos = glm::vec3(15.0f, 15.0f, 15.0f);
	vector<H7Object*> models;

	shadowMap(int shadow_size, Shader* inputShadowShader) 
	{
		shadowDepthShader = inputShadowShader;

		SHADOW_WIDTH = shadow_size, SHADOW_HEIGHT = shadow_size;
		glGenFramebuffers(1, &depthMapFBO);
		glGenTextures(1, &depthMap);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	glm::mat4 draw()
	{
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		float near_plane = 1.0f, far_plane = 100.0f;
		lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
		glm::vec3 plane_pos = glm::vec3(0.0);
		lightView = glm::lookAt(lightPos + plane_pos, plane_pos, glm::normalize(glm::vec3(-0.0, 1.0, -0.0)));
		lightSpaceMatrix = lightProjection * lightView;
		// render scene from light's point of view =========================================
		shadowDepthShader->use();
		shadowDepthShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);
		for (H7Object* model : models)
		{
			shadowDepthShader->setMat4("model", model->transformMat);
			model->Draw(*shadowDepthShader);
		}
		return lightSpaceMatrix;
	}
};

