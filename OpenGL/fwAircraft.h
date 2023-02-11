#pragma once
#include "H7Object.h"
#include "Bullet3.24/btBulletDynamicsCommon.h"


class fwAircraft : H7Object
{
	int engine_type; // 0 = piston, 1 = jet, 2 = rocket
	
	
	btRigidBody* physics_object;
};

