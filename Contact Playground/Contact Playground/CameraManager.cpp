#include "stdafx.h"
#include "CameraManager.h"

// Some constants for 3D math and the camera speed
#define RADIANS_PER_DEGREE 0.01745329f


CameraManager::CameraManager(
	const btVector3 &target,
	float distance,
	float pitch,
	float yaw,
	const btVector3 &upVector,
	float nearPlane,
	float farPlane,
	float cameraPosX,
	float cameraPosY)
{
	m_cameraTarget = target;
	m_cameraDistance = distance;
	m_cameraPitch = pitch;
	m_cameraYaw = yaw;
	m_upVector = upVector;
	m_nearPlane = nearPlane;
	m_farPlane = farPlane;

	m_origPosX = cameraPosX;
	m_origPosY = cameraPosY;

	m_cameraPosX = m_origPosX;
	m_cameraPosY = m_origPosY;

	SetupOrthographicCamera();

}

void CameraManager::UpdateCamera() {

	// exit in erroneous situations
	if (Constants::GetInstance().GetScreenWidth() == 0 && Constants::GetInstance().GetScreenHeight() == 0)
		return;

	switch (Constants::GetInstance().GetProjectionMode())
	{
	case ORTHOGRAPHIC:
		//SetupModelView();
		//SetupOrthographicModelView();
		SetupOrthographicCamera();
		break;
	case PERSPECTIVE:
		SetupPerspectiveCamera();
		break;
	default:
		break;
	}

}

void CameraManager::SetupOrthographicCamera() {
	//printf("Setting up orthographic camera\n");

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// boundaries of the camera for projection
	glOrtho(
		-Constants::GetInstance().GetScreenWidth() / 2,
		Constants::GetInstance().GetScreenWidth() / 2,
		-Constants::GetInstance().GetScreenHeight() / 2,
		Constants::GetInstance().GetScreenHeight() / 2,
		-100,
		100
		);
	SetupOrthographicModelView();
}

void CameraManager::SetupOrthographicModelView() {

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// Scale first
	float m2p = Constants::GetInstance().GetMetersToPixels(m_cameraDistance);
	glScalef(m2p, m2p, 1);
	// Translate in meters.
	glTranslatef(m_cameraPosX, m_cameraPosY, 0);

}

void CameraManager::Reset() {
	m_cameraPosX = m_origPosX;
	m_cameraPosY = m_origPosY;
}

void CameraManager::SetupPerspectiveCamera() {

	// select the projection matrix
	glMatrixMode(GL_PROJECTION);
	// set it to the matrix-equivalent of 1
	glLoadIdentity();
	// determine the aspect ratio of the screen
	float aspectRatio = Constants::GetInstance().GetScreenWidth() / (float)Constants::GetInstance().GetScreenHeight();
	// create a viewing frustum based on the aspect ratio and the
	// boundaries of the camera
	glFrustum(-aspectRatio * m_nearPlane, aspectRatio * m_nearPlane, -m_nearPlane, m_nearPlane, m_nearPlane, m_farPlane);
	// the projection matrix is now set
	SetupPerspectiveModelView();
}

void CameraManager::SetupPerspectiveModelView() {

	// select the view matrix
	glMatrixMode(GL_MODELVIEW);
	// set it to '1'
	glLoadIdentity();

	// our values represent the angles in degrees, but 3D 
	// math typically demands angular values are in radians.
	float pitch = m_cameraPitch * RADIANS_PER_DEGREE;
	float yaw = m_cameraYaw * RADIANS_PER_DEGREE;

	// create a quaternion defining the angular rotation 
	// around the up vector
	btQuaternion rotation(m_upVector, yaw);

	// set the camera's position to 0,0,0, then move the 'z' 
	// position to the current value of m_cameraDistance.
	btVector3 cameraPosition(0, 0, 0);
	//cameraPosition[2] = -m_cameraDistance;
	cameraPosition[2] = m_cameraDistance;

	// Translation
	m_cameraTarget[0] = m_cameraPosX;
	m_cameraTarget[1] = m_cameraPosY;

	// create a Bullet Vector3 to represent the camera 
	// position and scale it up if its value is too small.
	btVector3 forward(cameraPosition[0], cameraPosition[1], cameraPosition[2]);
	if (forward.length2() < SIMD_EPSILON) {
		forward.setValue(1.f, 0.f, 0.f);
	}

	// figure out the 'right' vector by using the cross 
	// product on the 'forward' and 'up' vectors
	btVector3 right = m_upVector.cross(forward);

	// create a quaternion that represents the camera's roll
	btQuaternion roll(right, -pitch);

	// turn the rotation (around the Y-axis) and roll (around 
	// the forward axis) into transformation matrices and 
	// apply them to the camera position. This gives us the 
	// final position
	cameraPosition = btMatrix3x3(rotation) * btMatrix3x3(roll) * cameraPosition;

	// save our new position in the member variable, and 
	// shift it relative to the target position (so that we 
	// orbit it)
	m_cameraPosition[0] = cameraPosition.getX();
	m_cameraPosition[1] = cameraPosition.getY();
	m_cameraPosition[2] = cameraPosition.getZ();
	m_cameraPosition += m_cameraTarget;

	// create a view matrix based on the camera's position and where it's
	// looking
	//printf("Camera Position = %f, %f, %f\n", cameraPosition[0], cameraPosition[1], cameraPosition[2]);
	// the view matrix is now set
	gluLookAt(m_cameraPosition[0], m_cameraPosition[1], m_cameraPosition[2], m_cameraTarget[0], m_cameraTarget[1], m_cameraTarget[2], m_upVector.getX(), m_upVector.getY(), m_upVector.getZ());

}

/* CAMERA MOVEMENT METHODS */

void CameraManager::RotateCamera(RotationType type, float value) {
	// change the value (it is passed by reference, so we
	// can edit it here)

	float *angle = nullptr;

	switch (type) {
	case YAW:
		angle = &m_cameraYaw;
		break;
	case PITCH:
		angle = &m_cameraPitch;
		break;
	case ROLL:
		break;
	default:
		break;
	}

	*angle -= value;
	// keep the value within bounds
	if (*angle < 0) *angle += 360;
	if (*angle >= 360) *angle -= 360;
	// update the camera since we changed the angular value
	UpdateCamera();
	//PrintCameraLocation();
}

void CameraManager::ZoomCamera(float distance) {
	// change the distance value
	m_cameraDistance -= distance;
	// prevent it from zooming in too far
	//if (m_cameraDistance < 0.1f) m_cameraDistance = 0.1f;
	// update the camera since we changed the zoom distance
	UpdateCamera();
	//PrintCameraLocation();
}

void CameraManager::TranslateCamera(TranslateDirection direction, float value) {

	float *positionValue = nullptr;

	switch (direction)
	{
	case UP: {
		positionValue = &m_cameraPosY;
		*positionValue += value;
	}
		break;
	case DOWN: {
		positionValue = &m_cameraPosY;
		*positionValue -= value;
	}
		break;
	case LEFT: {
		positionValue = &m_cameraPosX;
		*positionValue -= value;
	}
		break;
	case RIGHT: {
		positionValue = &m_cameraPosX;
		*positionValue += value;
	}
		break;
	default:
		break;
	}


	UpdateCamera();
	//PrintCameraLocation();
}

void CameraManager::PrintCameraLocation() {
	printf("Camera Position = %f, %f, %f\n", m_cameraPosition[0], m_cameraPosition[1], m_cameraPosition[2]);
	printf("Camera Target = %f, %f, %f \n", m_cameraTarget[0], m_cameraTarget[1], m_cameraTarget[2]);
}

btVector3 CameraManager::GetCameraLocation() {
	return btVector3(m_cameraPosX, m_cameraPosY, m_cameraDistance);
}

void CameraManager::SetCameraLocationX(float X) {
	m_cameraPosX = X;
}

CameraManager::~CameraManager()
{
}
