#include "H7Aircraft.h"
#include "H7WheelObject.h"
#include "flightModel.h"

H7Aircraft::H7Aircraft(btRigidBody* planeCore)
{
	this->planeCore = planeCore;
	this->flightModel = applyFlightModel;
}

void H7Aircraft::runFlightModel()
{
	flightModel(*planeCore);
}

void H7Aircraft::setPosition(btTransform transform)
{
	btTransform totalTransform = transform * planeCore->getCenterOfMassTransform().inverse();
	planeCore->setWorldTransform(transform);
	planeCore->setAngularVelocity(btVector3(0, 0, 0));
	planeCore->setLinearVelocity(btVector3(0, 0, 0));

	for (H7WheelObject *wheel : wheels)
	{
		wheel->firstHelper->setCenterOfMassTransform(totalTransform * wheel->firstHelper->getCenterOfMassTransform());
		wheel->firstHelper->setAngularVelocity(btVector3(0, 0, 0));
		wheel->firstHelper->setLinearVelocity(btVector3(0, 0, 0));
		wheel->wheel->setCenterOfMassTransform(totalTransform * wheel->wheel->getCenterOfMassTransform());
		wheel->wheel->setAngularVelocity(btVector3(0, 0, 0));
		wheel->wheel->setLinearVelocity(btVector3(0, 0, 0));
		if (wheel->rotating == true)
		{
			wheel->secondHelper->setCenterOfMassTransform(totalTransform * wheel->secondHelper->getCenterOfMassTransform());
			wheel->secondHelper->setAngularVelocity(btVector3(0, 0, 0));
			wheel->secondHelper->setLinearVelocity(btVector3(0, 0, 0));
		}
	}
}