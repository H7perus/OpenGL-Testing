#pragma once
#include "H7TerrainTile.h"
#include "H7Object.h"
#include "H7AssetManager.h"
#include "H7Asset.h"



class H7World
{
public:
	//terrain
	//light sources(sun, pointlights, etc.)
	//object registry
	Shader* TerrainShader;
	glm::dvec3* cameraPosPtr;
	std::vector<H7TerrainTile> terrain_tiles;
	H7World() {};


	void DrawAll(); //From this point onward, let the World class decide what to draw.
	void loadTile(std::string path, int index, int tilesplit, volatile int* tile_write_indicator, float low, float high, Shader* tshader);
	void loadAllTiles(string path, int tilesplit, Shader *tShader);
	void addTile(H7TerrainTile tile);
	void set_cameraPosPtr(glm::dvec3* cameraPos);
};

