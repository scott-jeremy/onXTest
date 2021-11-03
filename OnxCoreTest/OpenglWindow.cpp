#include "OpenglWindow.h"
#include <algorithm>
#include <cmath>
#include <glm.hpp>
#include <vector>
#include <array>

using namespace std;

OpenglWindow::OpenglWindow(void) :
	  mColorShader((GLuint)-1)
	, mColorLoc((GLuint)-1)
{
	////setup the shader
	mColorShader = setupShader((char*)"Resources/colorVertShader.txt", (char*)"Resources/colorPixelShader.txt");
	mColorLoc = glGetUniformLocation(mColorShader, "Color");
	setMousePosition(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
	memset(&_mouseButtons, 0, sizeof(_mouseButtons));
}

OpenglWindow::~OpenglWindow(void)
{
	glDeleteProgram(mColorShader);
}

// Generate a modelview matrix
glm::mat4x4 lookAt(glm::vec3 const &eye, glm::vec3 const &focus, glm::vec3 const &up)
	{
	auto zaxis = glm::normalize(eye - focus);
	auto xaxis = glm::normalize(glm::cross(zaxis, up));
	auto yaxis = glm::cross(xaxis, zaxis);

	auto xdot = dot(xaxis, eye),
		ydot = dot(yaxis, eye),
		zdot = dot(zaxis, eye);

	return glm::mat4x4(
					xaxis.x, yaxis.x, zaxis.x, 0.f,
					xaxis.y, yaxis.y, zaxis.y, 0,
					xaxis.z, yaxis.z, zaxis.z, 0,
					-xdot, -ydot, -zdot, 1.f
					);
	}

// Generate a perspective projection matrix
glm::mat4x4 perspective(float fov, float aspect, float znear, float zfar)
	{
	float yscale = 1 / tan(0.5f * fov);
	float xscale = yscale / aspect;

	return glm::mat4x4
		(
		xscale, 0, 0, 0,
		0, yscale, 0, 0,
		0, 0, zfar / (znear - zfar), -1,
		0, 0, znear * zfar / (znear - zfar), 0
		);
	}

// Generate colors, a gradient from blue->green as depth increases and
// red increasing as the subdivided quad increases in projected size
glm::vec4 makeTileColor(float depth, float length, float detail)
	{
	float scaled = depth / float(MAX_SUBDIVISION_DEPTH);
	return glm::vec4(scaled, std::max(1.f - (length / detail), 0.f), 1.0f - scaled, 1);
	}

// Recursively subdivides a quad, culling the output using the current
// _viewProjMatrix; results are available in the _tiles vector.
// There's a small bug in the culling algorithm that allows some tiles
// that are directly behind the camera to pass when the detail level is
// very low; didn't get a chance to track down what's causing that, but
// it's not causing performance issues so it won't interfere with a demo.
bool OpenglWindow::generateTiles(glm::vec2 const p1, glm::vec2 const p2, int const depth)
	{
	if (_maxFrameDetail < depth)
		_maxFrameDetail = depth;

	if (depth == MAX_SUBDIVISION_DEPTH)
		{
		_tiles.push_back(Tile(p1, p2, glm::vec4(1.f, 1.f, 1.f, 1.f)));
		return true;
		}

	glm::vec4 const corners[] =
		{								//		0		1
		glm::vec4(p1.x, p1.y, 0, 1),	//	 P1	+-------+ 
		glm::vec4(p2.x, p1.y, 0, 1),	//		|		|
		glm::vec4(p2.x, p2.y, 0, 1),	//		|		|
		glm::vec4(p1.x, p2.y, 0, 1)		//		+-------+ P2
		};								//		3		2

	glm::vec4 projected[] =
		{
		_viewProjMatrix * corners[0],
		_viewProjMatrix * corners[1],
		_viewProjMatrix * corners[2],
		_viewProjMatrix * corners[3]			
		};

	int behindCount = 0;
	for (int i = 0; i < 4; ++i)
		if (projected[i].w < 0)
			{
			++behindCount;
			projected[i].w = std::abs(projected[i].w);
			}

	if (behindCount == 4)
		return false;

	glm::vec3 screenCorners[] =
		{
		glm::vec3(projected[0].x, projected[0].y, projected[0].z) / projected[0].w,
		glm::vec3(projected[1].x, projected[1].y, projected[1].z) / projected[1].w,
		glm::vec3(projected[2].x, projected[2].y, projected[2].z) / projected[2].w,
		glm::vec3(projected[3].x, projected[3].y, projected[3].z) / projected[3].w,
		};

	if ((screenCorners[0].z >= 1 &&
		screenCorners[1].z >= 1 &&
		screenCorners[2].z >= 1 &&
		screenCorners[3].z >= 1) ||
		(projected[0].w <= 0.0f &&
			projected[1].w <= 0.0f &&
			projected[2].w <= 0.0f &&
			projected[3].w <= 0.0f))
		return false; // It's outside of Z clip space

	for (int dim = 0; dim <= 1; ++dim)
		if ((screenCorners[0][dim] >= 1.f &&
				screenCorners[1][dim] >= 1.f &&
				screenCorners[2][dim] >= 1.f &&
				screenCorners[3][dim] >= 1.f)
			|| (screenCorners[0][dim] <= -1.f &&
				screenCorners[1][dim] <= -1.f &&
				screenCorners[2][dim] <= -1.f &&
				screenCorners[3][dim] <= -1.f))
			{
			return false; // It's outside of X or Y clip space
			}

	glm::vec2 edges[] =
		{
		screenCorners[0] - screenCorners[1],
		screenCorners[1] - screenCorners[2],
		screenCorners[2] - screenCorners[3],
		screenCorners[3] - screenCorners[0]
		};

	float lengths[] =
		{							//			0
		glm::length(edges[0]),		//	 	+-------+
		glm::length(edges[1]),		//	  3 |		| 1
		glm::length(edges[2]),		//		|		|
		glm::length(edges[3]),		//		+-------+
		};							//			2

	if (lengths[0] <= _detail &&
		lengths[1] <= _detail &&
		lengths[2] <= _detail &&
		lengths[3] <= _detail)
		{
		auto minLen = std::min(std::min(lengths[0], lengths[1]), std::min(lengths[2], lengths[3]));
		_tiles.push_back(Tile(p1, p2, makeTileColor(depth, minLen, _detail)));
		return false;
		}

	// This check is 'redundant' with the recursive step for generating the 4 sub-quads, but
	// it allows us to save a lot of work by using values that we've already computed and
	// by not having to re-project/clip the quads if we already know they are small enough
	// to not need it.
	// The worst side-effect would be to potentially include some quads around the edges of
	// clip space whose parent passed clipping but they themselves would not have.
	bool quadrants[] = { false,			//		+---+---+
		false,							//		|_0_|_1_|
		false,							//		| 3 | 2 |
		false							//		+---+---+
		};

	for (auto i = 0; i < 4; ++i)
		if (lengths[i] > _detail)
			{
			quadrants[i] = true;
			quadrants[(i+1) & 3] = true;
			}

	auto center = (p1 + p2) * 0.5f;

	auto result = false;
	for (auto i = 0; i < 4; ++i)
		{
		if (!quadrants[i])
			_tiles.push_back(Tile(corners[i], center, makeTileColor(depth, lengths[i], _detail)));
		else
			result |= generateTiles(corners[i], center, depth + 1);
		}
	
	return result;
	}

// Set the OpenGL camera to a fixed location to see the results of
// the tile generation/culling algorithm, disables front/backface 
// culling, and sets fill mode to wireframe
void OpenglWindow::setDebugCamera()
	{
	auto mPosition = glm::vec3(10, 0, 10);
	auto mLookPoint = glm::vec3(0, 0, 0);

	//depth test on
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_NONE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//projection rendering
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float angle = 45.0f;
	float ratio = 1280.0f / 720.0f;
	float nearPlane = 1.0f;
	float farPlane = 100.0f;
	gluPerspective(angle,
					ratio,
					nearPlane, farPlane); //z near and far

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(mPosition.x, mPosition.y, mPosition.z,  //eye
				mLookPoint.x, mLookPoint.y, mLookPoint.z,      //look at point
				0.0, 0.0, 1.0);
	}

// Set the OpenGL camera to the orbit camera's location, disables
// front/backface culling, and sets the fill mode to solid
void OpenglWindow::setDeviceCamera()
	{
	//depth test on
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_NONE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//projection rendering
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(_fov,
					_aspect,
					_nearClip, _farClip); //z near and far

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(_cameraPosition.x, _cameraPosition.y, _cameraPosition.z,  //eye
				_focusPosition.x, _focusPosition.y, _focusPosition.z,      //look at point
				0.0, 0.0, 1.0);      //up
	}

//main render loop
void OpenglWindow::Render()
{
	if (!_cameraMoved)
		return;

	_maxFrameDetail = 0;

	_cameraMoved = false;
	_tiles.clear();

	bool debugCamera = false;

	updateCameraDistance();

	generateTiles(glm::vec2(5, 5), glm::vec2(-5, -5), 0);

	debugCamera = _lmb.state && _rmb.state;

	if (!debugCamera)
		setDeviceCamera();
	else
		setDebugCamera();

	//clear to black
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//make sure we can cleanup
	glPushMatrix();
	
	glUseProgram(mColorShader);

	for (auto &quad : _tiles)
		{
		auto level = quad.LoD / 20.0f;
		glUniform4f(mColorLoc, quad.Color.r, quad.Color.g, quad.Color.b, quad.Color.a);

		glBegin(GL_QUADS);
		glVertex3f(quad.P1.x,	quad.P1.y, 0);
		glVertex3f(quad.P2.x,	quad.P1.y, 0);
		glVertex3f(quad.P2.x,	quad.P2.y, 0);
		glVertex3f(quad.P1.x,	quad.P2.y, 0);
		glEnd();
		}

	if (debugCamera)
		{
		glUniform4f(mColorLoc, 0, 1, 0, 1);

		glBegin(GL_TRIANGLES);
		glVertex3f(_cameraPosition.x, _cameraPosition.y, _cameraPosition.z);
		glVertex3f(_focusPosition.x, _focusPosition.y, _focusPosition.z);
		glVertex3f(_cameraPosition.x, _cameraPosition.y, 0);
		glEnd();
		}

	//cleanup any matrix changes
	glPopMatrix();

	//bring our render calls to the visible buffer
	glutSwapBuffers();
	glFlush();
}

//load and compile the shaders
GLuint OpenglWindow::setupShader(char* vertPath, char* pixelPath)
{
	ifstream vertFile(vertPath);
	if(!vertFile)
	{
		cout << "could not read vert file " <<  vertPath;
		return (GLuint)-1;
	}
	stringstream  vertBuffer;
	vertBuffer << vertFile.rdbuf();
	string vs = vertBuffer.str();
	vertShaderText = &vs[0u];
	vertFile.close();
	vertBuffer.clear();

	//compile vert shader
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertShader, 1, &vertShaderText, NULL);
	glCompileShader(vertShader);

	GLint succeeded;
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &succeeded);
	if(!succeeded)
	{
		GLchar log[1024];
		glGetShaderInfoLog(vertShader, 1024, NULL, log);
		cout << "Vert shader " << vertPath << ":\n" << log;
	}

	ifstream pixelFile(pixelPath);
	if(!pixelFile)
	{
		cout << "could not read pixel file " << pixelPath;
		return (GLuint)-1;
	}

	stringstream pixelBuffer;
	pixelBuffer << pixelFile.rdbuf();
	string ps = pixelBuffer.str();
	pixelShaderText = &ps[0u];
	pixelFile.close();
	pixelBuffer.clear();

	//compile pixel shader
	GLuint pixelShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(pixelShader, 1, &pixelShaderText, NULL);
	glCompileShader(pixelShader);

	succeeded;
	glGetShaderiv(pixelShader, GL_COMPILE_STATUS, &succeeded);
	if(!succeeded)
	{
		GLchar log[1024];
		glGetShaderInfoLog(pixelShader, 1024, NULL, log);
		cout << "Pixel shader " << pixelPath << ":\n" <<log;
	}

	//link it all together
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertShader);
	glAttachShader(shaderProgram, pixelShader);
	glLinkProgram(shaderProgram);

	//cleanup
	glDeleteShader(vertShader);
	glDeleteShader(pixelShader);

	return shaderProgram;
}

void OpenglWindow::updateCameraDistance()
	{
	if (_lmb.state ^ _rmb.state)
		{
		// These min/max extents should be dependent on _detail
		if (_lmb.state && !_rmb.state)
			_distance = std::max(_distance * 0.97f, 0.00001f);
		else if (_rmb.state)
			_distance = std::min(_distance * 1.03f, 20.f);
		updateCamera();
		}
	}

// Drives the near/far clip plane
void OpenglWindow::setDetailLevel(float detail)
	{
	_detail = std::min(std::max(detail, 0.05f), 1.5f);
	updateCamera();
	}

// Update view/projection matrices used to generate the displayed tiles
void OpenglWindow::updateCamera()
	{
	auto posYZ = glm::vec3(0, std::sin(_orbitYZ * HALF_PI), std::cos(_orbitYZ * HALF_PI));
	auto posXZ = glm::vec3(posYZ.y * std::sin(_orbitXZ * PI * 2), posYZ.y * std::cos(_orbitXZ * PI * 2), posYZ.z);

	_cameraPosition = posXZ * _distance;
	_focusPosition = glm::vec3(0, 0, 0);

	_viewMatrix = lookAt(_cameraPosition, _focusPosition, glm::vec3(0, 0, 1));

	float angle = 45.0f;
	float ratio = 1280.0f / 720.0f;
	_nearClip = std::min(_distance * 0.05f, 1.f);
	_farClip = _nearClip * 100.f;

	_projMatrix = perspective(_fov, _aspect, _nearClip, _farClip);
	_viewProjMatrix = _projMatrix * _viewMatrix;
	_cameraMoved = true;
	}

// Listen for OpenGL mouse move events
void OpenglWindow::setMousePosition(int x, int y)
	{
	_orbitXZ = (float)x / WINDOW_WIDTH;
	_orbitYZ = std::max(0.05f, std::min(float(y) / WINDOW_HEIGHT, 0.95f)); // Prevent looking straight down
	updateCamera();
	}

// Listen for OpenGL mouse button events, strangely the mouse wheel
// seems to be mapped to mouse buttons 3 & 4 which don't have
// corresponding GLUT constants.  Wheel up increases detail level,
// wheel down decreases detail
void OpenglWindow::setMouseButton(int button, int state)
	{
	if (button >= 0 && button < MAX_MOUSE_BUTTONS)
		{
		_mouseButtons[button].state = state == GLUT_DOWN;
		_mouseButtons[button].stateTime = 0;
		_cameraMoved = true;
		}
	else if (button == 3 && state == GLUT_DOWN) // mouse wheel up
		setDetailLevel(_detail * 0.9f);
	else if (button == 4 && state == GLUT_DOWN) // mouse wheel up
		setDetailLevel(_detail * 1.1f);
	}