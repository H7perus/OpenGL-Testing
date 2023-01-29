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
#include "H7terrain.h"

const float cameraSpeedMulti = 80.0f;
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
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

int control_input_elev;
int control_input_ail;


void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static bool collisionCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* obj1, int id1, int index1, const btCollisionObjectWrapper* obj2, int id2, int index2);
//extern ContactAddedCallback gContactAddedCallback;




unsigned int planeVAO;



void renderQuad();
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
glm::mat4 array2mat4(const float* array) {     // OpenGL row major

	glm::mat4 matrix;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			matrix[i][j] = array[i + j];
		}
	}

	return matrix;
}
glm::mat4 btScalar2mat4(btScalar* matrix) {
	return glm::mat4(
		matrix[0], matrix[1], matrix[2], matrix[3],
		matrix[4], matrix[5], matrix[6], matrix[7],
		matrix[8], matrix[9], matrix[10], matrix[11],
		matrix[12], matrix[13], matrix[14], matrix[15]);
}

int main()
{
	
	//debugDrawer.lineShader = Shader("physicsdebug.vert", "physicsdebug.frag");
	/*float vertices[] = {
	-0.5f, -0.5f, 0.0f, 0.5, 0.0, 0.0,  0.0f, 0.0f,
	 0.5f, -0.5f, 0.0f, 0.0, 0.5, 0.0,  1.0f, 0.0f,
	 0.5f,  0.5f, 0.0f, 0.0, 0.0, 0.5,  1.0f, 1.0f,
	-0.5f,  0.5f, 0.0f, 0.5, 0.5, 0.0,  0.0f, 1.0f
	};*/

	// plane VAO
	

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
	
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, 1600, 900);

	Shader testShader("vertexShader.vert", "debugShader.frag");
	Shader testShader2("vertexShader.vert", "fragmentShader.frag");
	Shader lightShader("vertexShader.vert", "light_shader.frag");
	Shader planeShader("debugPlaneShader.vert", "debugPlaneShader.frag");
	Shader shadowShader("lightDepthShader.vert", "empty.frag");
	BulletDebugDrawer_OpenGL debugDrawer;

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

	Model cube("../assets/cylinder.obj");
	Model ourModel("../assets/Fw190A5.obj");
	Model floor("../assets/floor.obj");
	
	std::cout << ourModel.meshes.size() << std::endl;

	const unsigned int SHADOW_WIDTH = 1024 * 8, SHADOW_HEIGHT = 1024 * 8;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	unsigned int depthMap;
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

	planeShader.use();
	planeShader.setInt("depthMap", 0);

	glm::vec3 lightPos(15.0f, 15.0f, 15.0f);
	testShader2.use();
	testShader2.setVec3("sun.ambient", glm::vec3(0.1, 0.1, 0.1));
	testShader2.setVec3("sun.diffuse", glm::vec3(0.8, 0.8, 0.8));
	testShader2.setVec3("sun.specular", glm::vec3(1.5, 1.8, 1.8));
	testShader2.setVec3("sun.direction", glm::vec3(1.0, 1.0, 1.0));
	testShader2.setFloat("material.shininess", 128.0f);
	testShader2.setInt("shadowMap", 2);
	testShader.use();
	testShader.setVec3("sun.ambient", glm::vec3(0.1, 0.1, 0.1));
	testShader.setVec3("sun.diffuse", glm::vec3(0.5, 0.48, 0.45));
	testShader.setVec3("sun.specular", glm::vec3(0.5, 0.48, 0.45));
	testShader.setVec3("sun.direction", glm::vec3(1.0, 1.0, 1.0));
	testShader.setFloat("material.shininess", 64.0f);
	testShader.setInt("shadowMap", 2);

	
	//glEnable(GL_CULL_FACE);
	float sliderval = -15.0;
	glm::vec3 plane_pos = glm::vec3(0.0f, -0.31f, 0.0f);

	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	btBroadphaseInterface* overlappingPairCache = new btDbvtBroadphase();
	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
	btDiscreteDynamicsWorld* dynamicsWorld = new  btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, -10, 0));
	btAlignedObjectArray<btCollisionShape*> collisionShapes;
	btRigidBody* body1;
	btRigidBody* body2;
	btRigidBody* body3;
	btRigidBody* body4;
	btRigidBody* body5;
	btRigidBody* body6;
	const int terrain_size = 540;

	
	H7terrain<terrain_size> terrain("../assets/everest.png", 0, 35);

	//btHeightfieldTerrainShape* terr = new btHeightfieldTerrainShape(10, 10, testp, 10, 1, true, false);
	gContactAddedCallback = &collisionCallback;
	{
		//btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(100.), btScalar(50.), btScalar(100.)));
		btHeightfieldTerrainShape* terrainshape = new btHeightfieldTerrainShape(terrain_size, terrain_size, (void*)terrain.height_data, 10, 1, true, true);
		terrainshape->setLocalScaling(btVector3(1.0, 1.0, 1.0));
		//btStaticPlaneShape* terrainshape = new btStaticPlaneShape(btVector3(0, 1, 0), -2);
		//btTriangleMeshShape* trishape = new btTriangleMeshShape();
		cout << "testing testing" << endl;
		btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
		btGenerateInternalEdgeInfo(terrainshape, triangleInfoMap);
		terrainshape->setTriangleInfoMap(triangleInfoMap);

		btCollisionShape* groundShape = terrainshape;
		collisionShapes.push_back(terrainshape);

		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, -52, 0));
		groundTransform.setOrigin(btVector3(0, 3, 0));

		btScalar mass(0.);

		btVector3 localInertia(0, 0, 0);
		if (mass != 0.f)
			groundShape->calculateLocalInertia(mass, localInertia);

		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, terrainshape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		//body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
		body->setContactProcessingThreshold(0);
		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
	}
	dynamicsWorld->setDebugDrawer(&debugDrawer);

	btTransform startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btVector3(0, 4, -15));

	btQuaternion planerotate;
	planerotate.setEuler(btRadians(0), 0, 0);

	startTransform.setRotation(planerotate);
	btQuaternion quat;
	quat.setEuler(glm::radians(0.0), glm::radians(0.0), glm::radians(90.0));
	//startTransform.setRotation(quat);
	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	

	btTransform startTransform2;
	startTransform2.setIdentity();
	startTransform2.setOrigin(btVector3(0, 3, -15));
	btDefaultMotionState* myMotionState2 = new btDefaultMotionState(startTransform2);
	btDefaultMotionState* myMotionState3 = new btDefaultMotionState(startTransform2);
	startTransform2.setOrigin(btVector3(0, 0, 0));
	btDefaultMotionState* myMotionState4 = new btDefaultMotionState(startTransform2);
	btDefaultMotionState* myMotionState5 = new btDefaultMotionState(startTransform2);
	startTransform2.setOrigin(btVector3(0, 0, -5));
	btDefaultMotionState* myMotionState6 = new btDefaultMotionState(startTransform2);

	//PLANE BOX
	{
		//create a dynamic rigidbody

		btCollisionShape* colShape1 = new btBoxShape(btVector3(5.3, 0.6, 4.6));
		btCompoundShape* colShape = new btCompoundShape();
		btTransform cTransform;
		cTransform.setIdentity();
		cTransform.setOrigin(btVector3(0, 0.3, -2.6));
		cTransform.setRotation(btQuaternion(0, btRadians(5), 0));
		colShape->addChildShape(cTransform, colShape1);
		//btCollisionShape* colShape = new btSphereShape(btScalar(1.));
		collisionShapes.push_back(colShape);
		btScalar mass(3500.f);
		btVector3 localInertia(0, 0, 0);
		if (mass != 0.f)
			colShape->calculateLocalInertia(mass, localInertia);

		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		//body->setContactProcessingThreshold(0);
		dynamicsWorld->addRigidBody(body);
		body1 = body;
	}
	
	////WHEELBOX R
	glm::vec3 box_scale = glm::vec3(0.72, 0.72, 0.72);
	{
		btCollisionShape* colShape = new btBoxShape(btVector3(box_scale.x / 4, box_scale.y / 4, box_scale.z / 4));
		collisionShapes.push_back(colShape);
		btScalar mass(80.f);
		btVector3 localInertia(0, 0, 0);
		if (mass != 0.f)
			colShape->calculateLocalInertia(mass, localInertia);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState2, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		//body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
		dynamicsWorld->addRigidBody(body);
		body2 = body;
	}
	//WHEELBOX L
	{
		btCollisionShape* colShape = new btBoxShape(btVector3(box_scale.x / 4, box_scale.y / 4, box_scale.z / 4));
		//btCollisionShape* colShape = new btSphereShape(btScalar(1.));
		collisionShapes.push_back(colShape);

		btScalar mass(80.f);
		btVector3 localInertia(0, 0, 0);
		if (mass != 0)
			colShape->calculateLocalInertia(mass, localInertia);

		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState3, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		//body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
		dynamicsWorld->addRigidBody(body);
		body3 = body;
	}

	LinearSpring* basedconstraint = new LinearSpring(*body1, *body2, btVector3(-2.03033, -1.2981, 0.62), btVector3(-0.1, -0.6, 0), 120000, 5000, 12000);
	LinearSpring* basedconstraint2 = new LinearSpring(*body1, *body3, btVector3(2.03033, -1.2981, 0.62), btVector3(0.1, -0.6, 0), 120000, 5000, 12000);
	
	body1->setActivationState(DISABLE_DEACTIVATION);
	//body2->setActivationState(DISABLE_DEACTIVATION);
	//body3->setActivationState(DISABLE_DEACTIVATION);
	
	dynamicsWorld->addConstraint(basedconstraint, true);
	dynamicsWorld->addConstraint(basedconstraint2, true);

	//WHEEL R
	{
		//create a dynamic rigidbody

		btCollisionShape* colShape = new btCylinderShape(btVector3(box_scale.x / 2, box_scale.y / 5, box_scale.z / 2));
		//btCollisionShape* colShape = new btSphereShape(btScalar(1.));
		collisionShapes.push_back(colShape);

		/// Create Dynamic Objects



		btScalar mass(5.2f);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);

		


		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects

		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState4, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		dynamicsWorld->addRigidBody(body);
		body4 = body;
	}
	//WHEEL L 
	{
		//create a dynamic rigidbody

		btCollisionShape* colShape = new btCylinderShape(btVector3(box_scale.x / 2, box_scale.y / 5, box_scale.z / 2));
		//btCollisionShape* colShape = new btSphereShape(btScalar(1.));
		collisionShapes.push_back(colShape);

		/// Create Dynamic Objects



		btScalar mass(5.2f);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);




		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects

		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState5, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		dynamicsWorld->addRigidBody(body);
		body5 = body;
	}
	//TAILWHEEL
	{
		btCollisionShape* colShape = new btCylinderShape(btVector3(box_scale.x / 2, box_scale.y / 5, box_scale.z / 2));
		collisionShapes.push_back(colShape);
		btScalar mass(5.2f);
		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);
		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState6, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		body->setRestitution(0);
		dynamicsWorld->addRigidBody(body);
		body6 = body;
	}
	startTransform.setIdentity();
	startTransform2.setIdentity();
	
	startTransform2.setRotation(quat);
	btGeneric6DofSpring2Constraint* wheel1const = new btGeneric6DofSpring2Constraint(*body2, *body4, startTransform, startTransform2, RO_XYZ);

	wheel1const->setLimit(0, 0, 0);
	wheel1const->setLimit(1, 0, 0);
	wheel1const->setLimit(2, 0, 0);
	wheel1const->setLimit(3, 1, 0);
	wheel1const->setLimit(4, 0, 0);
	wheel1const->setLimit(5, 0, 0);
	dynamicsWorld->addConstraint(wheel1const, true);

	btGeneric6DofSpring2Constraint* wheel2const = new btGeneric6DofSpring2Constraint(*body3, *body5, startTransform, startTransform2, RO_XYZ);

	
	//wheel2const->setMaxMotorForce(3, 1000000);
	wheel2const->setLimit(0, 0, 0);
	wheel2const->setLimit(1, 0, 0);
	wheel2const->setLimit(2, 0, 0);
	wheel2const->setLimit(3, 1, 0);
	wheel2const->setLimit(4, 0, 0);
	wheel2const->setLimit(5, 0, 0);
	dynamicsWorld->addConstraint(wheel2const, true);
	body5->setFriction(1000000);
	body4->setFriction(1000000);

	LinearSpring* basedconstraint3value = new LinearSpring(*body1, *body6, btVector3(0, -0.5, -6), btVector3(0.0, -0.1, 0), 120000, 5000, 12000);
	btTransform tailwheeltransform;
	tailwheeltransform.setIdentity();
	tailwheeltransform.setRotation(btQuaternion(0, 0, btRadians(90)));
	btGeneric6DofSpring2Constraint* basedconstraint3 = new btGeneric6DofSpring2Constraint(*body1, *body6, basedconstraint3value->getFrameOffsetA(), tailwheeltransform);
	basedconstraint3->setLimit(0, 0, 0);
	basedconstraint3->setLimit(1, 0, 0.1);
	basedconstraint3->setLimit(2, 0, 0);
	basedconstraint3->setLimit(3, 1, 0);
	basedconstraint3->setLimit(4, 0, 0);
	basedconstraint3->setLimit(5, 0, 0);

	basedconstraint3->setStiffness(1, 60000);
	basedconstraint3->enableSpring(1, true);
	basedconstraint3->setDamping(1, 10000);
	basedconstraint3->setEquilibriumPoint(1, 0.1);
	

	dynamicsWorld->addConstraint(basedconstraint3, true);

	bool applyforce = false;
	bool applyrotate = false;
	//glfwSwapInterval(1);
	glfwSetCursorPosCallback(window, mouse_callback);
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		processInput(window);
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 10000.0f);
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);


		btTransform testTransform;
		myMotionState->getWorldTransform(testTransform);
		testTransform.getOrigin();
		btVector3 camUp(0, 1, 0);
		camUp = btCross(testTransform.getOrigin(), camUp);
		camUp = btCross(camUp, testTransform.getOrigin());
		glm::vec3 up(camUp.getX(), camUp.getY(), camUp.getZ());
		//view = glm::lookAt(cameraPos, glm::vec3(testTransform.getOrigin().getX(), testTransform.getOrigin().getY(), testTransform.getOrigin().getZ()), up);

		
		
		dynamicsWorld->stepSimulation(deltaTime, 100, 0.001);



		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Demo window");
		
		btTransform testTransform2;
		testTransform2 = dynamicsWorld->getCollisionObjectArray().at(1)->getWorldTransform();
		myMotionState->getWorldTransform(testTransform);
		btScalar testarr[16];

		testTransform.getOpenGLMatrix(testarr);

		glm::mat4 plane_modelmat = btScalar2mat4(testarr) * glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 2)); 
		

		ImGui::Checkbox("apply force", &applyforce);
		ImGui::Checkbox("apply rotate", &applyrotate);
		ImGui::End();
		ImGui::Render();

		applyFlightModel(*body1);

		btVector3 torque(25000 * -control_input_elev, 0, 25000 * control_input_ail);
		torque = body1->getWorldTransform().getBasis() * torque * body1->getLinearVelocity().length() * 0.03;
		body1->applyTorque(torque);

		if (applyforce) 
		{
			dynamicsWorld->getNonStaticRigidBodies().at(0)->activate();
			btQuaternion quate = dynamicsWorld->getNonStaticRigidBodies().at(0)->getOrientation();
			btMatrix3x3 basis = btMatrix3x3(quate);
			btVector3 forward = basis.getColumn(2);
			dynamicsWorld->getNonStaticRigidBodies().at(0)->applyCentralForce(forward * 15000);
			

			//body1->applyCentralForce(btVector3(0, -75000, 0));
			//body1->applyCentralForce(btVector3(0, 0, 20000));
			//dynamicsWorld->getNonStaticRigidBodies().at(0)->applyTorque(btVector3(8000, 0, 0));

		}
		if (applyrotate)
		{
			dynamicsWorld->getNonStaticRigidBodies().at(2)->activate();
			dynamicsWorld->getNonStaticRigidBodies().at(2)->applyTorque(btVector3(0, 0, 5000));
		}
		
		//configure shadow rendering ======================================
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		float near_plane = 13.0f, far_plane = 50.0f;
		lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, near_plane, far_plane);
		glm::vec3 plane_pos(testTransform.getOrigin().getX(), testTransform.getOrigin().getY(), testTransform.getOrigin().getZ());
		testTransform.getOrigin().getX();
		lightView = glm::lookAt(lightPos + plane_pos, plane_pos, glm::normalize(glm::vec3(-0.0, 1.0, -0.0)));
		lightSpaceMatrix = lightProjection * lightView;
		model = glm::mat4(1.0f);

		// render scene from light's point of view =========================================
		shadowShader.use();
		shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
		shadowShader.setMat4("model", model);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);
		floor.Draw(shadowShader);
		shadowShader.setMat4("model", plane_modelmat);
		ourModel.Draw(shadowShader);
		glCullFace(GL_BACK);
		//render the rest ==========================================
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		
		glm::mat4 trans = glm::mat4(1.0f);
		trans = glm::translate(trans, glm::vec3(0.0f, -0.0f, 0.0f));
		trans = glm::rotate(trans, (float)glfwGetTime(), glm::vec3(0.0, 0.0, 1.0));
		testShader.use();
		testShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		

		//glClearColor(0.6f, 0.6f, 0.8f, 1.0f);
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindVertexArray(VAO);

		testShader2.use();
		testShader2.setVec3("viewPos", cameraPos);
		testShader2.setMat4("projection", projection);
		testShader2.setMat4("view", view);
		testShader2.setMat4("model", plane_modelmat);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		ourModel.Draw(testShader2);

		myMotionState4->getWorldTransform(testTransform2);
		testTransform2.getOpenGLMatrix(testarr);
		plane_modelmat = btScalar2mat4(testarr);
		plane_modelmat = glm::scale(plane_modelmat, glm::vec3(box_scale.x, box_scale.y / 5, box_scale.z));

		testShader2.setMat4("model", plane_modelmat);

		cube.Draw(testShader2);
		myMotionState5->getWorldTransform(testTransform2);
		testTransform2.getOpenGLMatrix(testarr);
		plane_modelmat = btScalar2mat4(testarr);
		plane_modelmat = glm::scale(plane_modelmat, glm::vec3(box_scale.x, box_scale.y / 5, box_scale.z));
		testShader2.setMat4("model", plane_modelmat);
		cube.Draw(testShader2);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));

		testShader2.use();
		testShader2.setVec3("viewPos", cameraPos);
		testShader2.setMat4("view", view);
		testShader2.setMat4("projection", projection);
		testShader2.setMat4("model", model);

		model = glm::mat4(1.0f);
		testShader2.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		floor.Draw(testShader2);
		model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
		testShader2.setMat4("model", model);

		glBindBuffer(GL_ARRAY_BUFFER, terrain.tVBO);
		glBindVertexArray(terrain.tVAO);
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain.tEBO);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, pow(terrain_size - 1, 2) * 24 - 5);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glBindVertexArray(VAO);
		//debugDrawer.lineShader.use();
		//debugDrawer.SetMatrices(view, projection);
		//dynamicsWorld->debugDrawWorld();
		//debugDrawer.drawAll();
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
	control_input_elev = 0;
	control_input_ail = 0;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		control_input_elev -= 1;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		control_input_elev += 1;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		control_input_ail -= 1;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		control_input_ail += 1;

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
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

bool collisionCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* obj0, int id0, int index0, const btCollisionObjectWrapper* obj1, int id1, int index1)
{
	//cout << "when will this shit finally work man";
	btAdjustInternalEdgeContacts(cp, obj1, obj0, id1, index1);
	/*cp.m_normalWorldOnB = btVector3(0, 1, 0);
	cp.m_appliedImpulse = 0;
	cp.m_appliedImpulseLateral1 = 0;
	cp.m_appliedImpulseLateral2 = 0;*/
	return true;
}