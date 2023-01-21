#pragma once
#include "Bullet3.24/btBulletDynamicsCommon.h"

class LinearSpring : public btGeneric6DofSpring2Constraint
{
public:
	LinearSpring(btRigidBody& rbA, btRigidBody& rbB, btVector3 relLoc, btVector3 direction);
	LinearSpring(btRigidBody& rbA, btRigidBody& rbB, btVector3 relLoc, btVector3 direction, btScalar stiffness, btScalar damping, btScalar precompression);
};

