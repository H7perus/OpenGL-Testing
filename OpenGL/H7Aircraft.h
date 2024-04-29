#pragma once
#include "Bullet3.24/btBulletDynamicsCommon.h"
#include "Bullet3.24/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "Bullet3.24/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include "Bullet3.24/BulletCollision/CollisionShapes/btTriangleShape.h"
#include "H7WheelObject.h"
#include <vector>
class H7Aircraft
{
public:
	btRigidBody* planeCore;
	std::vector<H7WheelObject*> wheels;
	void(*flightModel)(btRigidBody& plane);

	H7Aircraft(btRigidBody* planeCore);
	void runFlightModel();
	void setPosition(btTransform transform);
};

