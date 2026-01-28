#pragma once
#ifndef SMAA
	#define SMAA 4
#endif // SMAA
#ifdef _WIN32
#include<malloc.h>
#endif // _WIN32

#include<memory.h>
#include<stdlib.h>
#include<cglm/cglm.h>

typedef struct Screen_t {
	int width;
	int height;
	int superSampleRate;
	int* pixelBuffer;
	float* z_buffer;
	int* drawlingBuffer;
}Screen;


typedef struct perpsective_t {
	float fov;
	float z_near;
	float z_far;
	float aspect_ratio;
	mat4 project_matrix; // projection matrix 
}perspective;


typedef struct Camera_t {
	perspective projection_perspective;
	//mat4 camera_to_world; // look at matrix
	//mat4 view_matrix; //inverse of the above
	vec3 pos; // inital position is just the 
	vec3 up;
	vec3 forward;
	vec3 right;
	vec3 target; // used in look at transformation
	float pitch; // up down angle
	float yaw; // left right angle

	float last_pitch;
	float last_yaw;
}Camera;


typedef struct triagnle_t {
	vec3 v1;
	vec3 v2;
	vec3 v3;
}triangle;


typedef struct sphere_t {
	vec4 center; // in homogenous cords
	float radius;
}sphere;

typedef struct object_t {
	mat4 modelMat;
	vec3 boundingBox[2];
	vec3 postion;// in world space
	vec3 scale;
	vec3* mesh; // defition of the vertices
	ivec3* faceList;
	ivec3* vertex_color; //rgb channels should consiend with the vertex at the same idx
	size_t meshSize;
	size_t faceListLength;
	float x_rotation; // in model space, around x axis
	float y_rotation; // in model space
	float z_rotation; // in model space
	int   has_vertex_color; // > 0 yes, <= 0 no
}object;



void initRenderCode(int width, int height, int ssma_mode);

void updateScreenSize(int width, int height);

void updateCamera(vec3 position);

void setWireframeMode(int enabled);

void draw(); // drawl all object from the scene to screen


void add_object_to_scene(object* obj);

void remove_object_from_scene(object* obj);

void updateObjectModelMat(object* obj);

void setBoundingBox(object* obj);

// box is expected to be an array of 2 vec 3 given in min max order
// the bounding box is epxected to be in model cords
void setKnownBoundingBox(object* obj, vec3* box);

// get the renders camera (useful when hooking up mouse and keyboard functions) (note only yaw and pitch should be modifed directly) 
Camera * getCamera();


void reset_scale_vector(object* obj);