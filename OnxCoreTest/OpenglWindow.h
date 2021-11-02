#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <glew.h>
#include <freeglut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Types.h"

#ifndef _OGLWINDOW_H_
#define _OGLWINDOW_H_

#define MAX_MOUSE_BUTTONS GLUT_RIGHT_BUTTON + 1

constexpr float PI = 3.141592653589793238463f;
constexpr float HALF_PI = PI * 0.5f;
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

struct MouseButton
	{
	bool state;
	int stateTime;
	};

class OpenglWindow
{

public:
	OpenglWindow(void);
	~OpenglWindow(void);
	void Render();
	GLuint setupShader(char* vertPath, char* pixelPath);
	void setMousePosition(int x, int y);
	void setMouseButton(int button, int state);
private:
	float _orbitXZ,
			_orbitYZ,
			_distance = 10,
			_detail = 0.2f;
	bool _cameraMoved = true;
	int _maxFrameDetail = 0;

	MouseButton _mouseButtons[MAX_MOUSE_BUTTONS];
	MouseButton &_lmb = _mouseButtons[GLUT_LEFT_BUTTON],
				&_rmb = _mouseButtons[GLUT_RIGHT_BUTTON];

	std::vector<Tile> _tiles;
	char* vertShaderText;
	char* pixelShaderText;
	GLuint mColorShader;
	GLint mColorLoc;
	glm::mat4x4 _viewMatrix,
		_projMatrix,
		_viewProjMatrix;
	glm::vec3 _cameraPosition,
		_focusPosition;

	float _fov = 45.0f;
	float _aspect = float(WINDOW_WIDTH) / float(WINDOW_HEIGHT);
	float _nearClip = .5f;
	float _farClip = 50.f;

	void updateCamera();
	void setDetailLevel(float detail);
	bool generateTiles(glm::vec2 const p1, glm::vec2 const p2, int const depth);
	void setDeviceCamera();
	void setDebugCamera();
};
#endif