// Falling Boxes.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FreeGLUTCallbacks.h"
#include "ContactLearningApp.h"


int main(int argc, char **argv)
{
	//ContactLearningApp contactLearningApp(ORTHOGRAPHIC);
	ContactLearningApp contactLearningApp(PERSPECTIVE);
	return glutmain(argc, argv, 1024, 768, "Simple Contact Learning", &contactLearningApp);
}
