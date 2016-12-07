#ifndef _GAMEOBJECT_H_
#define _GAMEOBJECT_H_

#include "btBulletDynamicsCommon.h"
#include "Constants.h"
#include "OpenGLMotionState.h"
#include <vector>

class GameObject
{
public:
	// Constructor + Destructor
	GameObject(btCollisionShape *pShape,
		float mass,
		const btVector3 &color,
		const btVector3 &initialPosition = btVector3(0, 0, 0),
		const btQuaternion &initialRotation = btQuaternion(0, 0, 0, 1));
	~GameObject();

	void Reposition(const btVector3 &position, const btQuaternion &orientation = btQuaternion(0, 0, 0, 1));
	float GetPosition() {
		return GetRigidBody()->getOrientation().getAngle();
	}

	// accessors
	btCollisionShape *GetShape() { return m_pShape; }
	btRigidBody *GetRigidBody() { return m_pBody; }
	btMotionState *GetMotionState() { return m_pMotionState; }
	btScalar GetMass() { return m_mass; }
	btVector3 GetInertia() { return m_inertia; }
	const btVector3 GetCOMPosition() {
		return m_pBody->getCenterOfMassPosition();
	}
	void GetTransform(btScalar *transform) {
		if (m_pMotionState) {
			m_pMotionState->GetWorldTransform(transform);
		}
	}

	float GetOrientation() {
		btScalar x, y, z;
		GetRigidBody()->getCenterOfMassTransform().getBasis().getEulerZYX(z, y, x);
		float angle = Constants::GetInstance().RadiansToDegrees(z);
		//float angle = Constants::GetInstance().RadiansToDegrees(GetRigidBody()->getOrientation().getAngleShortestPath());
		return angle;
	}

	float GetAngularVelocity() {
		/*printf("Angular Velocity = %f, %f, %f \n",
		Constants::GetInstance().RadiansToDegrees(GetRigidBody()->getAngularVelocity().x()),
		Constants::GetInstance().RadiansToDegrees(GetRigidBody()->getAngularVelocity().y()),
		Constants::GetInstance().RadiansToDegrees(GetRigidBody()->getAngularVelocity().z()));*/
		float angle = Constants::GetInstance().RadiansToDegrees(GetRigidBody()->getAngularVelocity().z());
		return angle;
	}

	void ApplyTorque(const btVector3 &torque) {
		GetRigidBody()->applyTorque(torque);
		//GetRigidBody()->applyTorqueImpulse(torque);
		//GetRigidBody()->applyTorqueImpulse(btVector3(0,0,-100));
	}

	btVector3 GetColor() { return m_color; }

	static void ClearForces(std::vector<GameObject *> objects) {
		for (std::vector<GameObject *>::iterator it = objects.begin(); it != objects.end(); ++it) {
			(*it)->GetRigidBody()->clearForces();
		}
	}

	static void ClearVelocities(std::vector<GameObject *>objects) {
		btVector3 zeroVec(0, 0, 0);
		for (std::vector<GameObject *>::iterator it = objects.begin(); it != objects.end(); ++it) {
			(*it)->GetRigidBody()->setLinearVelocity(zeroVec);
			(*it)->GetRigidBody()->setAngularVelocity(zeroVec);
		}
	}

	static void DisableObjects(std::vector<GameObject *> objects) {
		for (std::vector<GameObject *>::iterator it = objects.begin(); it != objects.end(); ++it) {
			//(*it)->GetRigidBody()->clearForces();
			// Make object static
			(*it)->GetRigidBody()->setMassProps(0.0f, btVector3(0, 0, 0));
		}
	}

	static void EnableObjects(std::vector<GameObject *> objects) {
		printf("ENABLE THE BODIES\n");
		for (std::vector<GameObject *>::iterator it = objects.begin(); it != objects.end(); ++it) {
			//(*it)->GetRigidBody()->setActivationState(DISABLE_DEACTIVATION);
			// Set mass to original mass
			(*it)->GetRigidBody()->setMassProps((*it)->GetMass(), (*it)->GetInertia());
			/*(*it)->GetRigidBody()->setLinearVelocity(btVector3(0, 0, 0));
			(*it)->GetRigidBody()->setAngularVelocity(btVector3(0, 0, 0));
			(*it)->GetRigidBody()->clearForces();*/
			(*it)->GetRigidBody()->activate();
		}
	}

	static void PrintOrientations(std::vector<GameObject *> objects) {
		for (std::vector<GameObject *>::iterator it = objects.begin(); it != objects.end(); ++it) {
			float angle = Constants::GetInstance().RadiansToDegrees((*it)->GetRigidBody()->getOrientation().getAngle());
			printf("Angle = %f, ", angle);
		}
		printf("\n");
	}

	static void DisableDeactivation(std::vector<GameObject *> objects) {
		for (std::vector<GameObject *>::iterator it = objects.begin(); it != objects.end(); ++it) {
			(*it)->GetRigidBody()->setActivationState(DISABLE_DEACTIVATION);
		}
	}

protected:
	btCollisionShape *m_pShape;
	btRigidBody *m_pBody;
	OpenGLMotionState *m_pMotionState;
	btVector3 m_color;

	btVector3 m_inertia;
	btScalar m_mass;


};
#endif


