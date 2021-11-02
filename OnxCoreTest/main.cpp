#include <stdio.h>
#include <stdlib.h>
#include <glew.h>
#include <freeglut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "OpenglWindow.h"

OpenglWindow* oglWindow;

//hooks into the main glut events we use to trigger and control things
int frameRate = 16; //ms
void frameTimer(int timeVal)
{
	glutPostRedisplay();
	glutTimerFunc(frameRate, frameTimer, timeVal + frameRate);
}

void Render()
{
	oglWindow->Render();
}

void Exit()
{
}

void mouseMoveListen(int x, int y)
	{
	oglWindow->setMousePosition(x, y);
	}

void mouseButtonListen(int button, int state, int x, int y)
	{
	oglWindow->setMouseButton(button, state);
	}

int main(int argc, char* argv[])
{
	//pass on command line args
	glutInit(&argc, argv);

	//double buffer, RGBA color
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

	//basic window setup
	glutInitWindowSize(1280, 720);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Enter Name");

	//check that glew is good to go as well
	GLenum res = glewInit();
	if (res != GLEW_OK)
	{
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	glutTimerFunc(0, frameTimer, 0);
	glutDisplayFunc(Render);
	glutPassiveMotionFunc(mouseMoveListen);
	glutMotionFunc(mouseMoveListen);
	glutMouseFunc(mouseButtonListen);

	//glut does a bit of nastyness where it closes things down without going beyond the glutMainLoop() 
	//call below, so have to have an exit function to clean things up
	glutCloseFunc(Exit);

	oglWindow = new OpenglWindow();

	//so it begins...
	glutMainLoop();

	return 0;
}