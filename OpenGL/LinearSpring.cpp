#include "LinearSpring.h"
#include <iostream>


LinearSpring::LinearSpring(btRigidBody& rbA, btRigidBody& rbB, btVector3 relLoc, btVector3 direction)
	: btGeneric6DofSpring2Constraint(rbA, rbB, btTransform(), btTransform())
{
	btScalar angleRoll = atan2(direction.getX(), direction.getY());
	btScalar anglePitch = atan2(direction.getY(), direction.getZ());
	btQuaternion connectionOrientation;

	//connectionOrientation.setEuler(0, anglePitch, angleRoll);
	connectionOrientation.setEuler(0, 0, angleRoll);
	//connectionOrientation.setEuler(0, btRadians(0), btRadians(0));


	btTransform frameInA;
	frameInA.setIdentity();
	frameInA.setOrigin(relLoc);
	frameInA.setRotation(connectionOrientation);
	btTransform frameInB;
	frameInB.setIdentity();
	frameInB.setRotation(connectionOrientation);


	setFrames(frameInA, frameInB);
	setLinearUpperLimit(btVector3(0, direction.length(), 0));
	setLinearLowerLimit(btVector3(0, 0, 0));
	setEquilibriumPoint(1, direction.length());
	setAngularUpperLimit(btVector3(0, 0, 0));
	setAngularLowerLimit(btVector3(0, 0, 0));
}

LinearSpring::LinearSpring(btRigidBody& rbA, btRigidBody& rbB, btVector3 relLoc, btVector3 direction, btScalar stiffness, btScalar damping, btScalar precompression)
	: btGeneric6DofSpring2Constraint(rbA, rbB, btTransform(), btTransform())
{
	btScalar angleRoll = atan2(direction.getX(), direction.getY());
	btScalar anglePitch = atan2(direction.getY(), direction.getZ());
	btQuaternion connectionOrientation;

	//connectionOrientation.setEuler(0, anglePitch, angleRoll);
	connectionOrientation.setEuler(0, 0, angleRoll);
	//connectionOrientation.setEuler(0, btRadians(0), btRadians(0));

	btTransform frameInA;
	frameInA.setIdentity();
	frameInA.setOrigin(relLoc);
	frameInA.setRotation(connectionOrientation);
	btTransform frameInB;
	frameInB.setIdentity();
	connectionOrientation.setEuler(0, 0, btRadians(90));
	frameInB.setRotation(connectionOrientation);


	setFrames(frameInA, frameInB);
	setLinearUpperLimit(btVector3(0, direction.length(), 0));
	setLinearLowerLimit(btVector3(0, 0, 0));
	
	setAngularUpperLimit(btVector3(0, 0, 0));
	setAngularLowerLimit(btVector3(1, 0, 0));

	setDamping(1, damping);
	setStiffness(1, stiffness);
	setEquilibriumPoint(1, direction.length() + precompression / stiffness);
	enableSpring(1, true);
}