#pragma once
#include "Bullet3.24/btBulletDynamicsCommon.h"
class H7WheelObject
{
public:
	btRigidBody* firstHelper;
	btGeneric6DofSpring2Constraint* swivelConstraint;
	btGeneric6DofSpring2Constraint* wheelspinConstraint;
	btRigidBody* secondHelper;
	btRigidBody* wheel;
	bool rotating;
	float rotation_limit;
	bool locked;
	btDefaultMotionState* myMotionState;
	btDefaultMotionState* myMotionState2;
	btDefaultMotionState* myMotionStateWheel;

	H7WheelObject(float wheel_radius, float wheel_thickness, float rotation_offset = 0, float rotation_angle = 0, bool rotating = false, bool lockable = false)
	{
		btVector3 localInertia(0, 0, 0);
		btVector3 localInertia2(0, 0, 0);
		btVector3 localInertiaWheel(0, 0, 0);
		btTransform defaultTransform;
		defaultTransform.setIdentity();
		defaultTransform.setOrigin(btVector3(-9125, 10.5, 5850));
		myMotionState = new btDefaultMotionState(defaultTransform);
		myMotionState2 = new btDefaultMotionState(defaultTransform);
		myMotionStateWheel = new btDefaultMotionState(defaultTransform);
		
		btCollisionShape* wheelShape = new btCylinderShape(btVector3(wheel_radius, wheel_thickness * 2, wheel_radius));



		this->rotating = rotating;
		this->rotation_limit = rotation_angle;
		btCollisionShape* colShape = new btBoxShape(btVector3(0.1, 0.1, 0.1)); //this could technically be any size, but I feel better if its small
		btScalar mass(20.f);
		btScalar mass2(20.f);
		btScalar massWheel(5.2f);

		colShape->calculateLocalInertia(mass, localInertia);
		colShape->calculateLocalInertia(mass2, localInertia2);
		wheelShape->calculateLocalInertia(massWheel, localInertiaWheel);
		
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody::btRigidBodyConstructionInfo rbInfo2(mass2, myMotionState2, colShape, localInertia2);
		btRigidBody::btRigidBodyConstructionInfo wheelInfo(massWheel, myMotionStateWheel, wheelShape, localInertiaWheel);
		wheel = new btRigidBody(wheelInfo);
		wheel->setRestitution(0);
		wheel->setFriction(999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999.9); //Had to crank this value because Bullets friction model doesn't feature static friction. Gotta write that some day. I want to have a custom friction model at some point.
		firstHelper = new btRigidBody(rbInfo);
		firstHelper->setCollisionFlags(firstHelper->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

		btTransform wheelTransform;
		wheelTransform.setIdentity();
		wheelTransform.setOrigin(btVector3(0, 0, 0));
		wheelTransform.setRotation(btQuaternion(0, 0, btRadians(90)));
		if (rotating)
		{
			secondHelper = new btRigidBody(rbInfo2);
			btTransform firstHelperTransform;
			firstHelperTransform.setIdentity();
			firstHelperTransform.setOrigin(btVector3(0, 0, 0));
			btTransform secondHelperTransform;
			secondHelperTransform.setIdentity();
			secondHelperTransform.setOrigin(btVector3(0, 0, rotation_offset));


			swivelConstraint = new btGeneric6DofSpring2Constraint(*firstHelper, *secondHelper, firstHelperTransform, secondHelperTransform, RO_XYZ);
			swivelConstraint->setLinearLowerLimit(btVector3(0, 0, 0));
			swivelConstraint->setLinearUpperLimit(btVector3(0, 0, 0));
			swivelConstraint->setAngularLowerLimit(btVector3(0, -btRadians(rotation_angle), 0));
			swivelConstraint->setAngularUpperLimit(btVector3(0, btRadians(rotation_angle), 0));

			wheelspinConstraint = new btGeneric6DofSpring2Constraint(*secondHelper, *wheel, firstHelperTransform, wheelTransform, RO_XYZ);
			wheelspinConstraint->setLinearLowerLimit(btVector3(0, 0, 0));
			wheelspinConstraint->setLinearUpperLimit(btVector3(0, 0, 0));
			wheelspinConstraint->setAngularLowerLimit(btVector3(1, 0, 0));
			wheelspinConstraint->setAngularUpperLimit(btVector3(0, 0, 0));
			secondHelper->setCollisionFlags(secondHelper->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
		}
		else
		{
			btTransform firstHelperTransform;
			firstHelperTransform.setIdentity();
			firstHelperTransform.setOrigin(btVector3(0, 0, 0));
			
			wheelspinConstraint = new btGeneric6DofSpring2Constraint(*firstHelper, *wheel, firstHelperTransform, wheelTransform, RO_XYZ);
			wheelspinConstraint->setLinearLowerLimit(btVector3(0, 0, 0));
			wheelspinConstraint->setLinearUpperLimit(btVector3(0, 0, 0));
			wheelspinConstraint->setAngularLowerLimit(btVector3(1, 0, 0));
			wheelspinConstraint->setAngularUpperLimit(btVector3(0, 0, 0));
			//wheelspinConstraint->enableMotor(3, true);
			//wheelspinConstraint->setMaxMotorForce(3, 100);
		}
	}
	void addToWorld(btDynamicsWorld* targetWorld)
	{
		if (rotating)
		{
			targetWorld->addRigidBody(secondHelper);
			targetWorld->addConstraint(swivelConstraint);
		}
		targetWorld->addConstraint(wheelspinConstraint);
		targetWorld->addRigidBody(firstHelper);
		targetWorld->addRigidBody(wheel);
	}
	void updateWheelState()
	{
		if (locked)
		{
			swivelConstraint->setAngularLowerLimit(btVector3(0, 0, 0));
			swivelConstraint->setAngularUpperLimit(btVector3(0, 0, 0));
		}
		else
		{
			swivelConstraint->setAngularLowerLimit(btVector3(0, -btRadians(rotation_limit), 0));
			swivelConstraint->setAngularUpperLimit(btVector3(0, btRadians(rotation_limit), 0));
		}
	}
};

