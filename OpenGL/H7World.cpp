#include "H7World.h"
#include <chrono>

void H7World::DrawAll()
{
	std::cout << "amount of tiles: " << terrain_tiles.size() << std::endl;
	for (H7TerrainTile &tile : terrain_tiles)
	{
		glm::mat4 modelmat = glm::translate(glm::dmat4(1.0), glm::dvec3(tile.position_offset.x, 0, tile.position_offset.y) - *cameraPosPtr);
		TerrainShader->setMat4("model", modelmat);
		tile.Draw(0);
	}
	//do a whole bunch of shit
}

void H7World::loadTile(std::string path, int index, int tilesplit, volatile int* tile_write_indicator, float low, float high, Shader* tshader)
{
	glm::dvec2 offset = glm::dvec2((index % tilesplit - 1) * 1024 * 25, -(index / tilesplit - 1) * 1024 * 25);
	H7TerrainTile tile = H7TerrainTile(1024, path + std::to_string(index) + "_h.png", low, high, 25, offset, tshader);
	std::cout << path + std::to_string(index) + "_h.png" << std::endl;


	while (*tile_write_indicator != index);
	addTile(tile);
	std::cout << "tile_write_indicator WITHIN THREAD: " << *tile_write_indicator << std::endl;
	*tile_write_indicator = index + 1;
}

void H7World::loadAllTiles(std::string path, int tilesplit, Shader* tshader)
{
	//hardcoding the height range for now
	this->TerrainShader = tshader;
	float low = -11, high = 211;
	volatile int tile_write_indicator = 0;
	for (int i = 0; i < tilesplit * tilesplit; i++)
	{
		std::cout << "STARTING THREAD" << std::endl;
		std::thread tileloader(&H7World::loadTile, this, path, i, tilesplit, &tile_write_indicator, low, high, tshader);
		tileloader.detach();
	}


	while (tile_write_indicator != tilesplit * tilesplit);
	for (H7TerrainTile &tile : terrain_tiles)
	{
		tile.finalize_load();
	}
}

void H7World::addTile(H7TerrainTile tile)
{
		terrain_tiles.push_back(tile);
}
void H7World::set_cameraPosPtr(glm::dvec3* cameraPos)
{
	cameraPosPtr = cameraPos;
}