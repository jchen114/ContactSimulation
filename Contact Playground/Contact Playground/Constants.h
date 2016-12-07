#pragma once

#include <freeglut\freeglut.h>
#include "LinearMath\btVector3.h"

enum ProjectionMode{ ORTHOGRAPHIC, PERSPECTIVE };
enum Dimension{ HEIGHT, WIDTH };

#define CAMERA_STEP_SIZE 1.0f
#define Z_PLANE  0

#define DEG_2_RAD 0.0174533
#define PI 3.1415927
#define TWO_PI 2 * PI

#define SCALING_FACTOR 1.0f

class Constants
{

private:

	ProjectionMode m_projectionMode;
	int m_screenWidth;
	int m_screenHeight;

public:

	static Constants& GetInstance() {
		static Constants instance;
		return instance;
	}

	void SetScreenWidth(int width);
	void SetScreenHeight(int height);
	void SetProjectionMode(ProjectionMode mode);

	int GetScreenWidth();
	int GetScreenHeight();

	ProjectionMode GetProjectionMode();

	float GetMetersToPixels(float dist2Camera);

	float DegreesToRadians(float degrees);
	float RadiansToDegrees(float radians);

	// Frame Rate
	int m_StartTime;
	int m_PrevTime;
	int m_CurrentTime;

protected:

	Constants();
	~Constants();
};

void DrawCircle(const float &radius);
void DisplayText(float x, float y, const btVector3 &color, const char *string);
float AngleBetweenVectors(const btVector3 &v1, const btVector3 &v2);
btVector3 Vector2DWithAngle(float angleRadians, const btVector3 &vector);