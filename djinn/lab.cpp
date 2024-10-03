#include <iostream> // Include C++ headers
#include <string>  

#include <GL/glew.h> // Include GLEW

#include <glfw3.h> // Include GLFW


#include <glm/glm.hpp>                   // Include GLM
#include <glm/gtc/matrix_transform.hpp>

#include <common/shader.h>  // Shader loading utilities and other
#include <common/util.h>
#include <common/camera.h>
#include <common/model.h>
#include <common/texture.h>
#include <common/light.h> 
#include "FountainEmitter.h"
#include <cmath>

using namespace std;
using namespace glm;

#define W_WIDTH 1536
#define W_HEIGHT 864 
#define TITLE "Djinn"
#define SHADOW_WIDTH 4096
#define SHADOW_HEIGHT 4096


// Creating a structure to store the material parameters of an object
struct Material {
	vec4 Ka;
	vec4 Kd;
	vec4 Ks;
	float Ns;
};

// Create sample materials
const Material polishedSilver{
	vec4{0.23125, 0.23125, 0.23125, 1},
	vec4{0.2775, 0.2775, 0.2775, 1},
	vec4{0.773911, 0.773911, 0.773911, 1},
	89.6f
};
const Material gold{
	vec4(0.24725, 0.1995, 0.0745, 1),
	vec4(0.75164, 0.60648, 0.22648, 1),
	vec4(0.628281, 0.555802, 0.366065, 1),
	51.2
};


// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();
void DrawObj(Drawable* obj, mat4 modelMatrix, GLuint objDiffuseTexture, GLuint objSpecularTexture);
void SimpleObj(Drawable* obj, mat4 modelMatrix, Material material);
void ShadowObj(Drawable* obj, mat4 modelMatrix, int obj_id);
void pollKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods);


// Global Variables
GLFWwindow* window;
Camera * camera;
Light * light;
GLuint shaderProgram, depthProgram, djinnProgram, particleShaderProgram;

Drawable * lantern, * table, * light_sphere, * ceiling_light, * door, * bookcase, * book, * couch, * djinn;
Drawable *plane, *wallLeft, *wallRight, *wallFront, *wallBack, *wallUp;

GLuint lanternDiffuseTexture, lanternSpecularTexture, tableDiffuseTexture, tableSpecularTexture,
ceilingDiffuseTexture, planeDiffuseTexture, planeSpecularTexture, wallDiffuseTexture, wallSpecularTexture,
doorDiffuseTexture, doorSpecularTexture, bookcaseDiffuseTexture, bookDiffuseTexture, 
couchDiffuseTexture, djinnDiffuseTexture;

GLuint depthFrameBuffer, depthTexture;

Drawable* quad;

// locations for shaderProgram
GLuint viewMatrixLocation, projectionMatrixLocation, modelMatrixLocation, projectionAndViewMatrix,
djinnviewMatrixLocation, djinnprojectionMatrixLocation, djinnmodelMatrixLocation;
GLuint KaLocation, KdLocation, KsLocation, NsLocation;
GLuint LaLocation, LdLocation, LsLocation;
GLuint lightPositionLocation, lightPowerLocation;
GLuint diffuseColorSampler, specularColorSampler, ambienceColorSampler, diffuseParticleSampler;
GLuint particletexture, cointexture;
GLuint useTextureLocation;
GLuint depthMapSampler;
GLuint lightVPLocation, lightDirectionLocation, lightFarPlaneLocation, lightNearPlaneLocation, objShadow;

// locations for depthProgram
GLuint shadowViewProjectionLocation; 
GLuint shadowModelLocation;

// global variables 
float djinn_y = -4.5;
float lantern_x = 0;
float djinn_rot = 0;
bool control = false;
int particles_slider = 10000;
bool coin = false;
bool par_switch = false;
bool pause = false;


// Creating a function to upload the material parameters of a model to the shader program
void uploadMaterial(const Material& mtl) {
	glUniform4f(KaLocation, mtl.Ka.r, mtl.Ka.g, mtl.Ka.b, mtl.Ka.a);
	glUniform4f(KdLocation, mtl.Kd.r, mtl.Kd.g, mtl.Kd.b, mtl.Kd.a);
	glUniform4f(KsLocation, mtl.Ks.r, mtl.Ks.g, mtl.Ks.b, mtl.Ks.a);
	glUniform1f(NsLocation, mtl.Ns);
}

// Draw textured obj
void DrawObj(Drawable* obj, mat4 modelMatrix, GLuint objDiffuseTexture, GLuint objSpecularTexture) {

	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, objDiffuseTexture);
	glUniform1i(diffuseColorSampler, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, objSpecularTexture);
	glUniform1i(specularColorSampler, 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, objDiffuseTexture);
	glUniform1i(ambienceColorSampler, 3);

	glUniform1i(useTextureLocation, 1);

	obj->bind();
	obj->draw();

}

// Draw untextured obj
void SimpleObj(Drawable* obj, mat4 modelMatrix, Material material) {

	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	uploadMaterial(material);
	glUniform1i(useTextureLocation, 0);
	obj->bind();
	obj->draw();
}

// Rendering obj in depth_pass
void ShadowObj(Drawable* obj, mat4 modelMatrix, int obj_id=0) {

	glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	glUniform1i(objShadow, obj_id);
	obj->bind();
	obj->draw();
}

// Creating a function to upload (make uniform) the light parameters to the shader program
void uploadLight(const Light& light) {
	glUniform4f(LaLocation, light.La.r, light.La.g, light.La.b, light.La.a);
	glUniform4f(LdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
	glUniform4f(LsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
	glUniform3f(lightPositionLocation, light.lightPosition_worldspace.x,
		light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
	glUniform1f(lightPowerLocation, light.power);
}

// Keyboard functions
void pollKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
	
	if (key == GLFW_KEY_EQUAL && !pause) {
		control = true;
		djinn_y += 0.04;
	}
	if (key == GLFW_KEY_MINUS && !pause) {
		control = true;
		djinn_y -= 0.04;
	}
	if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
		par_switch = !par_switch;
	}
	if (key == GLFW_KEY_T && action == GLFW_PRESS) {
		GLint polygonMode[2];
		glGetIntegerv(GL_POLYGON_MODE, &polygonMode[0]);

		// if GL_LINE, if GL_FILL check with polygonMode[0]
		if (polygonMode[0] == GL_LINE) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		if (polygonMode[0] == GL_FILL) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (key == GLFW_KEY_R) {
		djinn_y = -4.5;
		control = false;
		par_switch = false;
	}
	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		pause = !pause;
	}
	if (key == GLFW_KEY_C && action == GLFW_PRESS) {
		coin = !coin;
	}
}



void createContext() {

	// Create and complile our GLSL program from the shader
	shaderProgram = loadShaders("ShadowMapping.vertexshader", "ShadowMapping.fragmentshader");
	
	depthProgram = loadShaders("Depth.vertexshader", "Depth.fragmentshader");
	
	djinnProgram = loadShaders("Djinn.vertexshader", "Djinn.fragmentshader");

	particleShaderProgram = loadShaders("ParticleShader.vertexshader","ParticleShader.fragmentshader");
	
	// Get pointers to uniforms
	// --- shaderProgram ---
	projectionMatrixLocation = glGetUniformLocation(shaderProgram, "P");
	viewMatrixLocation = glGetUniformLocation(shaderProgram, "V");
	modelMatrixLocation = glGetUniformLocation(shaderProgram, "M");
	djinnprojectionMatrixLocation = glGetUniformLocation(djinnProgram, "P");
	djinnviewMatrixLocation = glGetUniformLocation(djinnProgram, "V");
	djinnmodelMatrixLocation = glGetUniformLocation(djinnProgram, "M");
	projectionAndViewMatrix = glGetUniformLocation(particleShaderProgram, "PV");

	// for phong lighting
	KaLocation = glGetUniformLocation(shaderProgram, "mtl.Ka");
	KdLocation = glGetUniformLocation(shaderProgram, "mtl.Kd");
	KsLocation = glGetUniformLocation(shaderProgram, "mtl.Ks");
	NsLocation = glGetUniformLocation(shaderProgram, "mtl.Ns");
	LaLocation = glGetUniformLocation(shaderProgram, "light.La");
	LdLocation = glGetUniformLocation(shaderProgram, "light.Ld");
	LsLocation = glGetUniformLocation(shaderProgram, "light.Ls");
	lightPositionLocation = glGetUniformLocation(shaderProgram, "light.lightPosition_worldspace");
	lightPowerLocation = glGetUniformLocation(shaderProgram, "light.power");
	diffuseColorSampler = glGetUniformLocation(shaderProgram, "diffuseColorSampler");
	specularColorSampler = glGetUniformLocation(shaderProgram, "specularColorSampler");
	ambienceColorSampler = glGetUniformLocation(shaderProgram, "ambienceColorSampler");
	useTextureLocation = glGetUniformLocation(shaderProgram, "useTexture"); 
	diffuseParticleSampler = glGetUniformLocation(particleShaderProgram, "texture0");

	particletexture = loadSOIL("textures/particle.jpg");
	cointexture = loadSOIL("textures/coin.jpg");
	// locations for shadow rendering
	depthMapSampler = glGetUniformLocation(shaderProgram, "shadowMapSampler");
	lightVPLocation = glGetUniformLocation(shaderProgram, "lightVP");
	objShadow = glGetUniformLocation(depthProgram, "obj_id");

	// --- depthProgram ---
	shadowViewProjectionLocation = glGetUniformLocation(depthProgram, "VP");
	shadowModelLocation = glGetUniformLocation(depthProgram, "M");
	

	// Loading the models

	
	lantern = new Drawable("objects/lantern.obj");
	lanternDiffuseTexture = loadSOIL("textures/lantern_base.jpg");
	lanternSpecularTexture = loadSOIL("textures/lantern_specular.jpg");
	
	
	table = new Drawable("objects/table.obj");
	tableDiffuseTexture = loadSOIL("textures/table_base.jpg");
	tableSpecularTexture = loadSOIL("textures/table_specular.jpg");


	ceiling_light = new Drawable("objects/lamp.obj", true);
	light_sphere = new Drawable("objects/earth.obj", true);

	door = new Drawable("objects/door.obj");
	doorDiffuseTexture = loadSOIL("textures/door_base.jpg");
	doorSpecularTexture = loadSOIL("textures/door_specular.jpg");

	bookcase = new Drawable("objects/bookcase.obj");
	bookcaseDiffuseTexture = loadSOIL("textures/bookcase_base.jpg");

	book = new Drawable("objects/book.obj");
	bookDiffuseTexture = loadSOIL("textures/book_base.jpeg");

	couch = new Drawable("objects/couch.obj");
	couchDiffuseTexture = loadSOIL("textures/couch_base.jpg");
	
	djinn = new Drawable("objects/wizard.obj");
	djinnDiffuseTexture = loadSOIL("textures/wizard_base.jpg");


	// room vertices
	float y = -1; // offset to move the place up/down across the y axis
	float expansion = 1.0;

	vector<vec3> floorVertices = {
		vec3(-20.0f, y, -20.0f),
		vec3(-20.0f, y,  20.0f),
		vec3(20.0f,  y,  20.0f),
		vec3(20.0f,  y,  20.0f),
		vec3(20.0f,  y, -20.0f),
		vec3(-20.0f, y, -20.0f),

	};

	// plane normals
	vector<vec3> floorNormals = {
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f)
	};

	// plane uvs
	vector<vec2> UVs = {
		vec2(0.0f, 0.0f),
		vec2(3.0f, 0.0f),
		vec2(3.0f, 3.0f),
		vec2(3.0f, 3.0f),
		vec2(0.0f, 3.0f),
		vec2(0.0f, 0.0f),
	};

	// wall left
	vector<vec3> wallVerticesLeft = {
		vec3(-20.0f, y, -20.0f),
		vec3(-20.0f, 10.0 + y,  -20.0f),
		vec3(-20.0f,  10.0 + y, 20.0f),
		vec3(-20.0f,  10.0 + y, 20.0f),
		vec3(-20.0f,  y,  20.0f),
		vec3(-20.0f,  y, -20.0f),
	};

	// wall left normals
	vector<vec3> wallNormalsLeft = {
		vec3(1.0f, 0.0f, 0.0f),
		vec3(1.0f, 0.0f, 0.0f),
		vec3(1.0f, 0.0f, 0.0f),
		vec3(1.0f, 0.0f, 0.0f),
		vec3(1.0f, 0.0f, 0.0f),
		vec3(1.0f, 0.0f, 0.0f)
	};

	//wall right
	vector<vec3> wallVerticesRight = {
		vec3(20.0f, y, -20.0f),
		vec3(20.0f, 10.0 + y,  -20.0f),
		vec3(20.0f,  10.0 + y, 20.0f),
		vec3(20.0f,  10.0 + y, 20.0f),
		vec3(20.0f,  y,  20.0f),
		vec3(20.0f,  y, -20.0f),
	};

	// wall right normals
	vector<vec3> wallNormalsRight = {
		vec3(-1.0f, 0.0f, 0.0f),
		vec3(-1.0f, 0.0f, 0.0f),
		vec3(-1.0f, 0.0f, 0.0f),
		vec3(-1.0f, 0.0f, 0.0f),
		vec3(-1.0f, 0.0f, 0.0f),
		vec3(-1.0f, 0.0f, 0.0f)
	};

	//wall front
	vector<vec3> wallVerticesFront = {
		vec3(-20.0f, y, 20.0f),
		vec3(-20.0f, 10.0 + y,  20.0f),
		vec3(20.0f,  10.0 + y, 20.0f),
		vec3(20.0f,  10.0 + y, 20.0f),
		vec3(20.0f,  y,  20.0f),
		vec3(-20.0f,  y, 20.0f),
	};

	// wall front normals
	vector<vec3> wallNormalsFront = {
		vec3(0.0f, 0.0f, -1.0f),
		vec3(0.0f, 0.0f, -1.0f),
		vec3(0.0f, 0.0f, -1.0f),
		vec3(0.0f, 0.0f, -1.0f),
		vec3(0.0f, 0.0f, -1.0f),
		vec3(0.0f, 0.0f, -1.0f)
	};

	//wall back
	vector<vec3> wallVerticesBack = {
		vec3(-20.0f, y, -20.0f),
		vec3(-20.0f, 10.0 + y,  -20.0f),
		vec3(20.0f,  10.0 + y, -20.0f),
		vec3(20.0f,  10.0 + y, -20.0f),
		vec3(20.0f,  y,  -20.0f),
		vec3(-20.0f,  y, -20.0f),
	};

	// wall back normals
	vector<vec3> wallNormalsBack = {
		vec3(0.0f, 0.0f, 1.0f),
		vec3(0.0f, 0.0f, 1.0f),
		vec3(0.0f, 0.0f, 1.0f),
		vec3(0.0f, 0.0f, 1.0f),
		vec3(0.0f, 0.0f, 1.0f),
		vec3(0.0f, 0.0f, 1.0f)
	};

	//ceiling 
	vector<vec3> wallVerticesUp = {
		vec3(-20.0f, y+10, -20.0f),
		vec3(-20.0f, y+10,  20.0f),
		vec3(20.0f,  y+10,  20.0f),
		vec3(20.0f,  y+10,  20.0f),
		vec3(20.0f,  y+10, -20.0f),
		vec3(-20.0f, y+10, -20.0f),

	};

	// ceiling normals
	vector<vec3> wallNormalsUp = {
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f)
	};

	vector<vec2> wallUVs = {
		vec2(0.0f, 0.0f),
		vec2(0.0f, 1.0f),
		vec2(2.0f, 1.0f),
		vec2(2.0f, 1.0f),
		vec2(2.0f, 0.0f),
		vec2(0.0f, 0.0f),
	};

	plane = new Drawable(floorVertices, UVs, floorNormals);
	planeDiffuseTexture = loadSOIL("textures/plane_base.jpg");
	planeSpecularTexture = loadSOIL("textures/plane_specular.jpg");

	wallLeft = new Drawable(wallVerticesLeft, wallUVs, wallNormalsLeft);
	wallRight = new Drawable(wallVerticesRight, wallUVs, wallNormalsRight);
	wallFront = new Drawable(wallVerticesFront, wallUVs, wallNormalsFront);
	wallBack = new Drawable(wallVerticesBack, wallUVs, wallNormalsBack);
	wallDiffuseTexture = loadSOIL("textures/wall_base.jpg");
	wallSpecularTexture = loadSOIL("textures/wall_specular.jpg");

	wallUp = new Drawable(wallVerticesUp, UVs, wallNormalsUp);
	ceilingDiffuseTexture = loadSOIL("textures/ceiling_base.jpg");



	
	vector<vec3> quadVertices = {
	  vec3(0.5, 0.5, 0.0),
	  vec3(1.0, 0.5, 0.0),
	  vec3(1.0, 1.0, 0.0),
	  vec3(1.0, 1.0, 0.0),
	  vec3(0.5, 1.0, 0.0),
	  vec3(0.5, 0.5, 0.0)
	};

	vector<vec2> quadUVs = {
	  vec2(0.0, 0.0),
	  vec2(1.0, 0.0),
	  vec2(1.0, 1.0),
	  vec2(1.0, 1.0),
	  vec2(0.0, 1.0),
	  vec2(0.0, 0.0)
	};

	quad = new Drawable(quadVertices, quadUVs);


	// ---------------------------------------------------------------------------- //
	// -  Task 3.2 Create a depth framebuffer and a texture to store the depthmap - //
	// ---------------------------------------------------------------------------- //
	
	glGenFramebuffers(1, &depthFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, depthFrameBuffer);


	// We need a texture to store the depth image
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	// Telling opengl the required information about the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
		GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);							
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);		// GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
	

	// Attaching the texture to the framebuffer, so that it will monitor the depth component
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
	

	// Since the depth buffer is only for the generation of the depth texture, 
	// there is no need to have a color output
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);


	// Finally, we have to always check that our frame buffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		glfwTerminate();
		throw runtime_error("Frame buffer not initialized correctly");
	}

	// Binding the default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//*/

}


void free() {
	// Delete Shader Programs
	glDeleteProgram(shaderProgram);
	glDeleteProgram(depthProgram);
	glDeleteProgram(djinnProgram);
	glDeleteProgram(particleShaderProgram);
	glfwTerminate();
}


void depth_pass(mat4 viewMatrix, mat4 projectionMatrix, FountainEmitter f_emitter, FountainEmitter coin_emitter) {


	// Setting viewport to shadow map size
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

	// Binding the depth framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthFrameBuffer);

	// Cleaning the framebuffer depth information (stored from the last render)
	glClear(GL_DEPTH_BUFFER_BIT);

	// Selecting the new shader program that will output the depth component
	glUseProgram(depthProgram);

	// sending the view and projection matrix to the shader
	mat4 view_projection = projectionMatrix * viewMatrix;
	glUniformMatrix4fv(shadowViewProjectionLocation, 1, GL_FALSE, &view_projection[0][0]);


	// ---- rendering the scene ---- //
	//  
	// creating lantern model matrix and sending to GPU
	mat4 modelMatrix = translate(mat4(), vec3(-0.5 + lantern_x, 0.8, 0.0)) * rotate(mat4(), 3 * 1.57f, vec3(0.0f, 1.0f, 0.0f)) * scale(mat4(), vec3(0.4f));
	ShadowObj(lantern, modelMatrix);

	// same for table
	modelMatrix = translate(mat4(), vec3(0.0, -1.0, 0.0)) * rotate(mat4(), 1.57f, vec3(0.0f, 1.0f, 0.0f)) * scale(mat4(), vec3(3.0f));
	ShadowObj(table, modelMatrix);

	// same for the couch
	modelMatrix = translate(mat4(), vec3(7.0, -1.0, 0.0)) * rotate(mat4(), 3 * 1.57f, vec3(0.0f, 1.0f, 0.0f)) * scale(mat4(), vec3(3.0f));
	ShadowObj(couch, modelMatrix);

	// same for djinn
	modelMatrix = translate(mat4(), vec3(0, djinn_y + 0.2, 0)) * rotate(mat4(), djinn_rot, vec3(0.0f, 1.0f, 0.0f)) * scale(mat4(), vec3(3, 3, 3));
	ShadowObj(djinn, modelMatrix, 1);

	// same for djinn particles
	glUniform1i(objShadow, 2);
	f_emitter.renderParticles();

	// same for coin particles
	coin_emitter.renderParticles();

	// binding the default framebuffer again
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	
}

void lighting_pass(mat4 viewMatrix, mat4 projectionMatrix) {

	// Step 1: Binding a frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, W_WIDTH, W_HEIGHT);

	// Step 2: Clearing color and depth info
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Step 3: Selecting shader program
	glUseProgram(shaderProgram);

	// Making view and projection matrices uniform to the shader program
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

	// uploading the light parameters to the shader program
	uploadLight(*light);

	// Display shadows on the scene
	
	// Sending the shadow texture to the shaderProgram
	glActiveTexture(GL_TEXTURE0);								// Activate texture position
	glBindTexture(GL_TEXTURE_2D, depthTexture);			// Assign texture to position 
	glUniform1i(depthMapSampler, 0);

	// Sending the light View-Projection matrix to the shader program
	mat4 light_VP = light->lightVP();
	glUniformMatrix4fv(lightVPLocation, 1, GL_FALSE, &light_VP[0][0]);
	


	// ----------------------------------------------------------------- //
	// --------------------- Drawing scene objects --------------------- //	
	// ----------------------------------------------------------------- //

	//Draw the lantern on the scene
	mat4 modelMatrix = translate(mat4(), vec3(-0.5 + lantern_x, 0.8 , 0.0)) * rotate(mat4(), 3*1.57f, vec3(0.0f, 1.0f, 0.0f)) * scale(mat4(), vec3(0.4f));
	DrawObj(lantern, modelMatrix, lanternDiffuseTexture, lanternSpecularTexture);

	//Draw the table on the scene
	modelMatrix = translate(mat4(), vec3(0.0, -1.0, 0.0)) * rotate(mat4(), 1.57f, vec3(0.0f, 1.0f, 0.0f)) * scale(mat4(), vec3(3.0f));
	DrawObj(table, modelMatrix, tableDiffuseTexture, tableSpecularTexture);

	//Draw the lamp on the scene
	modelMatrix = translate(mat4(), vec3(-5.0, 9.5, 7.0)) * rotate(mat4(), 3*1.57f, vec3(1.0f, 0.0f, 0.0f)) * scale(mat4(), vec3(0.3f));
	SimpleObj(ceiling_light, modelMatrix, gold);

	//Draw the door on the scene
	modelMatrix = translate(mat4(), vec3(0.0, 6.4, -20.0)) * scale(mat4(), vec3(3.5f));
	DrawObj(door, modelMatrix, doorDiffuseTexture, doorSpecularTexture);

	//Draw the bookcase1 on the scene
	modelMatrix = translate(mat4(), vec3(19.4, -1.15, 0.0)) * scale(mat4(), vec3(30.0f));
	DrawObj(bookcase, modelMatrix, bookcaseDiffuseTexture, bookcaseDiffuseTexture);
	
	//Draw the bookcase2 on the scene
	modelMatrix = translate(mat4(), vec3(-15.0, -1.15, 19.4)) * rotate(mat4(), 3*1.57f, vec3(0.0f, 1.0f, 0.0f)) * scale(mat4(), vec3(30.0f));
	DrawObj(bookcase, modelMatrix, bookcaseDiffuseTexture, bookcaseDiffuseTexture);
	
	//Draw the book on the scene
	modelMatrix = translate(mat4(), vec3(19.4, 4.1 , -1.0)) * scale(mat4(), vec3(45.0f));
	DrawObj(book, modelMatrix, bookDiffuseTexture, bookDiffuseTexture);

	//Draw the couch on the scene
	modelMatrix = translate(mat4(), vec3(7.0, -1.0, 0.0)) * rotate(mat4(), 3 * 1.57f, vec3(0.0f, 1.0f, 0.0f)) * scale(mat4(), vec3(3.0f));
	DrawObj(couch, modelMatrix, couchDiffuseTexture, couchDiffuseTexture);

	//Draw the light_sphere on the scene
	modelMatrix = translate(mat4(), light->lightPosition_worldspace) * scale(mat4(), vec3(0.2));;
	SimpleObj(light_sphere, modelMatrix, gold);

	
	//Draw the room on the scene 
	DrawObj(plane, mat4(1.0), planeDiffuseTexture, planeSpecularTexture);
	DrawObj(wallLeft, mat4(1.0), wallDiffuseTexture, wallSpecularTexture);
	DrawObj(wallRight, mat4(1.0), wallDiffuseTexture, wallSpecularTexture);
	DrawObj(wallFront, mat4(1.0), wallDiffuseTexture, wallSpecularTexture);
	DrawObj(wallBack, mat4(1.0), wallDiffuseTexture, wallSpecularTexture);
	DrawObj(wallUp, mat4(1.0), ceilingDiffuseTexture, ceilingDiffuseTexture);



	glUseProgram(djinnProgram);

	// Making view and projection matrices uniform to the shader program
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
	uploadLight(*light);

	// Sending the shadow texture to the djinnProgram
	glActiveTexture(GL_TEXTURE0);								// Activate texture position
	glBindTexture(GL_TEXTURE_2D, depthTexture);			// Assign texture to position 
	glUniform1i(depthMapSampler, 0);

	// Sending the light View-Projection matrix to the shader program
	glUniformMatrix4fv(lightVPLocation, 1, GL_FALSE, &light_VP[0][0]);

	//Draw the djinn on the scene
	modelMatrix = translate(mat4(), vec3(0, djinn_y+0.2, 0)) * rotate(mat4(), djinn_rot, vec3(0.0f, 1.0f, 0.0f)) * scale(mat4(), vec3(4, 3, 4));
	DrawObj(djinn, modelMatrix, djinnDiffuseTexture, djinnDiffuseTexture);
}



void mainLoop() {

	light->update();
	mat4 light_proj = light->projectionMatrix;
	mat4 light_view = light->viewMatrix;

	auto* quadpar = new Drawable("objects/earth.obj");
	auto* coinpar = new Drawable("objects/coin.obj");

	FountainEmitter f_emitter = FountainEmitter(quadpar, particles_slider);
	f_emitter.use_rotations = true;
	f_emitter.emitter_pos = vec3(1.0, 1.0, 0.0);

	FountainEmitter coin_emitter = FountainEmitter(coinpar, particles_slider/7);
	coin_emitter.use_rotations = true;
	coin_emitter.emitter_pos = vec3(0.0, 10.0, 0.0);

	float t = glfwGetTime();
	float temp = t;

	do {
		bool reset = false;
		
		light->update();
		mat4 light_proj = light->projectionMatrix;
		mat4 light_view = light->viewMatrix;


		float currentTime = glfwGetTime();
		float dt = currentTime - t;

		// Getting camera information
		camera->update();
		mat4 projectionMatrix = camera->projectionMatrix;
		mat4 viewMatrix = camera->viewMatrix;

		// Djinn rotation
		float x = camera->position.x - 2.26;
		float z = camera->position.z - 1.71;
		float rad = 0;

		if (abs(z) > abs(x) && x > 0 && z > 0) rad = atan(x / z);
		else if (abs(z) > abs(x) && x < 0 && z>0) rad = atan(x / z);
		else if (abs(x) > abs(z) && x > 0 && z > 0) rad = atan(x / z);
		else if (abs(x) > abs(z) && x > 0 && z < 0) rad = 3.14 + atan(x / z);

		else if (abs(z) > abs(x) && x > 0 && z < 0) rad = 3.14 + atan(x / z);
		else if (abs(z) > abs(x) && x < 0 && z < 0) rad = 3.14 + atan(x / z);
		else if (abs(x) > abs(z) && x < 0 && z < 0) rad = 3.14 + atan(x / z);
		else if (abs(x) > abs(z) && x < 0 && z > 0) rad = atan(x / z);

		if (!pause)	djinn_rot = rad;

		// Djinn coming out of lamp, lamp vibrating
		if (!control && !pause) {
			djinn_y += 0.45 * dt;
			lantern_x = 0.05 * sin(50*currentTime);
			if (djinn_y > 0.8) {
				djinn_y = 0.8;
				lantern_x = 0;
			}
		}
		
		// Rendering the scene from light's perspective when F1 is pressed
		if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
			lighting_pass( light_view, light_proj );
		}
		else {
			lighting_pass( viewMatrix, projectionMatrix );
		}


		// Smoke Particles

		glUseProgram(particleShaderProgram);
		auto PV = projectionMatrix * viewMatrix;
		glUniformMatrix4fv(projectionAndViewMatrix, 1, GL_FALSE, &PV[0][0]);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, particletexture);
		glUniform1i(diffuseParticleSampler, 0);

		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			reset = true;
			coin = false;
		}
		

		if (!par_switch && !pause && !reset) f_emitter.updateParticles(currentTime, dt);
		else if (!par_switch && pause  && !reset);
		else f_emitter.updateParticles(0, dt);
		f_emitter.renderParticles();


		//coins
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cointexture);
		glUniform1i(diffuseParticleSampler, 0);

		if (coin && !par_switch && !pause && !reset) coin_emitter.updateCoins(currentTime, dt);
		else if (coin && !par_switch && pause && !reset);
		else if (coin && par_switch) coin_emitter.updateCoins(0, dt);
		if (reset) coin_emitter.updateCoins(0, -1);
		coin_emitter.renderParticles();

		// Shadowing
		depth_pass(light_view, light_proj, f_emitter, coin_emitter);
		
		glfwSwapBuffers(window);
		glfwPollEvents();
		t = currentTime;
	} while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

}


void initialize() {
	// Initialize GLFW
	if (!glfwInit()) {
		throw runtime_error("Failed to initialize GLFW\n");
	}


	glfwWindowHint(GLFW_SAMPLES, 16);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	// Open a window and create its OpenGL context
	window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, NULL, NULL);
	if (window == NULL) {
		glfwTerminate();
		throw runtime_error(string(string("Failed to open GLFW window.") +
			" If you have an Intel GPU, they are not 3.3 compatible." +
			"Try the 2.1 version.\n"));
	}
	glfwMakeContextCurrent(window);

	// Start GLEW extension handler
	glewExperimental = GL_TRUE;

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		glfwTerminate();
		throw runtime_error("Failed to initialize GLEW\n");
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Hide the mouse and enable unlimited movement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, W_WIDTH / 2, W_HEIGHT / 2);

	// Gray background color
	glClearColor(0.5f, 0.5f, 0.5f, 0.0f);

	glfwSetKeyCallback(window, pollKeyboard);
	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	// glEnable(GL_CULL_FACE);

	// enable texturing and bind the depth texture
	glEnable(GL_TEXTURE_2D);

	// Log
	logGLParameters();

	// Create camera
	camera = new Camera(window);

	// Creating a custom light 
	light = new Light(window,
		vec4{ 1, 0.77, 0.56, 1 },
		vec4{ 1, 0.77, 0.56, 1 },
		vec4{ 1, 0.77, 0.56, 1 },
		vec3{ -5, 7.85, 7 },
		85
	);

}

int main(void) {
	try {
		initialize();
		createContext();
		mainLoop();
		free();
	}
	catch (exception& ex) {
		cout << ex.what() << endl;
		getchar();
		free();
		return -1;
	}

	return 0;
}