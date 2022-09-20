#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

#include <cmath>

#include <iostream>
#include <string>
#include "shader.h"
#include "model.h"

#include "stb_image.h"

const float cameraSpeedMulti = 2.5f;
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;


void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);



int main()
{



	/*float vertices[] = {
	-0.5f, -0.5f, 0.0f, 0.5, 0.0, 0.0,  0.0f, 0.0f,
	 0.5f, -0.5f, 0.0f, 0.0, 0.5, 0.0,  1.0f, 0.0f,
	 0.5f,  0.5f, 0.0f, 0.0, 0.0, 0.5,  1.0f, 1.0f,
	-0.5f,  0.5f, 0.0f, 0.5, 0.5, 0.0,  0.0f, 1.0f
	};*/
	float vertices[] = {
		// positions          // normals           // texture coords
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
	};

	glm::vec3 cubePositions[] = {
	glm::vec3(0.0f,  0.0f,  0.0f),
	glm::vec3(2.0f,  5.0f, -15.0f),
	glm::vec3(-1.5f, -2.2f, -2.5f),
	glm::vec3(-3.8f, -2.0f, -12.3f),
	glm::vec3(2.4f, -0.4f, -3.5f),
	glm::vec3(-1.7f,  3.0f, -7.5f),
	glm::vec3(1.3f, -2.0f, -2.5f),
	glm::vec3(1.5f,  2.0f, -2.5f),
	glm::vec3(1.5f,  0.2f, -1.5f),
	glm::vec3(-1.3f,  1.0f, -1.5f)
	};

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	GLFWwindow* window = glfwCreateWindow(1600, 900, "OpenGLR", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, 1600, 900);

	Shader testShader("vertexShader.vert", "debugShader.frag");
	Shader testShader2("vertexShader.vert", "fragmentShader.frag");
	Shader lightShader("vertexShader.vert", "light_shader.frag");



	unsigned int VBO, VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);



	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	testShader.use();
	/*testShader.setInt("material.diffuse", 0);
	testShader.setInt("material.specular", 1);*/

	//transform matrix
	//glm::mat4 trans = glm::mat4(1.0f);
	//trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0)); 
	//trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));

	//unsigned int transformLoc = glGetUniformLocation(testShader.ID, "transform");
	//glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans)); 

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));


	glm::mat4 projection;
	projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);


	glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);

	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));

	float radius = 2.0f;
	glm::vec3 lights[2];

	glm::mat4 view;

	glfwSwapInterval(0);

	glEnable(GL_DEPTH_TEST);

	Model ourModel("../assets/Fw190A5.obj");
	Model floor("../assets/floor.obj");
	
	std::cout << ourModel.meshes.size() << std::endl;

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		glm::mat4 trans = glm::mat4(1.0f);
		trans = glm::translate(trans, glm::vec3(0.0f, -0.0f, 0.0f));
		trans = glm::rotate(trans, (float)glfwGetTime(), glm::vec3(0.0, 0.0, 1.0));
		testShader.use();
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		glClearColor(0.2f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT);

		testShader.setMat4("model", model);
		testShader.setMat4("projection", projection);
		testShader.setMat4("view", view);
		


		glBindVertexArray(VAO);

		lightShader.use();

		lightShader.setMat4("view", view);
		lightShader.setMat4("projection", projection);
		for (int i = 0; i < 2; i++)
		{
			lights[i] = glm::vec3(sin(glfwGetTime() * 0.5 + i) * radius, 0, cos(glfwGetTime() * 0.5 + i) * radius);
			glm::mat4 model = glm::translate(glm::mat4(1.0f), lights[i]);
			model = glm::scale(model, glm::vec3(0.2));

			lightShader.setMat4("model", model);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
		testShader.use();
		testShader.setVec3("pointLights[0].position", lights[0]);
		testShader.setVec3("pointLights[0].ambient", glm::vec3(0.5, 0.5, 0.5));
		testShader.setVec3("pointLights[0].diffuse", glm::vec3(0.5, 0.5, 0.5));
		testShader.setVec3("pointLights[0].specular", glm::vec3(0.8, 0.8, 0.8));
		testShader.setVec3("pointLights[1].position", lights[1]);
		testShader.setVec3("pointLights[1].ambient", glm::vec3(0.5, 0.5, 0.5));
		testShader.setVec3("pointLights[1].diffuse", glm::vec3(0.8, 0.8, 0.8));
		testShader.setVec3("pointLights[1].specular", glm::vec3(0.8, 0.8, 0.8));
		testShader.setVec3("sun.ambient", glm::vec3(0.1, 0.1, 0.1));
		testShader.setVec3("sun.diffuse", glm::vec3(0.5, 0.48, 0.45));
		testShader.setVec3("sun.specular", glm::vec3(0.5, 0.48, 0.45));
		testShader.setVec3("sun.direction", glm::vec3(1.0, 1.0, 1.0));
		
		testShader.setVec3("viewPos", cameraPos);
		testShader.setFloat("material.shininess", 64.0f);
		//for (unsigned int i = 0; i < 10; i++)
		//{
		//	glm::mat4 model = glm::mat4(1.0f);
		//	model = glm::translate(model, cubePositions[i]);
		//	float angle = 20.0f * i;
		//	model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0, 0.3, 0.4f));
		//	testShader.setMat4("model", model);
		//	ourModel.Draw(testShader);
		//	//glDrawArrays(GL_TRIANGLES, 0, 36);
		//}

		glm::mat4 model = glm::mat4(1.0f);
		testShader.setMat4("model", model);
		ourModel.Draw(testShader);

		model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
		testShader2.use();
		testShader2.setVec3("pointLights[0].position", lights[0]);
		testShader2.setVec3("pointLights[0].ambient", glm::vec3(0.5, 0.5, 0.5));
		testShader2.setVec3("pointLights[0].diffuse", glm::vec3(0.5, 0.5, 0.5));
		testShader2.setVec3("pointLights[0].specular", glm::vec3(0.8, 0.8, 0.8));
		testShader2.setVec3("pointLights[1].position", lights[1]);
		testShader2.setVec3("pointLights[1].ambient", glm::vec3(0.5, 0.5, 0.5));
		testShader2.setVec3("pointLights[1].diffuse", glm::vec3(0.8, 0.8, 0.8));
		testShader2.setVec3("pointLights[1].specular", glm::vec3(1.5, 1.8, 1.8));

		testShader2.setVec3("sun.ambient", glm::vec3(0.5, 0.5, 0.5));
		testShader2.setVec3("sun.diffuse", glm::vec3(0.8, 0.8, 0.8));
		testShader2.setVec3("sun.specular", glm::vec3(1.5, 1.8, 1.8));
		testShader2.setVec3("sun.direction", glm::vec3(1.0, 1.0, 1.0));

		testShader2.setVec3("viewPos", cameraPos);
		testShader2.setFloat("material.shininess", 128.0f);
		testShader2.setMat4("model", model);
		testShader2.setMat4("projection", projection);
		testShader2.setMat4("view", view);
		floor.Draw(testShader2);



		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window)
{
	float cameraSpeed = cameraSpeedMulti * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f; // change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}
