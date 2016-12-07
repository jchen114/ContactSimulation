#ifndef _OPENGLMOTIONSTATE_H_
#define _OPENGLMOTIONSTATE_H_

#include "btBulletCollisionCommon.h"

class OpenGLMotionState : public btDefaultMotionState {
public:
	OpenGLMotionState(const btTransform &transform) : btDefaultMotionState(transform) {} // Constructor inherits from btDefaultMotionState Creates a motion state
	
	void GetWorldTransform(btScalar * transform) {
		btTransform trans;
		getWorldTransform(trans); // Gets the world transform from bullet
		trans.getOpenGLMatrix(transform); // Convenience method to convert the bullet world transform to OpenGL's world for rendering
	}
};

#endif