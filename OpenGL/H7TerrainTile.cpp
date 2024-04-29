#include "H7TerrainTile.h"

H7TerrainTile::H7TerrainTile(const int map_size_in, std::string terrain_heightmap, float min_height, float max_height, const float u_per_px_constructor, glm::dvec2 pos_offset, Shader* in_Shader)
{
	tShader = in_Shader;
	map_size = map_size_in;
	width = map_size, length = map_size;
	position_offset = pos_offset;
	height_data = new double[map_size * map_size];
	u_per_px = u_per_px_constructor;
	height_image_data = stbi_load_16(terrain_heightmap.c_str(), &width, &length, &channels, 1);
	

	glGenBuffers(1, &tVBO);
	glBindBuffer(GL_ARRAY_BUFFER, tVBO);
	glBufferData(GL_ARRAY_BUFFER, pow(map_size - 1, 2) * sizeof(GLfloat) * 6 * 8, NULL, GL_STATIC_DRAW); //why should this be pow(mapsize - 1, 2) ???

	glGenBuffers(1, &tEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, pow(map_size - 1, 2) * 6 * sizeof(GLuint), NULL, GL_STATIC_DRAW);

	glGenBuffers(1, &tEBOLOD1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBOLOD1);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBOLOD1);

	int indices[6] = { 0, width - 1, width * width - 1, width * width - 1, width * width - width, 0 };
	std::cout << "GL VARIABLES. VBO: " << tVBO << " EBO: " << tEBO << std::endl;
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

	glGenVertexArrays(1, &tVAO);
	glBindVertexArray(tVAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));


	GLfloat data[8];
	GLuint index[3];
	float height_range = max_height - min_height;
	auto time_start = std::chrono::steady_clock::now();
	auto time_heightstart = std::chrono::steady_clock::now();
	//for (int i = 0; i < pow(map_size, 2); i++)
	//{
	//	height_data[i] = float(*(height_image_data + i)) / 65535 * height_range;

	//	if (i < pow(map_size, 2) - map_size && (i + 1) % map_size != 0)
	//	{
	//		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBO);
	//		index[0] = i;
	//		index[1] = i + map_size + 1;
	//		index[2] = i + 1;
	//		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, EBO_size, sizeof(index), index);
	//		index[2] = i + map_size;
	//		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, EBO_size + 3 * sizeof(GLuint), sizeof(index), index);
	//		EBO_size += 2 * sizeof(index);
	//	}
	//	//glGetBufferSubData(GL_ARRAY_BUFFER, index[1] * sizeof(data), sizeof(data), data);
	//	//std::cout << data[0] << " " << data[1] << " " << data[2] << std::endl;
	//}
	for (int y = 0; y < map_size; y++)
		for (int x = 0; x < map_size; x++)
		{
			height_data[y * map_size + x] = float(height_image_data[y * map_size + x]) / 65535 * height_range;

			if (x < map_size - 1 && y < map_size - 1)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBO);
				index[0] = y * map_size + x;
				index[1] = y * map_size + x + map_size + 1;
				index[2] = y * map_size + x + 1;

				//TESTCODE FOR THREADED LOADING
				tEBOcontent.push_back(index[0]);
				tEBOcontent.push_back(index[1]);
				tEBOcontent.push_back(index[2]);


				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, EBO_size, sizeof(index), index);
				index[2] = y * map_size + x + map_size;
				//SAME HERE
				tEBOcontent.push_back(index[0]);
				tEBOcontent.push_back(index[1]);
				tEBOcontent.push_back(index[2]);

				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, EBO_size + 3 * sizeof(GLuint), sizeof(index), index);
				EBO_size += 2 * sizeof(index);
			}
		}


	auto time_height = std::chrono::steady_clock::now() - time_heightstart;
	std::cout << "total time for map height generation: " << (float)time_height.count() / 1000000000 << std::endl;
	auto terrain_collisionshapestart = std::chrono::steady_clock::now();
	{
		//btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(100.), btScalar(50.), btScalar(100.)));
		//btHeightfieldTerrainShape* terrainshape = new btHeightfieldTerrainShape(map_size, map_size, (void*)height_data, 30, 1, true, true);
		btHeightfieldTerrainShape* terrainshape = new btHeightfieldTerrainShape(map_size, map_size, (double*)height_data, btScalar(min_height), btScalar(max_height), 1, true);

		terrainshape->setLocalScaling(btVector3(u_per_px_constructor, 1.0, u_per_px_constructor));
		//btStaticPlaneShape* terrainshape = new btStaticPlaneShape(btVector3(0, 1, 0), -2);
		//btTriangleMeshShape* trishape = new btTriangleMeshShape();
		std::cout << "testing testing" << std::endl;
		btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
		btGenerateInternalEdgeInfo(terrainshape, triangleInfoMap); //Not sure if this and the next line will be required in the future
		terrainshape->setTriangleInfoMap(triangleInfoMap);

		//collisionShapes.push_back(terrainshape);

		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(position_offset.x, (max_height+min_height)/2, position_offset.y));
		btScalar mass(0.);

		btVector3 localInertia(0, 0, 0);


		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, terrainshape, localInertia);
		physics_body = new btRigidBody(rbInfo);
		//body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		physics_body->setCollisionFlags(physics_body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
		//body->setContactProcessingThreshold(0);  //ENABLING THIS FUCKS WITH SLEEPSTATES
		//add the body to the dynamics world
		//dynamicsWorld->addRigidBody(body);
	}
	auto terrain_collisionshapeend = std::chrono::steady_clock::now();
	std::cout << "terrain collision shape generation: " << (float)(terrain_collisionshapeend - terrain_collisionshapestart).count() / 1000000000 << std::endl;
	//std::cout << pow(map_size, 2) - map_size << std::endl;
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBO);
	glGetBufferSubData(GL_ARRAY_BUFFER, 8 * 4, sizeof(data), data);
	//std::cout << data[3] << " " << data[4] << " " << data[5] << std::endl;

	auto time_generationstart = std::chrono::steady_clock::now();


	for (int i = 0; i < pow(map_size, 2); i++)
	{
		get_vertdata(data, i, min_height, max_height);
		get_vertnormal(data, i);

		glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(data), sizeof(data), data);

		//TEXTCODE FOR THREADED LOADING
		for (int j = 0; j < 8; j++)
		{
			tVBOcontent.push_back(data[j]);
		}
	}

	auto time_gen = std::chrono::steady_clock::now() - time_heightstart;
	std::cout << "total time for map mesh generation: " << (float)time_gen.count() / 1000000000 << std::endl;
	std::cout << "total time: " << (float)(time_gen + time_height).count() / 1000000000 << std::endl;
	GLfloat face_data[24];
}
void H7TerrainTile::finalize_load()
{
	glGenBuffers(1, &tVBO);
	glBindBuffer(GL_ARRAY_BUFFER, tVBO);
	glBufferData(GL_ARRAY_BUFFER, pow(map_size, 2) * sizeof(GLfloat) * 8, tVBOcontent.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &tEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, pow(map_size - 1, 2) * 6 * sizeof(GLuint), tEBOcontent.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &tEBOLOD1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBOLOD1);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), tEBOLOD1content.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBOLOD1);

	int indices[6] = { 0, width - 1, width * width - 1, width * width - 1, width * width - width, 0 };
	std::cout << "GL CONTENT. VBO: " << tVBOcontent[0] << " EBO: " << tEBOcontent[100] << std::endl;
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

	glGenVertexArrays(1, &tVAO);
	glBindVertexArray(tVAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));



}


void H7TerrainTile::load_color(std::string terrain_albedomap)
{
	tShader->use(); //I am an idiot, forgot this and was chasing texture assign bugs
	add_texture((std::string("../assets/france_testmap/") + terrain_albedomap).c_str(), "main_albedo");

	add_texture("../assets/france_testmap/forestground_albedo.png", "subtex_land");
	add_texture("../assets/france_testmap/water1.png", "subtex_water");

	glGenTextures(1, &splat_map);
	glBindTexture(GL_TEXTURE_2D, splat_map);
	if (height_image_data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, width, length, 0, GL_RED, GL_UNSIGNED_SHORT, height_image_data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture (splat)" << std::endl;
	}
	stbi_image_free(color_data2);
	tShader->setInt("splatMap", textures.size());
	textures.push_back(splat_map);
	stbi_image_free(height_image_data);

	add_decal("../assets/france_testmap/airfield_decal2.png");

	tShader->setInt("subtex_land", 1);
}