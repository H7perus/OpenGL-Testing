#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

#include <cmath>

#include <iostream>
#include <string>
#include "shader.h"

#include "stb_image.h"
#include "physicsObject.h"
#include "imgui-1.89.1/imgui.h"
#include "imgui-1.89.1/backends/imgui_impl_opengl3.h"
#include "imgui-1.89.1/backends/imgui_impl_glfw.h"

#include "Bullet3.24/btBulletDynamicsCommon.h"
#include "Bullet3.24/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "Bullet3.24/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include "Bullet3.24/BulletCollision/CollisionShapes/btTriangleShape.h"
#include "debugdraw.h"

#include "LinearSpring.h"
#include "flightModel.h"
const float cameraSpeedMulti = 8.5f;
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
bool prevTabbed = false;
bool guiEnabled = true;

void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static bool collisionCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* obj1, int id1, int index1, const btCollisionObjectWrapper* obj2, int id2, int index2);

int main()
{
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
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 10000.0f);
	glm::mat4 view;
	glEnable(GL_DEPTH_TEST);

	BulletDebugDrawer_OpenGL debugDrawer;
	btDefaultCollisionConfiguration*		collisionConfiguration	= new	btDefaultCollisionConfiguration();
	btCollisionDispatcher*					dispatcher				= new	btCollisionDispatcher(collisionConfiguration);
	btBroadphaseInterface*					overlappingPairCache	= new	btDbvtBroadphase();
	btSequentialImpulseConstraintSolver*	solver					= new	btSequentialImpulseConstraintSolver;
	btDiscreteDynamicsWorld*				dynamicsWorld			= new	btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, -5, 0));
	btRigidBody* body1;
	btRigidBody* body4;
	btRigidBody* body5;
	dynamicsWorld->setDebugDrawer(&debugDrawer);


	double testheight[20 * 30];
	for (int i = 0; i < 20 * 30; i++)
	{
		testheight[i] = float(i % 20) / 20.0f;
		
	}

	gContactAddedCallback = collisionCallback;
	{
		//btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(100.), btScalar(50.), btScalar(100.)));
		
		btHeightfieldTerrainShape* terrainshape = new btHeightfieldTerrainShape(20, 30, &testheight, btScalar(10), 1, true, true);
		terrainshape->setUseDiamondSubdivision(true);
		btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
		btGenerateInternalEdgeInfo(terrainshape, triangleInfoMap);
		terrainshape->setTriangleInfoMap(triangleInfoMap);

		btCollisionShape* groundShape = terrainshape;
		groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), -6);
		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, 3, -10));

		

		btScalar mass(0.);
		btVector3 localInertia(0, 0, 0);
		if (mass != 0.f)
			groundShape->calculateLocalInertia(mass, localInertia);
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, terrainshape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
		std::cout << "threshold: " << body->getContactProcessingThreshold() << std::endl;
		body->setContactProcessingThreshold(0);
		dynamicsWorld->addRigidBody(body);
	}
	
	btTransform startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btVector3(0, 5, -15));
	btTransform startTransform2;
	startTransform2.setIdentity();
	startTransform2.setOrigin(btVector3(0, 0, 0));

	startTransform.setRotation(btQuaternion(1, 1, 1)); //COMMENT THIS OUT FOR NEUTRAL ROTATION OF THE BOX
	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	btDefaultMotionState* myMotionState4 = new btDefaultMotionState(startTransform2);
	btDefaultMotionState* myMotionState5 = new btDefaultMotionState(startTransform2);
	{
		btCollisionShape* colShape1 = new btBoxShape(btVector3(5.3, 0.6, 4.6));
		btCompoundShape* colShape = new btCompoundShape();
		btTransform cTransform;
		cTransform.setIdentity();
		cTransform.setOrigin(btVector3(0, 0, -2.6));
		colShape->addChildShape(cTransform, colShape1);
		btScalar mass(3500.f);
		btVector3 localInertia(0, 0, 0);
		if (mass != 0.f)
			colShape->calculateLocalInertia(mass, localInertia);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		body->setContactProcessingThreshold(0);
		dynamicsWorld->addRigidBody(body);
		body1 = body;
	}
	body1->setActivationState(DISABLE_DEACTIVATION);
	////UNCOMMENT THIS FOR THE WHEELS
	//glm::vec3 box_scale = glm::vec3(0.72, 0.72, 0.72);
	//
	////WHEEL R
	//{
	//	btCollisionShape* colShape = new btCylinderShape(btVector3(box_scale.x / 2, box_scale.y / 5, box_scale.z / 2));
	//	btScalar mass(5.2f);
	//	bool isDynamic = (mass != 0.f);
	//	btVector3 localInertia(0, 0, 0);
	//	if (isDynamic)
	//		colShape->calculateLocalInertia(mass, localInertia);
	//	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState4, colShape, localInertia);
	//	btRigidBody* body = new btRigidBody(rbInfo);
	//	dynamicsWorld->addRigidBody(body);
	//	body4 = body;
	//}
	////WHEEL L 
	//{
	//	btCollisionShape* colShape = new btCylinderShape(btVector3(box_scale.x / 2, box_scale.y / 5, box_scale.z / 2));
	//	btScalar mass(5.2f);
	//	bool isDynamic = (mass != 0.f);
	//	btVector3 localInertia(0, 0, 0);
	//	if (isDynamic)
	//		colShape->calculateLocalInertia(mass, localInertia);
	//	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState5, colShape, localInertia);
	//	btRigidBody* body = new btRigidBody(rbInfo);
	//	dynamicsWorld->addRigidBody(body);
	//	body5 = body;
	//}
	//LinearSpring* basedconstraint = new LinearSpring(*body1, *body4, btVector3(-2.03033, -1.2981, 0.62), btVector3(-0.0, -0.65, 0), 120000, 5000, 12000);
	//LinearSpring* basedconstraint2 = new LinearSpring(*body1, *body5, btVector3(2.03033, -1.2981, 0.62), btVector3(0.0, -0.65, 0), 120000, 5000, 12000);
	//dynamicsWorld->addConstraint(basedconstraint);
	//dynamicsWorld->addConstraint(basedconstraint2);

	glfwSetCursorPosCallback(window, mouse_callback);
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	ImGui::StyleColorsDark();
	bool applyforce = false;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, 1600, 900);

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glfwPollEvents();
		processInput(window);
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		dynamicsWorld->stepSimulation(deltaTime, 100, 0.001);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Demo window");
		ImGui::Checkbox("apply force", &applyforce);
		ImGui::End();
		ImGui::Render();
		if (applyforce) 
		{
			dynamicsWorld->getNonStaticRigidBodies().at(0)->activate();
			body1->applyCentralForce(btVector3(0, 0, 20000));
		}
		debugDrawer.lineShader.use();
		debugDrawer.SetMatrices(view, projection);
		dynamicsWorld->debugDrawWorld();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}
	glfwTerminate();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
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
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) 
	{
		if (!prevTabbed)
			if (guiEnabled)
			{
				//glfwSetCursorPosCallback(window, mouse_callback);
				//ImGui_ImplGlfw_CursorPosCallback(window, mouse_callback);
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				guiEnabled = false;
			}
			else
			{
				//glfwSetCursorPosCallback(window, NULL);
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				
				guiEnabled = true;
			}
		prevTabbed = true;
	}
	else
		prevTabbed = false;
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
	if (!guiEnabled) {
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
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}
bool collisionCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* obj0, int id0, int index0, const btCollisionObjectWrapper* obj1, int id1, int index1)
{
	btAdjustInternalEdgeContacts(cp, obj1, obj0, id1, index1);
	/*cp.m_normalWorldOnB = btVector3(0, 1, 0);
	cp.m_appliedImpulse = 0;
	cp.m_appliedImpulseLateral1 = 0;
	cp.m_appliedImpulseLateral2 = 0;*/
	return true;
}