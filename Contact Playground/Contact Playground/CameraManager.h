#ifndef _CAMERAMANAGER_H_
#define _CAMERAMANAGER_H_

#include <Windows.h>

#include "BulletDynamics\Dynamics\btDynamicsWorld.h"
#include <gl\GL.h>
#include <freeglut\freeglut.h>

// CONSTANTS
#include "Constants.h"

enum RotationType { YAW, PITCH, ROLL };
enum TranslateDirection{ UP, DOWN, LEFT, RIGHT };

class CameraManager
{
public:
	CameraManager(
		const btVector3 &target, 
		float distance, 
		float pitch, 
		float yaw, 
		const btVector3 &upVector, 
		float nearPlane, 
		float farPlane, 
		float cameraPosX = 0.0f,
		float cameraPosY = 0.0f);
	~CameraManager();

	void UpdateCamera();
	void RotateCamera(RotationType type, float value);
	void ZoomCamera(float distance);
	void TranslateCamera(TranslateDirection direction, float value);
	void PrintCameraLocation();
	btVector3 GetCameraLocation();
	void SetCameraLocationX(float X);
	void CameraManager::Reset();

	float m_nearPlane;
	float m_farPlane;

	btVector3 m_cameraPosition;
	btVector3 m_cameraTarget;

	btVector3 m_upVector;

protected:

	void SetupPerspectiveCamera();
	void SetupOrthographicCamera();
	void SetupPerspectiveModelView();
	void SetupOrthographicModelView();

	
	float m_cameraDistance;
	float m_cameraPitch;
	float m_cameraYaw;

	float m_cameraPosX;
	float m_cameraPosY;

	float m_origPosX;
	float m_origPosY;

};

#endif