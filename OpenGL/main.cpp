#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtx/rotate_vector.hpp>

#include <cmath>

#include <iostream>
#include <string>
#include "shader.h"
//#include "model.h"

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
#include "H7Object.h"
#include "skyRendering.h"
#include "shadowMap.h"
#include <chrono>


float cameraSpeedMulti = 80.0f;
glm::dvec3 cameraPos = glm::vec3(-9683.33, 15.0, 6400);
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
int control_input_rud;

int global_height = 900, global_width = 1600;

void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static bool collisionCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* obj1, int id1, int index1, const btCollisionObjectWrapper* obj2, int id2, int index2);

void renderQuad();
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
glm::dmat4 array2mat4(const float* array) {     // OpenGL row major

	glm::dmat4 matrix;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			matrix[i][j] = array[i + j];
		}
	}

	return matrix;
}
glm::dmat4 btScalar2mat4(btScalar* matrix) {
	return glm::dmat4(
		matrix[0], matrix[1], matrix[2], matrix[3],
		matrix[4], matrix[5], matrix[6], matrix[7],
		matrix[8], matrix[9], matrix[10], matrix[11],
		matrix[12], matrix[13], matrix[14], matrix[15]);
}

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

	glViewport(0, 0, 1600, 900);

	Shader testShader("vertexShader.vert", "debugShader.frag");
	Shader testShader2("vertexShader.vert", "fragmentShader.frag");
	Shader lightShader("vertexShader.vert", "light_shader.frag");
	Shader planeShader("debugPlaneShader.vert", "debugPlaneShader.frag");
	Shader shadowShader("lightDepthShader.vert", "empty.frag");
	BulletDebugDrawer_OpenGL debugDrawer;

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	testShader.use();

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 projection;
	projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
	glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraDirection = glm::normalize(glm::vec3(cameraPos) - cameraTarget);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
	glm::mat4 view;

	glEnable(GL_DEPTH_TEST);

	H7Object wheelL("../assets/cylinder.obj");
	H7Object wheelR("../assets/cylinder.obj");
	H7Object airplane("../assets/Fw190A5.obj");

	vector<H7Object*> models;
	models.push_back(&wheelL);
	models.push_back(&wheelR);
	models.push_back(&airplane);



	std::cout << airplane.model->meshes.size() << std::endl;


	planeShader.use();
	planeShader.setInt("depthMap", 0);

	
	testShader2.use();
	testShader2.setVec3("sun.ambient", glm::vec3(0.15, 0.2, 0.23));
	testShader2.setVec3("sun.diffuse", glm::vec3(0.9, 0.85, 0.8));
	testShader2.setVec3("sun.specular", glm::vec3(1.8, 1.5, 1.5));
	testShader2.setVec3("sun.direction", glm::vec3(1.0, 1.0, 1.0));
	testShader2.setFloat("material.shininess", 128.0f);
	testShader2.setInt("shadowMap", 2);
	

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

	const int terrain_size = 1024;
	Shader terrainShader = Shader("vertexShader.vert", "terrainShader.frag");
	H7terrain terrain(terrain_size, "../assets/france_testmap/heightmap.png", 0, 10, 25, &terrainShader);
	terrain.load_color();
	//btHeightfieldTerrainShape* terr = new btHeightfieldTerrainShape(10, 10, testp, 10, 1, true, false);
	gContactAddedCallback = &collisionCallback;
	auto terrain_collisionshapestart = std::chrono::steady_clock::now();
	{
		//btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(100.), btScalar(50.), btScalar(100.)));
		btHeightfieldTerrainShape* terrainshape = new btHeightfieldTerrainShape(terrain_size, terrain_size, (void*)terrain.height_data, 10, 1, true, true);
		terrainshape->setLocalScaling(btVector3(25.0, 1.0, 25.0));
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
		//body->setContactProcessingThreshold(0);  //ENABLING THIS FUCKS WITH SLEEPSTATES
		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
	}
	auto terrain_collisionshapeend = std::chrono::steady_clock::now();
	std::cout << "terrain collision shape generation: " << (float)(terrain_collisionshapeend - terrain_collisionshapestart).count() / 1000000000 << std::endl;
	dynamicsWorld->setDebugDrawer(&debugDrawer);

	btTransform startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btVector3(0, 3, -15));

	btQuaternion quat;
	quat.setEuler(glm::radians(0.0), glm::radians(0.0), glm::radians(90.0));
	//startTransform.setRotation(quat);
	
	btTransform spawn1Trans;
	spawn1Trans.setIdentity();
	spawn1Trans.setOrigin(btVector3(-9120, 11.5, 5892));
	spawn1Trans.setRotation(btQuaternion(glm::radians(-60.0f), 0, 0));
	btTransform spawn2Trans;
	spawn2Trans.setIdentity();
	spawn2Trans.setOrigin(btVector3(10, 2, 10));
	btDefaultMotionState* myMotionState = new btDefaultMotionState(spawn1Trans);


	btTransform startTransform2;
	startTransform2.setIdentity();
	startTransform2.setOrigin(btVector3(-9125, 10.5, 5850));
	btDefaultMotionState* myMotionState2 = new btDefaultMotionState(spawn1Trans);
	btDefaultMotionState* myMotionState3 = new btDefaultMotionState(spawn1Trans);
	startTransform2.setOrigin(btVector3(-9119, 11.5+1, 5900));
	btDefaultMotionState* myMotionState4 = new btDefaultMotionState(spawn1Trans);
	btDefaultMotionState* myMotionState5 = new btDefaultMotionState(spawn1Trans);
	startTransform2.setOrigin(btVector3(-9120, 11.5+1, 5900 -5));
	btDefaultMotionState* myMotionState6 = new btDefaultMotionState(spawn1Trans);
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

	LinearSpring* basedconstraint = new LinearSpring(*body1, *body2, btVector3(-2.03033, -1.2981, 0.62), btVector3(-0.1, -0.6, 0), 120000, 5000, 20000);
	LinearSpring* basedconstraint2 = new LinearSpring(*body1, *body3, btVector3(2.03033, -1.2981, 0.62), btVector3(0.1, -0.6, 0), 120000, 5000, 20000);
	
	//body1->setActivationState(DISABLE_DEACTIVATION);
	//body1->setActivationState(WANTS_DEACTIVATION);
	dynamicsWorld->addConstraint(basedconstraint, true);
	dynamicsWorld->addConstraint(basedconstraint2, true);

	//WHEEL R
	{
		btCollisionShape* colShape = new btCylinderShape(btVector3(box_scale.x / 2, box_scale.y / 5, box_scale.z / 2));
		collisionShapes.push_back(colShape);
		btScalar mass(5.2f);
		bool isDynamic = (mass != 0.f);
		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState4, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		dynamicsWorld->addRigidBody(body);
		body4 = body;
	}
	//WHEEL L 
	{
		btCollisionShape* colShape = new btCylinderShape(btVector3(box_scale.x / 2, box_scale.y / 5, box_scale.z / 2));
		collisionShapes.push_back(colShape);
		btScalar mass(5.2f);
		bool isDynamic = (mass != 0.f);
		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);
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
		bool isDynamic = (mass != 0.f);
		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState6, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		body->setRestitution(0);
		dynamicsWorld->addRigidBody(body);
		body6 = body;
		body->setFriction(1000);
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
	wheel1const->setDamping(3, 1);
	wheel1const->enableMotor(3, true);
	wheel1const->setMaxMotorForce(3, 2500);
	dynamicsWorld->addConstraint(wheel1const, true);
	
	btGeneric6DofSpring2Constraint* wheel2const = new btGeneric6DofSpring2Constraint(*body3, *body5, startTransform, startTransform2, RO_XYZ);
	wheel2const->setLimit(0, 0, 0);
	wheel2const->setLimit(1, 0, 0);
	wheel2const->setLimit(2, 0, 0);
	wheel2const->setLimit(3, 1, 0);
	wheel2const->setLimit(4, 0, 0);
	wheel2const->setLimit(5, 0, 0);
	wheel2const->setDamping(3, 1);
	wheel2const->enableMotor(3, true);
	wheel2const->setMaxMotorForce(3, 2500);
	dynamicsWorld->addConstraint(wheel2const, true);
	body5->setFriction(1000);
	body4->setFriction(1000);

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
	bool freecam = true;
	
	glfwSetCursorPosCallback(window, mouse_callback);
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	shadowMap shadowMap(4096, &shadowShader);
	shadowMap.models = models;
	skyRendering skyrender;
	skyrender.updateSunDir(glm::vec3(1.0, 1.0, 1.0));

	terrain.tShader->use();
	terrain.tShader->setVec3("viewPos", glm::vec3(0));
	terrain.tShader->setVec3("sun.ambient", glm::vec3(0.15, 0.2, 0.23));
	terrain.tShader->setVec3("sun.diffuse", glm::vec3(0.9, 0.85, 0.8));
	terrain.tShader->setVec3("sun.specular", glm::vec3(1.8, 1.5, 1.5));
	terrain.tShader->setVec3("sun.specular", glm::vec3(0.0, 0.0, 0.0));
	terrain.tShader->setVec3("sun.direction", glm::vec3(1.0, 1.0, 1.0));
	terrain.tShader->setFloat("material.shininess", 1.0f);
	terrain.tShader->setInt("shadowMap", 2);
	/*terrain.tShader->setInt("texture_diffuse1", 0);
	terrain.tShader->setInt("texture_splat", 1);
	terrain.tShader->setInt("texture_diffuse2", 3);
	terrain.tShader->setInt("texture_diffuse3", 4);*/
	//terrain.tShader->setInt("decals[0].albedo", 5);
	terrain.tShader->setInt("numOfDecals", 1);
	//terrain.tShader->setInt("numOfPointLights", 100);
	for(int i = 0; i < 10; i++)
		for (int j = 0; j < 10; j++)
		{
			terrain.tShader->setVec3("pointLights[" + std::to_string(i * 10 + j) + "].position", glm::vec3(0.0 + i * 10, 0.0, 0.0 + j * 10));
			terrain.tShader->setVec3("pointLights[" + std::to_string(i * 10 + j) + "].ambient", glm::vec3(1.0f, 1.0f, 1.0f));
			terrain.tShader->setVec3("pointLights[" + std::to_string(i * 10 + j) + "].diffuse", glm::vec3(10.0f, 10.0f, 10.0f));
		}
	terrain.tShader->setVec3("pointLights[0].position", glm::vec3(0.0, 0.0, 0.0));
	terrain.tShader->setVec3("pointLights[0].ambient", glm::vec3(10.0f, 10.0f, 10.0f));
	terrain.tShader->setVec3("pointLights[0].diffuse", glm::vec3(100.0f, 100.0f, 100.0f));
	terrain.cameraPosPtr = &cameraPos;
	glfwSwapInterval(0);
	float af_rotation = 150;
	bool factivate = true;
	//body1->setCenterOfMassTransform(spawn1Trans);
	//body1->setAngularVelocity(btVector3(0, 0, 0));
	//body1->setLinearVelocity(btVector3(0, 0, 0));


	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		processInput(window);
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		//projection = glm::perspective(glm::radians(45.0f), (float)global_height / global_width, 0.1f, 100.0f);
		projection = glm::perspective(glm::radians(45.0f), (float)global_width / global_height, 0.1f, 25000.0f);
		
		btTransform testTransform;
		myMotionState->getWorldTransform(testTransform);
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Demo window");
		ImGui::Checkbox("apply force", &applyforce);
		ImGui::SliderFloat("movementspeed", &cameraSpeedMulti, 0.0f, 1000.0);
		ImGui::SliderFloat("airfield rotation", &af_rotation, 0.0f, 360.0);
		ImGui::Checkbox("freecam", &freecam);
		ImGui::Checkbox("activate", &factivate);
		bool spawn1 = ImGui::Button("Spawn 1");
		bool spawn2 = ImGui::Button("Spawn 2");
		

		if (spawn1)
		{
			body1->setCenterOfMassTransform(spawn1Trans);

			body1->setAngularVelocity(btVector3(0, 0, 0));
			body1->setLinearVelocity(btVector3(0, 0, 0));

		}
		if (spawn2)
		{
			body1->setCenterOfMassTransform(spawn2Trans);
			body1->setAngularVelocity(btVector3(0, 0, 0));
			body1->setLinearVelocity(btVector3(0, 0, 0));
		}
		

		ImGui::Text("speed: %.1f%s", body1->getLinearVelocity().length() * 3.6, "kph");
		ImGui::End();
		ImGui::Render();
		if (factivate)
		{
			body1->activate();
		}
		dynamicsWorld->stepSimulation(deltaTime, 100, 0.001);

		
		
		btTransform testTransform2;
		testTransform2 = dynamicsWorld->getCollisionObjectArray().at(1)->getWorldTransform();
		myMotionState->getWorldTransform(testTransform);
		btScalar testarr[16];

		//testTransform.setOrigin(btVector3(0, 0, 0));
		testTransform.getOpenGLMatrix(testarr);

		glm::dmat4 plane_modelmat = btScalar2mat4(testarr) * glm::translate(glm::dmat4(1.0), glm::dvec3(0, 0, 2));

		
		
		view = glm::lookAt(glm::vec3(0.0), cameraFront, cameraUp);
		if (!freecam)
		{
			glm::dvec4 plane_pos;
			plane_pos = plane_modelmat * glm::dvec4(1.8, 0.2, -7.5, 1.0);
			glm::vec3 plane_front(glm::mat3(plane_modelmat) * glm::vec3(0.0, 0.0, 1.0));
			glm::vec3 plane_up(glm::mat3(plane_modelmat) * glm::vec3(0.0, 1.0, 0.0));
			cameraPos = plane_pos;
			view = glm::lookAt(glm::vec3(0), plane_front, plane_up);
		}

		plane_modelmat = glm::translate(glm::dmat4(1.0), -cameraPos) * plane_modelmat;
		//body1->activate();
		applyFlightModel(*body1);
		
		btVector3 torque(25000 * -control_input_elev, 15000 * -control_input_rud, 25000 * control_input_ail);
		torque = body1->getWorldTransform().getBasis() * torque * body1->getLinearVelocity().length() * 0.03;
		body1->applyTorque(torque);

		if (applyforce) 
		{
			body1->activate();
			btVector3 forward = body1->getWorldTransform().getBasis().getColumn(2);
			dynamicsWorld->getNonStaticRigidBodies().at(0)->applyCentralForce(forward * 30000);
		}
		
		//configure shadow rendering ======================================
		model = glm::mat4(1.0f);
		

		
		//glm::mat4 done_plane_modelmat = glm::mat4(plane_modelmat);

		testShader2.use();
		testShader2.setVec3("viewPos", glm::vec3(0.0));
		testShader2.setMat4("projection", projection);
		testShader2.setMat4("view", view);
		airplane.setTransform(plane_modelmat);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, shadowMap.depthMap);
		

		
		myMotionState4->getWorldTransform(testTransform2);
		testTransform2.getOpenGLMatrix(testarr);
		plane_modelmat = btScalar2mat4(testarr);
		plane_modelmat = glm::scale(plane_modelmat, glm::dvec3(box_scale.x, box_scale.y / 5, box_scale.z));
		wheelR.setTransform(glm::translate(glm::dmat4(1.0), -cameraPos) * plane_modelmat);
		
		myMotionState5->getWorldTransform(testTransform2);
		testTransform2.getOpenGLMatrix(testarr);
		plane_modelmat = btScalar2mat4(testarr);
		plane_modelmat = glm::scale(plane_modelmat, glm::dvec3(box_scale.x, box_scale.y / 5, box_scale.z));
		wheelL.setTransform(glm::translate(glm::dmat4(1.0), -cameraPos) * plane_modelmat);

		glm::mat4 lightSpaceMatrix = shadowMap.draw();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		glViewport(0, 0, global_width, global_height);
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		skyrender.shader.use();
		skyrender.shader.setMat4("view", view);
		skyrender.shader.setMat4("projection",	projection);
		skyrender.Draw();
		testShader2.use();
		airplane.Draw(testShader2);
		wheelR.Draw(testShader2);
		wheelL.Draw(testShader2);

		terrain.tShader->use();
		terrain.tShader->setVec3("viewPos", glm::vec3(0.0));
		terrain.tShader->setMat4("view", view);
		terrain.tShader->setMat4("projection", projection);

		terrain.tShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

		glm::vec3 airfield_pos(-9683.33, 50.0, 6400);

		float af_scale = 5;

		float airfield_hwidth = 176.8 * af_scale, airfield_hheight = 81.3 * af_scale;
		

		glm::mat4 decal_mat = glm::ortho(-airfield_hwidth, airfield_hwidth, -airfield_hheight, airfield_hheight, 0.0f, 200.0f) * glm::lookAt(airfield_pos - glm::vec3(cameraPos), airfield_pos - glm::vec3(cameraPos) - glm::vec3(0, 1, 0), glm::rotate(glm::vec3(0, 0, 1), glm::radians(af_rotation), glm::vec3(0, -1, 0)));;
		terrain.tShader->setMat4("decals[0].decalTransform", decal_mat);
		glActiveTexture(GL_TEXTURE10);
		terrain.tShader->setInt("shadowMap", 10);
		glBindTexture(GL_TEXTURE_2D, shadowMap.depthMap);
		//terrain.tShader->setInt("main_albedo", 0);
		//terrain.tShader->setInt("subtex_land", 1);
		terrain.draw();
		glBindVertexArray(0);
		/*debugDrawer.lineShader.use();
		debugDrawer.SetMatrices(glm::translate(view, glm::vec3(-cameraPos)), projection);
		glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
		dynamicsWorld->debugDrawWorld();
		debugDrawer.drawAll();*/
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
	control_input_rud = 0;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		control_input_elev -= 1;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		control_input_elev += 1;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		control_input_ail -= 1;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		control_input_ail += 1;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		control_input_rud -= 1;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		control_input_rud += 1;

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

	global_height = height, global_width = width;
	std::cout << "behold, it has been resized :O" << std::endl;
}
bool collisionCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* obj0, int id0, int index0, const btCollisionObjectWrapper* obj1, int id1, int index1)
{
	//cout << "when will this shit finally work man";
	btAdjustInternalEdgeContacts(cp, obj1, obj0, id1, index1);
	return true;
}