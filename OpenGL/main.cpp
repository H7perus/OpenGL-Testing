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
#include "model.h"
#include "H7Asset.h"

#include "stb_image.h"
#include "physicsObject.h"
#include "imgui-1.89.1/imgui.h"
#include "imgui-1.89.1/backends/imgui_impl_opengl3.h"
#include "imgui-1.89.1/backends/imgui_impl_glfw.h"

#include "Bullet3.24/btBulletDynamicsCommon.h"
#include "Bullet3.24/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "Bullet3.24/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include "Bullet3.24/BulletCollision/CollisionShapes/btTriangleShape.h"
#include "H7WheelObject.h"
#include "debugdraw.h"

#include "LinearSpring.h"
//#include "flightModel.h"
#include "H7Aircraft.h"
#include "H7TerrainTile.h"
#include "H7World.h"
#include "H7Object.h"
#include "H7Assetmanager.h"
#include "H7cloudnoiseutil.h"
#include "skyRendering.h"
#include "shadowMap.h"
#include <chrono>

H7AssetManager AssetManager;

float cameraSpeedMulti = 80.0f;
glm::dvec3 cameraPos = glm::vec3(-9683.33, 15.0, 6400);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float near = 0.1; float far = 50000;


bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

int iFrame = 0;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool prevTabbed = false;
bool guiEnabled = true;

int control_input_elev;
int control_input_ail;
int control_input_rud;

int global_height = 900, global_width = 1600;

//glm::vec3 sunDirection = glm::vec3(1.0, 1.0, 1.0);	
	//glm::vec3 sunDiffuse = glm::vec3(0.9, 0.85, 0.8);
glm::vec3 sunDirection = glm::normalize(glm::vec3(1.0, 1.0, 1.0));
glm::vec3 sunDiffuse = glm::vec3(1.0, 0.9, 0.8);
glm::vec3 sunAmbient = glm::vec3(0.04, 0.05, 0.08);

void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static bool collisionCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* obj1, int id1, int index1, const btCollisionObjectWrapper* obj2, int id2, int index2);

void renderQuad(Shader &shader);
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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
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
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	glViewport(0, 0, 1600, 900);

	Shader testShader("vertexShader.vert", "debugShader.frag");
	Shader testShader2("vertexShader.vert", "fragmentShader.frag");
	/*int maxLength = 0;
	glGetProgramiv(testShader2.ID, GL_INFO_LOG_LENGTH, &maxLength);

	std::vector<GLchar> errorLog(maxLength);
	glGetProgramInfoLog(testShader2.ID, maxLength, &maxLength, &errorLog[0]);

	for (int i = 0; i < maxLength; i++)
	{
		std::cout << errorLog[i];
	}
	std::cout << std::endl;*/
	Shader lightShader("vertexShader.vert", "light_shader.frag");
	Shader debugQuadShader("debugPlaneShader.vert", "debugPlaneShader.frag");
	Shader redrawShader("redrawVShader.vert", "redrawShader.frag");
	Shader cloudShader("redrawVShader.vert", "cloudShader.frag");

	Shader shadowShader("lightDepthShader.vert", "empty.frag");
	Shader treeShader("vertexShader.vert", "AlphaCullShader.frag");
	BulletDebugDrawer_OpenGL debugDrawer;

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);

	AssetManager.add_asset(testShader2, "wheel", { "../assets/cylinder.obj" });
	H7Object wheelL(AssetManager, "wheel");
	H7Object wheelR(AssetManager, "wheel");
	H7Object airplane(AssetManager, testShader2, "3", { "../assets/Fw190A5.obj" }, true);
	H7Object tree(AssetManager, treeShader, "4", {"../assets/france_testmap/tree/pine.obj"}, true);
	//H7Object treeLOD1(AssetManager, treeShader, "5", { "../assets/france_testmap/tree/pineLOD1.obj" }, true);

	H7Object pine(AssetManager, testShader2, "6", { "../assets/france_testmap/tree/pine.obj", "../assets/france_testmap/tree/pineLOD1.obj"}, true);

	wheelL.setModifier(glm::scale(glm::mat4(1.0), glm::vec3(0.72, 0.72 * .2, 0.72)));
	wheelR.setModifier(glm::scale(glm::mat4(1.0), glm::vec3(0.72, 0.72 * .2, 0.72)));

	vector<H7Object*> models; //right now only used for shadows.
	models.push_back(&wheelL);
	models.push_back(&wheelR);
	models.push_back(&airplane); 

	debugQuadShader.use();
	debugQuadShader.setInt("depthMap", 0);

	testShader2.use();
	testShader2.setVec3("sun.ambient", sunAmbient);
	testShader2.setVec3("sun.diffuse", sunDiffuse);
	testShader2.setVec3("sun.specular", sunDiffuse * 2.f);
	testShader2.setVec3("sun.direction", sunDirection);
	testShader2.setFloat("material.shininess", 128.0f);
	testShader2.setInt("shadowMap", 2);

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

	Shader terrainShader = Shader("vertexTerrainShader.vert", "terrainShader.frag");
	H7TerrainTile terrain(1024, "../assets/channelmap/tile_7_h.png", -11, 211, 25, glm::dvec2(0, 0), &terrainShader);
	terrain.load_color("../assets/france_testmap/colormap.png");
	H7World world;
	world.loadAllTiles("../assets/channelmap/tile_", 3, &terrainShader);

	world.cameraPosPtr = &cameraPos;
	//world.addTile(terrain);
	gContactAddedCallback = &collisionCallback;
	//btStaticPlaneShape* terrainshape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);
	//btTransform PlaneTransform;
	//PlaneTransform.setIdentity();
	//PlaneTransform.setOrigin(btVector3(-9683.33, 8, 6400));
	//btDefaultMotionState* PlaneState = new btDefaultMotionState(PlaneTransform);
	//btRigidBody::btRigidBodyConstructionInfo rbInfo(0, PlaneState, terrainshape, btVector3(0, 0, 0));
	//btRigidBody* groundPlane = new btRigidBody(rbInfo);
	////dynamicsWorld->addRigidBody(groundPlane);
	dynamicsWorld->addRigidBody(terrain.physics_body);

	dynamicsWorld->setDebugDrawer(&debugDrawer);

	btTransform startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btVector3(0, 3, -15));
	btTransform spawn1Trans;
	spawn1Trans.setIdentity();
	spawn1Trans.setOrigin(btVector3(-9120, 23.5, 5892));
	spawn1Trans.setRotation(btQuaternion(glm::radians(-60.0f), 0, 0));
	btTransform spawn2Trans;
	spawn2Trans.setIdentity();
	spawn2Trans.setOrigin(btVector3(10, 2, 10));

	btDefaultMotionState* myMotionState = new btDefaultMotionState(spawn1Trans);
	btDefaultMotionState* myMotionState4 = new btDefaultMotionState(spawn1Trans);
	btDefaultMotionState* myMotionState5 = new btDefaultMotionState(spawn1Trans);
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

	glm::vec3 box_scale = glm::vec3(0.72, 0.72, 0.72);
	H7WheelObject WheelR(box_scale.y / 2, box_scale.y / 15);
	WheelR.wheelspinConstraint->enableMotor(3, true);
	WheelR.wheelspinConstraint->setMaxMotorForce(3, 2500);
	H7WheelObject WheelL(box_scale.y / 2, box_scale.y / 15);
	WheelL.wheelspinConstraint->enableMotor(3, true);
	WheelL.wheelspinConstraint->setMaxMotorForce(3, 2500);
	H7WheelObject WheelTail(box_scale.y / 3, box_scale.y / 15, 0.2, 70.0, true);
	btTransform tailW;
	tailW.setIdentity();
	tailW.setOrigin(btVector3(-9120, 11.5 + 1, 5900 - 5));
	WheelTail.firstHelper->setWorldTransform(tailW);
	WheelTail.wheel->setWorldTransform(tailW);

	WheelR.addToWorld(dynamicsWorld);
	WheelL.addToWorld(dynamicsWorld);
	WheelTail.addToWorld(dynamicsWorld);

	LinearSpring* basedconstraint = new LinearSpring(*body1, *WheelR.firstHelper, btVector3(-2.03033, -1.2981, 0.62), btVector3(-0.1, -0.6, 0), 100000, 5000, 15000);
	LinearSpring* basedconstraint2 = new LinearSpring(*body1, *WheelL.firstHelper, btVector3(2.03033, -1.2981, 0.62), btVector3(0.1, -0.6, 0), 100000, 5000, 15000);
	myMotionState4 = WheelR.myMotionStateWheel;
	myMotionState5 = WheelL.myMotionStateWheel;
	//body1->setActivationState(DISABLE_DEACTIVATION);
	//body1->setActivationState(WANTS_DEACTIVATION);
	dynamicsWorld->addConstraint(basedconstraint, true);
	dynamicsWorld->addConstraint(basedconstraint2, true);

	btTransform tailwheeltransform;
	btTransform planetailwheeltransform;
	tailwheeltransform.setIdentity();
	tailwheeltransform.setOrigin(btVector3(0,0,0));
	planetailwheeltransform.setIdentity();
	planetailwheeltransform.setOrigin(btVector3(0, -0.6, -6));
	btGeneric6DofSpring2Constraint* basedconstraint3 = new btGeneric6DofSpring2Constraint(*body1, *WheelTail.firstHelper, planetailwheeltransform, tailwheeltransform);
	WheelTail.wheel->setRestitution(0);
	WheelTail.wheel->setFriction(1000);
	basedconstraint3->setStiffness(1, 60000);
	basedconstraint3->enableSpring(1, true);
	basedconstraint3->setDamping(1, 10000);
	basedconstraint3->setEquilibriumPoint(1, 0.1);
	basedconstraint3->setAngularLowerLimit(btVector3(0, 0, 0));
	basedconstraint3->setAngularUpperLimit(btVector3(0, 0, 0));
	
	dynamicsWorld->addConstraint(basedconstraint3, true);

	H7Aircraft Fw190(body1);
	Fw190.wheels.push_back(&WheelL);
	Fw190.wheels.push_back(&WheelR);
	Fw190.wheels.push_back(&WheelTail);

	bool applyforce = false;
	bool applyrotate = false;
	bool freecam = true;
	
	shadowMap shadowMap(4096, &shadowShader);
	shadowMap.models = models;
	skyRendering skyrender;
	skyrender.updateSunDir(sunDirection);
	
	skyrender.updateSunColor(sunDiffuse);
	terrainShader.use();
	terrainShader.setVec3("viewPos", glm::vec3(0));
	terrainShader.setVec3("sun.ambient", sunAmbient);
	terrainShader.setVec3("sun.diffuse", sunDiffuse);
	terrainShader.setVec3("sun.specular", glm::vec3(1.8, 1.5, 1.5));
	terrainShader.setVec3("sun.specular", glm::vec3(0.0, 0.0, 0.0));
	terrainShader.setVec3("sun.direction", sunDirection);
	terrainShader.setFloat("material.shininess", 1.0f);
	terrainShader.setInt("shadowMap", 2);
	terrainShader.setInt("numOfDecals", 1);

	glfwSwapInterval(0);
	float af_rotation = 150;
	bool factivate = true;

	glm::vec3 airfield_pos(-9683.33, 50.0, 6400);
	terrain.generate_tree_locations("../assets/france_testmap/treemap.png");

	//glEnable(GL_MULTISAMPLE);
	bool paused = false;
	glm::mat4 view, model, projection;


	
	
	unsigned int frameBuffer;
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	//glEnable(GL_DEPTH_TEST);

	unsigned int tex_colorBuffer;
	glGenTextures(1, &tex_colorBuffer);
	glBindTexture(GL_TEXTURE_2D, tex_colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, global_width, global_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_colorBuffer, 0);

	
	unsigned int frameRenderBuffer;
	glGenRenderbuffers(1, &frameRenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, frameRenderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, global_width, global_height); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameRenderBuffer); // now actually attach it
	
	unsigned int tex_depthBuffer;
	glGenTextures(1, &tex_depthBuffer);
	glBindTexture(GL_TEXTURE_2D, tex_depthBuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, global_width, global_height, 0, GL_DEPTH_COMPONENT32F, GL_FLOAT, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depthBuffer, 0);

	unsigned int cloudBuffer;
	glGenFramebuffers(1, &cloudBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, cloudBuffer);

	unsigned int tex_cloudBuffer;
	glGenTextures(1, &tex_cloudBuffer);
	glBindTexture(GL_TEXTURE_2D, tex_cloudBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, global_width, global_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_cloudBuffer, 0);
	

	
	int detailWorleySize = 48;
	GLfloat* detailWorleyNoiseData = new GLfloat[detailWorleySize * detailWorleySize * detailWorleySize * 3]();
	three_ch_worley_from_to_file("detailWorleyNoise.bin", detailWorleyNoiseData, 4, detailWorleySize, 1.0, 0, 0);

	for (int i = 0; i < detailWorleySize; i++)
	{
		std::cout << detailWorleyNoiseData[i] << std::endl;
	}

	unsigned int detailWorleyNoiseTexture;
	glGenTextures(1, &detailWorleyNoiseTexture);
	glBindTexture(GL_TEXTURE_3D, detailWorleyNoiseTexture);


	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F, detailWorleySize, detailWorleySize, detailWorleySize, 0, GL_RGB, GL_FLOAT, detailWorleyNoiseData);

	int perlin_worley_tex_size = 64;
	GLfloat* perlinWorleyNoiseData = new GLfloat[perlin_worley_tex_size * perlin_worley_tex_size * perlin_worley_tex_size * 4]();
	perlinworley_from_to_file("perlinWorleyNoise.bin", perlinWorleyNoiseData, perlin_worley_tex_size);

	unsigned int perlinWorleyNoiseTexture;
	glGenTextures(1, &perlinWorleyNoiseTexture);
	glBindTexture(GL_TEXTURE_3D, perlinWorleyNoiseTexture);


	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F, perlin_worley_tex_size, perlin_worley_tex_size, perlin_worley_tex_size, 0, GL_RED, GL_FLOAT, perlinWorleyNoiseData);
	

	float cloudCover = 0.5;
	float scatterFactor = 0.85;
	float densityMultiplier = 0.3;
	float scatterMultiplier = 1.0;
	float absorptionMultiplier = 0.1;

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glEnable(GL_MULTISAMPLE);
	while (!glfwWindowShouldClose(window))
	{



		glBindTexture(GL_TEXTURE_2D, tex_cloudBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, global_width, global_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, tex_colorBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, global_width, global_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, tex_depthBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, global_width, global_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);




		glfwPollEvents();
		processInput(window);
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		projection = glm::perspective(glm::radians(45.0f), (float)global_width / global_height, near, far);
		
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
		ImGui::Checkbox("pause", &paused);
		ImGui::Checkbox("lock tailwheel", &WheelTail.locked);
		bool spawn1 = ImGui::Button("Spawn 1");
		bool unload = ImGui::Button("unload");
		if (ImGui::Button("reload Shader"))
		{
			testShader2 = Shader("vertexShader.vert", "fragmentShader.frag");
			testShader2.use();
			testShader2.setVec3("sun.ambient", sunAmbient);
			testShader2.setVec3("sun.diffuse", sunDiffuse);
			testShader2.setVec3("sun.specular", sunDiffuse * 2.f);
			testShader2.setVec3("sun.direction", sunDirection);
			testShader2.setFloat("material.shininess", 128.0f);
			testShader2.setInt("shadowMap", 2);

			redrawShader = Shader("redrawVShader.vert", "redrawShader.frag");
			cloudShader = Shader("redrawVShader.vert", "cloudShader.frag");
			terrainShader = Shader("vertexTerrainShader.vert", "terrainShader.frag");
			skyrender = skyRendering();
		}
		



		WheelTail.updateWheelState();
		if (spawn1)
		{
			Fw190.setPosition(spawn1Trans);
		}
		if (unload)
		{
			AssetManager.Assets[airplane.asset_id]->Unload(0);
		}

		ImGui::Text("speed: %.1f%s", body1->getLinearVelocity().length() * 3.6, "kph");
		glm::vec3 dirvector = glm::normalize(cameraPos - glm::dvec3(airfield_pos));
		dirvector = glm::normalize(glm::vec3(1, 0, 1));
		ImGui::Text("rotation: %.5f%s", atan2(dirvector.x, dirvector.z), "rad");
		ImGui::BeginTable("Position: ", 3);
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("x: %.1f", cameraPos.x);
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("y: %.1f", cameraPos.y);
		ImGui::TableSetColumnIndex(2);
		ImGui::Text("z: %.1f", cameraPos.z);
		ImGui::EndTable();


		ImGui::End();
		ImGui::Render();
		if (factivate)
		{
			body1->activate();
		}
		if (!paused)
		{
			dynamicsWorld->stepSimulation(deltaTime, 100, 0.001);
		}

		btTransform testTransform2;
		testTransform2 = dynamicsWorld->getCollisionObjectArray().at(1)->getWorldTransform();
		myMotionState->getWorldTransform(testTransform);
		glm::dmat4 plane_modelmat = glm::dmat4(1.0);
		testTransform.getOpenGLMatrix((btScalar*)&plane_modelmat);
		plane_modelmat = glm::translate(plane_modelmat, glm::dvec3(0, 0, 2));

		view = glm::lookAt(glm::vec3(0.0), cameraFront, cameraUp);
		if (!freecam)
		{
			glm::dvec4 plane_pos;
			plane_pos = plane_modelmat * glm::dvec4(1.8, 0.2, -11.5, 1.0);
			glm::vec3 plane_front(glm::mat3(plane_modelmat) * glm::vec3(0.0, 0.0, 1.0));
			glm::vec3 plane_up(glm::mat3(plane_modelmat) * glm::vec3(0.0, 1.0, 0.0));
			cameraPos = plane_pos;
			view = glm::lookAt(glm::vec3(0), plane_front, plane_up);

			cloudShader.use();
			cloudShader.setMat3("rotMat", glm::mat3(glm::normalize(glm::cross(plane_up, plane_front)), plane_up, plane_front));


		}

		plane_modelmat = glm::translate(glm::dmat4(1.0), -cameraPos) * plane_modelmat;
		//body1->activate();
		Fw190.runFlightModel();
		
		btVector3 torque(25000 * -control_input_elev, 15000 * -control_input_rud, 25000 * control_input_ail);
		torque = body1->getWorldTransform().getBasis() * torque * (body1->getLinearVelocity().length() * 0.03 + .1);
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
		treeShader.use();
		treeShader.setVec3("viewPos", glm::vec3(0.0));
		treeShader.setMat4("projection", projection);
		treeShader.setMat4("view", view);
		testShader2.use();
		testShader2.setVec3("viewPos", glm::vec3(0.0));
		testShader2.setMat4("projection", projection);
		testShader2.setMat4("view", view);
		airplane.setTransform(plane_modelmat);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, shadowMap.depthMap);
		
		myMotionState4->getWorldTransform(testTransform2);
		wheelR.setTransform(testTransform2, cameraPos);
		myMotionState5->getWorldTransform(testTransform2);
		wheelL.setTransform(testTransform2, cameraPos);
		model = glm::translate(glm::mat4(1.0f), glm::vec3(-9683.33, 8.0, 6400) - glm::vec3(cameraPos));
		tree.setTransform(model);

		glm::mat4 lightSpaceMatrix = shadowMap.draw();
		testShader2.use();
		testShader2.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		//glEnable(GL_MULTISAMPLE);
		glViewport(0, 0, global_width, global_height);
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		skyrender.shader.use();
		skyrender.shader.setMat4("view", view);
		skyrender.shader.setMat4("projection",	projection);
		skyrender.shader.setVec3("cameraPos", glm::vec3(cameraPos));
		skyrender.Draw();
		testShader2.use();
		airplane.draw(0);
		wheelR.draw(0);
		wheelL.draw(0);

		auto treeStart = std::chrono::steady_clock::now();
		for (glm::dvec3 treeloc : terrain.treelocations)
		{
			if (glm::distance(cameraPos, treeloc) > 300)
			{
				glm::dvec4 posrot(treeloc, -1.0);
				pine.drawActionPosRot(posrot, 1);
			}
			else
			{
				glm::dvec4 posrot(treeloc, 0.0);
				pine.drawActionPosRot(posrot, 0);
			}
		}
		testShader.setDVec3("cameraPos", cameraPos);
		AssetManager.drawInstancedPositions();
		auto treeTime = std::chrono::steady_clock::now() - treeStart;

		terrainShader.use();
		terrainShader.setVec3("viewPos", glm::vec3(0.0));
		terrainShader.setMat4("view", view);
		terrainShader.setMat4("projection", projection);
		terrainShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		
		float af_scale = 5;
		float airfield_hwidth = 176.8 * af_scale, airfield_hheight = 81.3 * af_scale;
		
		glm::mat4 decal_mat = glm::ortho(-airfield_hwidth, airfield_hwidth, -airfield_hheight, airfield_hheight, 0.0f, 200.0f) * glm::lookAt(airfield_pos - glm::vec3(cameraPos), airfield_pos - glm::vec3(cameraPos) - glm::vec3(0, 1, 0), glm::rotate(glm::vec3(0, 0, 1), glm::radians(af_rotation), glm::vec3(0, -1, 0)));;
		terrainShader.setMat4("decals[0].decalTransform", decal_mat);
		glActiveTexture(GL_TEXTURE10);
		terrainShader.setInt("shadowMap", 10);
		glBindTexture(GL_TEXTURE_2D, shadowMap.depthMap);

		glm::mat4 modelmat = glm::translate(glm::dmat4(1.0), glm::dvec3(0, 0, 0) - cameraPos);
		terrainShader.setMat4("model", modelmat);
		world.DrawAll();


		glBindFramebuffer(GL_FRAMEBUFFER, cloudBuffer);
		//glViewport(0, 0, global_width, global_height);
		//glClearColor(0.5f, 0.4f, 0.3f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT);
		cloudShader.use();
		cloudShader.setInt("iFrame", iFrame);
		cloudShader.setMat4("viewMat", view);

		if(freecam)
			cloudShader.setMat3("rotMat", glm::mat3(glm::normalize(glm::cross(cameraUp, cameraFront)), glm::cross(cameraFront, glm::normalize(glm::cross(cameraUp, cameraFront))), cameraFront));

		glm::mat3 rotMat = glm::mat3(glm::normalize(glm::cross(cameraUp, cameraFront)), cameraUp, cameraFront);
		std::cout << (rotMat * glm::vec3(0, 0, 1)).x << " " << (rotMat * glm::vec3(0, 0, 1)).y << " " << (rotMat * glm::vec3(0, 0, 1)).z << std::endl;
		std::cout << cameraUp.x << " " << cameraUp.y << " " << cameraUp.z << std::endl;
		cloudShader.setVec3("cameraPos", cameraPos);


		cloudShader.setVec2("screensize", glm::vec2(global_width, global_height));

		cloudShader.setFloat("fov", glm::degrees(atan(tan(glm::radians(fov / 2)) / global_height * global_width) * 2));
		cloudShader.setFloat("u_cloudDensityMultiplier", 0.3f);
		cloudShader.setFloat("u_lightAbsorptionMultiplier", 0.1f);
		cloudShader.setFloat("u_lightScatterMultiplier", 1.f);
		cloudShader.setFloat("u_cloudCover", 0.5);
		cloudShader.setFloat("u_scatterFactor", 0.750f);
		cloudShader.setVec3("u_sunDir", sunDirection);
		cloudShader.setVec3("u_sunDiff", sunDiffuse);
		cloudShader.setVec3("u_sunAmbi", sunAmbient);
		cloudShader.setInt("detailWorleyTexture", 0);
		cloudShader.setInt("perlinWorleyTexture", 1);
		cloudShader.setInt("redraw_texture1_d", 2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, detailWorleyNoiseTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_3D, perlinWorleyNoiseTexture);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, tex_depthBuffer);

		renderQuad(cloudShader);


		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glViewport(0, 0, global_width, global_height);
		glClearColor(0.5f, 0.4f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		redrawShader.use();
		redrawShader.setInt("redraw_texture1", 0);
		redrawShader.setInt("redraw_texture2", 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_colorBuffer);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex_cloudBuffer);
		renderQuad(redrawShader);
		
		
		
		glBindVertexArray(0);
		debugDrawer.lineShader.use();
		debugDrawer.SetMatrices(glm::translate(view, glm::vec3(-cameraPos)), projection);
		glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
		//dynamicsWorld->debugDrawWorld();
		//debugDrawer.drawAll();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);

		iFrame++;
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
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				guiEnabled = false;
			}
			else
			{
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
void renderQuad(Shader &shader)
{
	float vertices[] = {
	-1.0f, -1.0f, 0.5f, 0.5, 0.0, 0.0,  0.0f, 0.0f,
	 1.0f, -1.0f, 0.5f, 0.0, 0.5, 0.0,  1.0f, 0.0f,
	 1.0f,  1.0f, 0.5f, 0.0, 0.0, 0.5,  1.0f, 1.0f,
	-1.0f,  1.0f, 0.5f, 0.5, 0.5, 0.0,  0.0f, 1.0f
	};
	unsigned int indices[] = {
	0, 1, 2,
	0, 2, 3
	};

	unsigned int VBO, VAO, EBO;
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	shader.use();
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}