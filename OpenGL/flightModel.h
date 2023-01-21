#pragma once
#include "Bullet3.24/btBulletDynamicsCommon.h"
#include <iostream>
#include <iomanip>

void applyFlightModel(btRigidBody& plane)
{
	btMatrix3x3 rotmat = plane.getWorldTransform().getBasis().transpose();
	btTransform localDir = plane.getWorldTransform();
	localDir(btVector3(1, 0, 0));
	btVector3 cgVel = rotmat * plane.getVelocityInLocalPoint(btVector3(0, 0, 0));
	btVector3 tailVel = rotmat * plane.getVelocityInLocalPoint(btVector3(0, 0, -5.6));

	
	btVector3 up(0, 1, 0);
	btVector3 right(1, 0, 0);
	btScalar main_force = btAngle(up, cgVel);
	main_force -= 0.5 * acos(-1);
	main_force *= 4000 * cgVel.length();
	btScalar elev_force = btAngle(up, tailVel);
	elev_force -= 0.5 * acos(-1);

	//std::cout << std::setw(8) << (rotmat.inverse() * btVector3(0, 1, 0)).getX() << std::setw(8) << (rotmat.inverse() * btVector3(0, 1, 0)).getY() << std::setw(8) << (rotmat.inverse() * btVector3(0, 1, 0)).getZ() << std::endl;
	//std::cout << (btVector3(0, 1, 0) * main_force).getX() << std::endl;
	elev_force *= 100 * tailVel.length();
	
	btScalar rudder_force = btAngle(right, tailVel);
	rudder_force -= 0.5 * acos(-1);
	//std::cout << std::setw(8) << std::setprecision(2) << std::fixed << std::left;
	//std::cout << rudder_force * 180 / acos(-1) << std::setw(8) << tailVel.getX() << std::setw(8) << tailVel.getY() << std::setw(8) << tailVel.getZ() << std::endl;

	rudder_force *= 500 * tailVel.length();

	plane.applyForce(rotmat.inverse() * (btVector3(0, 1, 0) * main_force), rotmat.inverse() * btVector3(0, 0, 0));
	plane.applyForce(rotmat.inverse() * (btVector3(0, 1, 0) * elev_force), rotmat.inverse() * btVector3(0, 0, -5.6));
	plane.applyForce(rotmat.inverse() * (btVector3(1, 0, 0) * rudder_force), rotmat.inverse() * btVector3(0, 0, -5.6));
}