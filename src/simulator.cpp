#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <algorithm>
using namespace glm;
#include <vector>
#include <iostream>

#include "simulator.h"
#include "camera.h"

void Simulator::init() {
	FBO = new Framebuffer();

	/* Create new grid */
	grid = new Grid();
	grid->init(100, 0.1, 1);

	/* Create Camera and Set Projection Matrix */
	CAMERA = new Camera();
	CAMERA->init(SCR_WIDTH, SCR_HEIGHT);

	/* BUILD & COMPILE SHADERS */
	cellShader = new Shader("../src/shaders/defaultShader.vert", "../src/shaders/defaultShader.frag");
	renderShader = new Shader("../src/shaders/texShader.vert", "../src/shaders/renderShader.frag");

	/*Misc*/
	glEnable(GL_DEPTH_TEST);

	bindScreenVertices();

	/*Set Perspective Matrix*/
	changeScrDimensions((int) SCR_WIDTH, (int) SCR_HEIGHT);
	/*Set o2w values*/
	rotateX = 0; 
	rotateY = 0;
}

void Simulator::moveCamera(vec3 moveBy) {
	CAMERA->move(moveBy);
}

void Simulator::rotateCamera(double deltaX, double deltaY) {
	rotateX += deltaX/2.;
	rotateY += deltaY/2.;
}

void Simulator::changeScrDimensions(int width, int height) {
	SCR_WIDTH = (float) width;
	SCR_HEIGHT = (float) height;

	CAMERA->changeScreenDimens(SCR_WIDTH, SCR_HEIGHT);
	glViewport(0, 0, (unsigned int) SCR_WIDTH, (unsigned int)SCR_HEIGHT);

	//Projection matrix (Camera to screen)
	glm::mat4 c2s;
	c2s = perspective(radians(60.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

	cellShader->use();
	cellShader->setMat4("projection", c2s);
	renderShader->use();
	renderShader->setMat4("projection", c2s);

	int size = min(height, width);
	std::vector<float> buffer = std::vector<float> (size * size * 3, 0.f);
	screenTex = FBO->createTexture(800, &buffer[0]);
	screenFBO = FBO->createFBO(screenTex);
}

void Simulator::bindScreenVertices() {
	
	/* SCREEN VERTICES */
	float screen[] = {
		//Vertices       //Tex Coords
		-1.f, -1.f, 0.f,    0.f, 0.f,
		-1.f, 1.f, 0.f,    0.f, 1.f,
		1.f, 1.f, 0.f,    1.f, 1.f,

		-1.f, -1.f, 0.f,    0.f, 0.f,
		1.f, -1.f, 0.f,    1.f, 0.f,
		1.f, 1.f, 0.f,    1.f, 1.f,
	};

	glGenBuffers(1, &screenVBO);
	glGenVertexArrays(1, &screenVAO);

	/* Bind VBO and set VBO data */
	glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
	glBufferData(GL_ARRAY_BUFFER, 6 * 5 * sizeof(float), screen, GL_STATIC_DRAW);

	/* Bind VAO and set VAO configuration */
	glBindVertexArray(screenVAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);

	/* Unbind */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Simulator::simulate(float time) {
	grid->stepOnce(5);
	grid->extForces(time);
	grid->projectGPU(10);
	grid->moveDye(time);
	drawContents();
}

void Simulator::drawContents() {
	render();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	/* CLEAR PREVIOUS */
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	cellShader->use();
	glBindTexture(GL_TEXTURE_2D, screenTex);
	glBindVertexArray(screenVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void Simulator::render() {
	int size = min(SCR_WIDTH, SCR_HEIGHT);
	glViewport(0, 0, size, size);
	glBindVertexArray(screenVAO);
	glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
	/* CLEAR PREVIOUS */
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Set View Matrix */
	//View matrix
	glm::mat4 w2c;
	w2c = CAMERA->getViewMatrix();
	glm::mat4 o2w;
	o2w = translate(o2w, vec3(0.f, 0.f, CAMERA->camPos.z - 1.f));
	glm::mat4 obj;
	obj = rotate(obj, radians(rotateY), vec3(1.f, 0.f, 0.f));
	obj = rotate(obj, radians(rotateX), vec3(0.f, 1.f, 0.f));

	renderShader->use();
	renderShader->setMat4("view", w2c);
	renderShader->setVec4("camPos", vec4(CAMERA->camPos, 1.f));
	renderShader->setMat4("model", o2w);
	renderShader->setMat4("object", obj);
	renderShader->setFloat("cellSize", grid->cell_size/2.f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, grid->renderToScreen);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}