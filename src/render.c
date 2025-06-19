#include<3dTerminal/render.h>
#include<stdio.h>
#include<3dTerminal/datastructures/dynamicArray.h>
#include<3dTerminal/draw.h>
#include<float.h>

/*
	TODO 4/2/25
	replace malloc with _aligned_malloc
	windows wont align past 8 byte alignment which is frustrating since the standard
	said that malloc should align upto 16 bytes :) windows
*/


#define CHARSET 12

#define UP 0.0f , 1.0f , 0.0f
#define FORWARD 0.0f, 0.0f, -1.0f

#define VFOV 90.0f

#define Z_NEAR 0.05f
#define Z_FAR 50.0f

char ascii_set[] = { '@', '#', '%', 'W', 'X', '8', 'o', '+', '=' ,'-',':','.' };

static Screen* screen;
static Camera* camera;
static object* sceneObjects[10];
extern void programExit(unsigned short value);

inline void renderFace_smma_mode_0(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z);
inline void renderFace_smma_mode_1(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z);
inline void renderFace_smma_mode_2(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z);


inline void downSample_smmaa_mode1();
inline void downSample_smaa_mode2();

inline static void ndcToScreen(const vec4 ndc,vec2 cords){
    float width = (float)screen->width;
    float height = (float)screen->height;
	cords[0] = ((ndc[0] + 1.0f) * 0.5f) * (width - 1);
	cords[1] = (1.0-(1.0f + ndc[1]) * 0.5f) * (height - 1);
}
inline static bool outsideScreen(const vec4 ndcPoint){
    return (ndcPoint[0] < -1 || ndcPoint[0] > 1) || (ndcPoint[1] < -1 || ndcPoint[1] > 1);
}
inline static void calculateBayCords(vec2 v0, vec2 v1, vec2 v2, vec3 dest){
    //from http://realtimecollisiondetection.net/
    float d00 = glm_vec2_dot(v0,v0);
    float d01 = glm_vec2_dot(v0,v1);
    float d11 = glm_vec2_dot(v1,v1);
    float d20 = glm_vec2_dot(v2,v0);
    float d21 = glm_vec2_dot(v2,v1);
    float inverse = 1.0f / (d00 * d11 - d01 * d01);
    dest[0] = (d11 * d20 - d01 * d21) * inverse;
    dest[1] = (d00 * d21 - d01 * d20 )  * inverse;
    dest[2] = 1.0f - dest[0] - dest[1]; // the final value is just the complement
}
inline static void calculateNorm(triangle* face, vec3 normal) {
	vec3 U;
	vec3 V;
	glm_vec3_sub(face->v2, face->v1, U);
	glm_vec3_sub(face->v3, face->v1, V);
	glm_vec3_cross(U, V, normal);
	glm_vec3_normalize(normal);
}
// returns true if the sphere intersects or is inside the plane
bool sphereIntersectsPlane(sphere* sphere, vec4 plane) {
	// get the signed distance from the center of the sphere to the plane
	// to do that we just take the plane equation dot it with the sphere center
	// and since the sphere is in homogenous cords we dont need to add d from the plane
	// if the distance from the plane to the sphere is between [-r,r] than sphere intersects
	// if the distances is less than -r than its outside, otherwise the sphere is inside the plane

	float dist = glm_vec4_dot(sphere->center, plane);
	if (dist < -1.0f * sphere->radius) {
		return false;
	}
	return true;
}


void updateScreenSize(int width, int height) {
	screen->width = width;
	screen->height = height;
	camera->projection_perspective.aspect_ratio = (float)width / (float)height;
	glm_perspective(camera->projection_perspective.fov, camera->projection_perspective.aspect_ratio
		, camera->projection_perspective.z_near, camera->projection_perspective.z_far, camera->projection_perspective.project_matrix);
	void * tmp = realloc(screen->pixelBuffer, sizeof(int) * width * height);
	if (!tmp) {
		free(screen->pixelBuffer);
		fprintf(stderr, "OOM in render, during the updating of the screen pixel buffer\n");
		programExit(3);
		return;
	}
	screen->pixelBuffer = (int*)tmp;
	// todo add anti aliasing
	tmp = realloc(screen->drawlingBuffer, sizeof(int) * width * height);
	if (!tmp) {
		free(screen->drawlingBuffer);
		fprintf(stderr, "OOM in render, during the updating of the screen drawling buffer\n");
		programExit(3);
		return;
	}
	screen->drawlingBuffer = (int*)tmp;
	tmp = realloc(screen->z_buffer, sizeof(float) * width * height);
	if (!tmp) {
		free(screen->z_buffer);
		fprintf(stderr, "OOM in render, during the updating of the screen z_buffer buffer\n");
		programExit(3);
		return;
	}
	screen->z_buffer = (float*)tmp;
}

void updateCamera(vec3 position)
{
	if (!camera) {
		fprintf(stderr, "Modification of the camera occurred before initialization of the program\n");
		programExit(3);
		return;
	}
	glm_vec3_copy(position, camera->pos);
	glm_vec3_add(camera->pos, camera->forward, camera->target);
	glm_lookat(camera->pos, camera->target, camera->up, camera->view_matrix);
	glm_mat4_inv(camera->view_matrix, camera->camera_to_world);
}

Screen* createScreen(int width, int height, int smaa_mode)
{
	Screen* screen_ptr = (Screen*) malloc(sizeof(Screen));
	if (!screen_ptr) {
		return NULL;
	}
	screen_ptr->superSampleRate = smaa_mode;
	screen_ptr->height = height;
	screen_ptr->width = width;
	screen_ptr->pixelBuffer = malloc(sizeof(int) * width * height);
	switch (smaa_mode)
	{
		case 0:
			screen_ptr->drawlingBuffer = malloc(sizeof(int) * width * height);
			screen_ptr->z_buffer = malloc(sizeof(float) * width * height);
			break;
		case 1: // 2x2 mode, quater blocks
			screen_ptr->drawlingBuffer = malloc(sizeof(int) * width * height * 2 * 2);
			screen_ptr->z_buffer = malloc(sizeof(float) * width * height * 2 * 2);
			break;
		case 2: // 3x2 mode, braille pattern dots
			screen_ptr->drawlingBuffer = malloc(sizeof(int) * width * height * 3 * 2);
			screen_ptr->z_buffer = malloc(sizeof(float) * width * height * 3 * 2);
			break;
		default:
			screen_ptr->drawlingBuffer = malloc(sizeof(int) * width * height);
			screen_ptr->z_buffer = malloc(sizeof(float) * width * height);
			break;
	}
	return screen_ptr;
}

Camera* createCamera(vec3 origin)
{
	Camera* cam_ptr = (Camera*)_aligned_malloc(sizeof(Camera),16); // 16 byte alignment
	if (!cam_ptr) {
		return NULL;
	}
	cam_ptr->projection_perspective.fov = VFOV;
	cam_ptr->projection_perspective.z_far = Z_FAR;
	cam_ptr->projection_perspective.z_near = Z_NEAR;
	cam_ptr->projection_perspective.aspect_ratio = (float)screen->width / (float)screen->height;
	glm_perspective(cam_ptr->projection_perspective.fov, cam_ptr->projection_perspective.aspect_ratio
		, cam_ptr->projection_perspective.z_near, cam_ptr->projection_perspective.z_far, cam_ptr->projection_perspective.project_matrix);
	// create the camera next
	glm_vec3_copy(origin, cam_ptr->pos);
	glm_vec3_copy((vec3) { FORWARD }, cam_ptr->forward);
	glm_vec3_copy((vec3) { UP }, cam_ptr->up);
	glm_vec3_add(cam_ptr->forward, cam_ptr->pos, cam_ptr->target);
	glm_lookat(cam_ptr->pos, cam_ptr->target,cam_ptr->up, cam_ptr->view_matrix);
	glm_mat4_inv(cam_ptr->view_matrix, cam_ptr->camera_to_world);
	// in degrees
	cam_ptr->pitch = 0.0f; //looking straight in between looking straight up and straight down [-90,90]
	cam_ptr->yaw = 0.0f; // [-180,180] this really doesn't matter since yaw can be periodic without the user noticing
	return cam_ptr;
}


static int size_object_array = 0;





void draw()
{
	//todo anti alliasing
	switch (screen->superSampleRate)
	{
		case 0:
			memset(screen->z_buffer, -10000, sizeof(float) * screen->width * screen->height);
			memset(screen->drawlingBuffer, 0, sizeof(int) * screen->width * screen->height);
			break;
		case 1:
			memset(screen->z_buffer, -10000, sizeof(float) * screen->width * screen->height * 2 * 2);
			memset(screen->drawlingBuffer, 0, sizeof(int) * screen->width * screen->height * 2 * 2);
			break;
		case 2:
			memset(screen->z_buffer, -10000, sizeof(float) * screen->width * screen->height * 3 * 2);
			memset(screen->drawlingBuffer, 0, sizeof(int) * screen->width * screen->height * 3 * 2);
			break;
		default:
			memset(screen->z_buffer, -10000, sizeof(float) * screen->width * screen->height);
			memset(screen->drawlingBuffer, 0, sizeof(int) * screen->width * screen->height);
			break;
	}
	memset(screen->pixelBuffer, 0, sizeof(int) * screen->width * screen->height);
	mat4 viewProj;
	mat4 viewMatrix;
	mat4 rotationMatrix;
    mat4 MV;
	vec3 cameraDirection;

	glm_mat4_identity(rotationMatrix);
	glm_rotate(rotationMatrix, glm_rad(camera->pitch), (vec3) { 1.0f, 0.0f, 0.0f }); // rotate about x-axis
	glm_rotate(rotationMatrix, glm_rad(camera->yaw), (vec3) { 0.0f, 1.0f, 0.0f }); // rotate about y-axis
	glm_mat4_mul(camera->view_matrix, rotationMatrix, viewMatrix);
	glm_mat4_mul(camera->projection_perspective.project_matrix, viewMatrix, viewProj); // read it right to left, proj * view

	// extract view frustum from camera
	vec4 planes[6]; // 6 vec4 one for each plane
	glm_frustum_planes(viewProj, planes); // in world space
	glm_mat4_mulv3(rotationMatrix, camera->forward, 1.0f, cameraDirection); // get rotated camera direction, need for back face culling
    // render loop ie a d
	for (int i = 0; i < size_object_array; i++) {

		vec3 worldBox[2];
		glm_aabb_transform(sceneObjects[i]->boundingBox, sceneObjects[i]->modelMat, worldBox);
		bool is_in = glm_aabb_frustum(worldBox, planes);
		if (!is_in) {
			// frustum culled, in theory this does not check against the corners of the plane
			continue;
		}
        glm_mat4_mul(viewMatrix,sceneObjects[i]->modelMat,MV);
		vec4* transformed_verts = (vec4*)_aligned_malloc(sizeof(vec4) * sceneObjects[i]->meshSize,16);
        dynamicFaceArray face_array;
        dynamicFaceArray_create(&face_array);
		if (!transformed_verts)
		{
			fprintf(stderr, "OOM, inside render loop, tried to allocate memory of size: %zu", sizeof(vec4) * sceneObjects[i]->meshSize);
			programExit(3);
			return;
		}
        //backface culling and then at some point clipping
		{
			vec4 homogenous_cords;
			homogenous_cords[3] = 1.0f; // set w to 1.0
			for (int vert = 0; vert < sceneObjects[i]->meshSize; vert++) {
				glm_vec3_copy(sceneObjects[i]->mesh[vert], homogenous_cords); // copy x,y,z values over
				glm_mat4_mulv(MV, homogenous_cords, transformed_verts[vert]);
			}
			vec3 face_vec;
			for (int face = 0; face < sceneObjects[i]->faceListLength; face++) {
				glm_vec3_copy(sceneObjects[i]->faceList[face], face_vec);
				// do back face culling here
				vec3 norm;
				triangle triangle;
				glm_vec3_copy(transformed_verts[(int)face_vec[0]], triangle.v1);
				glm_vec3_copy(transformed_verts[(int)face_vec[1]], triangle.v2);
				glm_vec3_copy(transformed_verts[(int)face_vec[2]], triangle.v3);
                calculateNorm(&triangle, norm);
				// do back face calc here 
				float value = glm_vec3_dot(cameraDirection, norm);
				if (value > 0.0f) {
					// backface cull, disregard face
					//continue;
				}
                // copies the address of the face into an array for later rendering
                dynamicFaceArray_add(&face_array,&(sceneObjects[i]->faceList[face]));
			}
            // about here implement clipping to clip triangles that arnt fully in view frustum
		}
        // drawling process
		// continue drawling process
		// 3. after all, the above, take the subset of faces and continue draw process
		// 4. use 2d bounding box and check points using barycentric cords
        for(int face_index = 0; face_index < face_array.size; face_index++){
            vec4 a;
            vec4 b;
            vec4 c;
            vec3 * face = face_array.data[face_index];
            float a_z, b_z, c_z;
			glm_vec4_copy(transformed_verts[(int)((*face)[0])], a);
			glm_vec4_copy(transformed_verts[(int)((*face)[1])], b);
			glm_vec4_copy(transformed_verts[(int)((*face)[2])], c);
            a_z = a[2];
            b_z = b[2];
            c_z = c[2];
            if(min(min(a[2],b[2]),c[2]) > -Z_NEAR){
                continue; // discard for now, fixed by clipping
            }
            // these vectors are in view space
            glm_mat4_mulv(camera->projection_perspective.project_matrix,a,a);
            glm_mat4_mulv(camera->projection_perspective.project_matrix,b,b);
            glm_mat4_mulv(camera->projection_perspective.project_matrix,c,c);
            //perspective divide
            glm_vec4_scale(a,1.0/a[3],a);
            glm_vec4_scale(b,1.0/b[3],b);
            glm_vec4_scale(c,1.0/c[3],c);
            vec2 a_prime;
            vec2 b_prime;
            vec2 c_prime;
            ndcToScreen(a,a_prime);
            ndcToScreen(b,b_prime);
            ndcToScreen(c,c_prime);
            int start_x = floor(max(0.0,min(a_prime[0],min(b_prime[0],c_prime[0]))));
			int end_x = (int)ceilf(min((float)(screen->width-1), max(a_prime[0], max(b_prime[0], c_prime[0]))));
            int start_y = floor(max(0.0,min(a_prime[1],min(b_prime[1],c_prime[1]))));
			int end_y = (int)ceilf(min((float)(screen->height-1), max(a_prime[1], max(b_prime[1], c_prime[1]))));
            vec2 v0;
            vec2 v1;
            glm_vec2_sub(b_prime,a_prime,v0);
            glm_vec2_sub(c_prime,a_prime,v1);
			switch (screen->superSampleRate)
			{
			case 0:
				renderFace_smma_mode_0(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
				break;
			case 1:
				renderFace_smma_mode_1(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
				break;
			case 2:
				renderFace_smma_mode_2(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
				break;
			default:
				renderFace_smma_mode_0(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
				break;
			}
        }
        _aligned_free(transformed_verts);
        dynamicFaceArray_destroy(&face_array);
	}
	// todo anti allaising, down sampler code
	// copy data over to pixel buffer
	for (int y_inter = 0; y_inter < screen->height; y_inter++) {
		for (int x_iter = 0; x_iter < screen->width; x_iter++) {
			screen->pixelBuffer[(y_inter)*screen->width + x_iter] = screen->drawlingBuffer[y_inter * screen->width + x_iter];
			
		}
	}
	draw_to_screen(screen->pixelBuffer, 0, 0, screen->width, screen->height);
}

inline void rendermodeSuperSample1(vec2 v0, vec2 v1, vec2 a_prime, vec2 initPoint, float a_z, float b_z, float c_z) {
	static float step = 0.50;
	static float center = 0.25;
	static vec3 bayValues;
	vec2 point_cpy;
	vec2 v2;
	int x = (int)initPoint[0];
	int y = (int)initPoint[1];
	glm_vec2_zero(v2);
	glm_vec2_copy(initPoint, point_cpy);
	for (int dx = 0; dx < 2; dx++) {
		for (int dy = 0; dy < 2; dy++) {
			point_cpy[0] += (step * dx) + center;
			point_cpy[1] += (step * dy) + center;
			glm_vec2_sub(point_cpy, a_prime, v2);
			calculateBayCords(v0, v1, v2, bayValues);
			if (bayValues[0] < 0 || bayValues[1] < 0 || bayValues[2] < 0) {
				point_cpy[0] -= center;
				point_cpy[1] -= center;
				continue; // not in triangle
			}
			float z_interp = bayValues[0] * a_z + bayValues[1] * b_z + bayValues[2] * c_z;
			float z_buffer_val = screen->z_buffer[(y * 2 + dy) * (screen->width*2) + x * 2 + dx];
			// todo check z interp value with buffer and overwrite if nesscary
			// also then overwrite value in the pixel buffer
			if (z_interp > z_buffer_val) {
				screen->z_buffer[(y * 2 + dy) * (screen->width * 2) + x * 2 + dx] = z_interp;
				screen->drawlingBuffer[(y * 2 + dy) * (screen->width * 2) + x * 2 + dx] = 1; // 1 means occupied
			}
			point_cpy[0] -= center;
			point_cpy[1] -= center;
		}
	}
}
inline void rendermodeSuperSample2(vec2 v0, vec2 v1, vec2 a_prime, vec2 initPoint, float a_z, float b_z, float c_z) {
	static float x_step = 0.50;
	static float y_step = 1.0 / 3.0;
	static float x_center = 0.25;
	static float y_center = 1.0 / 6.0;
	static vec3 bayValues;
	for (int dx = 0; dx < 2; dx++) {
		for (int dy = 0; dy < 3; dy++) {
			// todo finish braille code
		}
	}
}

inline void renderFace_smma_mode_0(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z) {
	for (int x = start_x; x <= end_x; x++) {
		for (int y = start_y; y <= end_y; y++) {
			// need to get baycords
			vec2 v2;
			vec2 point;
			vec3 bayValues;
			point[0] = (float)x;
			point[1] = (float)y;
			glm_vec2_sub(point, a_prime, v2);
			calculateBayCords(v0, v1, v2, bayValues);
			if (bayValues[0] < 0 || bayValues[1] < 0 || bayValues[2] < 0) {
				continue; // not in triangle
			}
			float z_interp = bayValues[0] * a_z + bayValues[1] * b_z + bayValues[2] * c_z;
			float z_buffer_val = screen->z_buffer[y * screen->width + x];
			// do z_buffering here, the z value here should be negative since from the camera perspective
			// - z values are in front of it
			if (z_interp > z_buffer_val) {
				screen->z_buffer[y * screen->width + x] = z_interp;
				screen->drawlingBuffer[y * screen->width + x] = (int)' ';
			}
		}
	}
}
inline void renderFace_smma_mode_1(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z) {
	for (int x = start_x; x <= end_x; x++) {
		for (int y = start_y; y <= end_y; y++) {
			vec2 point;
			point[0] = (float)x;
			point[1] = (float)y;
			rendermodeSuperSample1(v0, v1, a_prime, point, a_z, b_z, c_z);
		}
	}
}
inline void renderFace_smma_mode_2(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z) {
	for (int x = start_x; x <= end_x; x++) {
		for (int y = start_y; y <= end_y; y++) {
			vec2 point;
			point[0] = (float)x;
			point[1] = (float)y;
			rendermodeSuperSample2(v0, v1, a_prime, point, a_z, b_z, c_z);
		}
	}
}

inline void downSample_smmaa_mode1()
{

}

inline void downSample_smaa_mode2()
{

}


void initRenderCode(int width, int height,int smaa_mode)
{
	
	screen = createScreen(width, height,smaa_mode);
	if (!screen) {
		fprintf(stderr, "OOM inside render start up code\n");
		programExit(3);
	}
	vec3 origin;
	glm_vec3_zero(origin);
	camera = createCamera(origin);
	if (!camera) {
		fprintf(stderr, "OOM inside render start up code\n");
		programExit(3);
	}
}

void add_object_to_scene(object* obj) {
	static int index;
	if (size_object_array < 10) {
        sceneObjects[index] = obj;
		index += 1;
	}
	size_object_array += 1;
}

void remove_object_from_scene(object* obj) {
	int indexer = 0;
	for (; indexer < size_object_array; indexer++) {
		if (obj == sceneObjects[indexer]) {
            sceneObjects[indexer] = NULL;
			size_object_array -= 1;
			break;
		}
	}
	if (indexer < size_object_array) {
		//shift left
		for (; indexer < 9; indexer++) {
            sceneObjects[indexer] = sceneObjects[indexer + 1];
			indexer++;
		}
        sceneObjects[indexer] = NULL;
	}
}

void updateObjectModelMat(object* obj)
{
	mat4 model;
	// right to left, so this is the last matrix
	glm_translate_make(model, obj->postion);
	// note that currently this assumes that the object is centered
	// around the origin, in the future calculating a bound volume surface may be a better idea
	// i.e., rotate around the center of the bounding shape
	glm_rotate_z(model, glm_rad(obj->z_rotation), model);
	glm_rotate_y(model, glm_rad(obj->y_rotation), model);
	glm_rotate_x(model, glm_rad(obj->x_rotation), model);
	glm_mat4_copy(model, obj->modelMat);
}

void setBoundingBox(object* obj)
{
	if (obj->meshSize == 0) {
		return;
	}
	vec3 min;
	vec3 max;
	glm_vec3_zero(min);
	glm_vec3_zero(max);
	glm_vec3_copy(obj->mesh[0], min);
	glm_vec3_copy(obj->mesh[0], max);
	for (int i = 1; i < obj->meshSize; i++) {
		if (obj->mesh[i][0] < min[0]) {
			min[0] = obj->mesh[i][0];
		}
		if (obj->mesh[i][1] < min[1]) {
			min[1] = obj->mesh[i][1];
		}
		if (obj->mesh[i][2] < min[2]) {
			min[2] = obj->mesh[i][2];
		}
		if (obj->mesh[i][0] > max[0]) {
			max[0] = obj->mesh[i][0];
		}
		if (obj->mesh[i][1] > max[1]) {
			max[1] = obj->mesh[i][1];
		}
		if (obj->mesh[i][2] > max[2]) {
			max[2] = obj->mesh[i][2];
		}
	}
	glm_vec3_copy(min, obj->boundingBox[0]);
	glm_vec3_copy(max, obj->boundingBox[1]);
}

void setKnownBoundingBox(object* obj, vec3* box)
{
	glm_vec3_copy(box[0], obj->boundingBox[0]);
	glm_vec3_copy(box[1], obj->boundingBox[1]);
}