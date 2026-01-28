#include<3dTerminal/render.h>
#include<stdio.h>
#include<3dTerminal/datastructures/dynamicArray.h>
#include<3dTerminal/draw.h>
#include<float.h>
#include<math.h>


#define CHARSET 12

#define UP 0.0f , 1.0f , 0.0f
#define FORWARD 0.0f, 0.0f, -1.0f

#define VFOV 120.0f

#define Z_NEAR 0.05f
#define Z_FAR 500.0f

#define DEFAULT_COLOR_VEC 255, 255, 255
#define DEFAULT_COLOR (1 << 24) - 1

#define Z_MAX_DEPTH -1000000.0f



typedef struct TriBaryCache_t {
	float d00, d01, d11;
	float invDenom;
}TriBaryCache;


static inline void compute_bary_cache(vec2 v0, vec2 v1, TriBaryCache* dest) {
	dest->d00 = glm_vec2_dot(v0, v0);
	dest->d01 = glm_vec2_dot(v0, v1);
	dest->d11 = glm_vec2_dot(v1, v1);
	dest->invDenom = 1.0f / (dest->d00 * dest->d11 - dest->d01 * dest->d01);
}

static Screen* screen;
static Camera* camera;
static object* sceneObjects[10];
static cellInfo* cellUnicodeArray;
static int wireframe_mode = 0;
extern void programExit(unsigned short value);

inline void renderFace_smaa_mode_0(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z);
inline void renderFace_smaa_mode_1(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z);
inline void renderFace_smaa_mode_2(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z);
inline void renderFace_smaa_mode_1_with_vertex_colors(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z, ivec3 c_a, ivec3 c_b, ivec3 c_c);
inline void renderFace_smaa_mode_2_with_vertex_colors(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z, ivec3 c_a, ivec3 c_b, ivec3 c_c);
inline void wireframeRender(vec2 a_prime, vec2 b_prime, vec2 c_prime, float a_z, float b_z, float c_z);


// camera methods 
inline void update_camera_vectors();


inline void downSample_smaa_mode1();
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

inline static void calculateBayCords_cache(vec2 v0, vec2 v1, vec2 v2, TriBaryCache * cache , vec3 dest) {
	float d20 = glm_vec2_dot(v2, v0);
	float d21 = glm_vec2_dot(v2, v1);
	dest[0] = (cache->d11 * d20 - cache->d01 * d21) * cache->invDenom;
	dest[1] = (cache->d00 * d21 - cache->d01 * d20) * cache->invDenom;
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
	void* tmp = realloc(screen->pixelBuffer, sizeof(int) * width * height);
	if (!tmp) {
		free(screen->pixelBuffer);
		fprintf(stderr, "OOM in render, during the updating of the screen pixel buffer\n");
		programExit(3);
		return;
	}
	screen->pixelBuffer = (int*)tmp;
	// todo add anti aliasing
	switch (screen->superSampleRate) {
	case 0:
		tmp = realloc(screen->drawlingBuffer, sizeof(int) * width * height);
		break;
	case 1:
		tmp = realloc(screen->drawlingBuffer, sizeof(int) * width * height * 4);
		break;
	case 2:
		tmp = realloc(screen->drawlingBuffer, sizeof(int) * width * height * 8);
		break;
	default:
		tmp = realloc(screen->drawlingBuffer, sizeof(int) * width * height);
		break;
	}
	if (!tmp) {
		free(screen->drawlingBuffer);
		fprintf(stderr, "OOM in render, during the updating of the screen drawling buffer\n");
		programExit(3);
		return;
	}
	screen->drawlingBuffer = (int*)tmp;
	switch (screen->superSampleRate) {
	case 0:
		tmp = realloc(screen->z_buffer, sizeof(float) * width * height);
		break;
	case 1:
		tmp = realloc(screen->z_buffer, sizeof(float) * width * height * 4);
		break;
	case 2:
		tmp = realloc(screen->z_buffer, sizeof(float) * width * height * 8);
		break;
	default:
		tmp = realloc(screen->z_buffer, sizeof(float) * width * height);
		break;
	}
	if (!tmp) {
		free(screen->z_buffer);
		fprintf(stderr, "OOM in render, during the updating of the screen z_buffer buffer\n");
		programExit(3);
		return;
	}
	screen->z_buffer = (float*)tmp;
	if (!screen->superSampleRate) {
		return;
	}
	tmp = realloc(cellUnicodeArray, sizeof(cellInfo) * width * height);
	if (!tmp) {
		free(cellUnicodeArray);
		fprintf(stderr, "OOM in render, during the updating of cellInfo Unicode array \n");
		programExit(3);
		return;
	}
	cellUnicodeArray = (cellInfo*)tmp;
}

void updateCamera(vec3 position)
{
	if (!camera) {
		fprintf(stderr, "Modification of the camera occurred before initialization of the program\n");
		programExit(3);
		return;
	}
	// old code fixed via moving lookat per frame
	//glm_vec3_copy(position, camera->pos);
	//glm_vec3_add(camera->pos, camera->forward, camera->target);
	//glm_lookat(camera->pos, camera->target, camera->up, camera->view_matrix);
	//glm_mat4_inv(camera->view_matrix, camera->camera_to_world);

	glm_vec3_copy(position, camera->pos);
}

void setWireframeMode(int enabled)
{
	wireframe_mode = enabled ? 1 : 0;
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
		case 2: // 4x2 mode, braille pattern dots
			screen_ptr->drawlingBuffer = malloc(sizeof(int) * width * height * 4 * 2);
			screen_ptr->z_buffer = malloc(sizeof(float) * width * height * 4 * 2);
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
	cam_ptr->last_pitch = -1.0;
	cam_ptr->last_yaw = -1.0;

	//  old camera system code
	//glm_lookat(cam_ptr->pos, cam_ptr->target,cam_ptr->up, cam_ptr->view_matrix);
	//glm_mat4_inv(cam_ptr->view_matrix, cam_ptr->camera_to_world);
	

	// in degrees
	cam_ptr->pitch = 0.0f; //looking straight in between looking straight up and straight down [-90,90]
	cam_ptr->yaw = -90.0f; // [-180,180] this really doesn't matter since yaw can be periodic without the user noticing
	return cam_ptr;
}


static int size_object_array = 0;

inline void render_faces(vec4 * transformed_verts, dynamicFaceArray* faces_array) {
	// drawling process
	// continue drawling process
	// 3. after all, the above, take the subset of faces and continue draw process
	// 4. use 2d bounding box and check points using barycentric cords
	dynamicFaceArray face_array = *faces_array;
	for (int face_index = 0; face_index < face_array.size; face_index++) {
		vec4 a;
		vec4 b;
		vec4 c;
		ivec3* face = face_array.data[face_index];
		float a_z, b_z, c_z;
		glm_vec4_copy(transformed_verts[(*face)[0]], a);
		glm_vec4_copy(transformed_verts[(*face)[1]], b);
		glm_vec4_copy(transformed_verts[(*face)[2]], c);
		a_z = a[2];
		b_z = b[2];
		c_z = c[2];
		if (min(min(a[2], b[2]), c[2]) > -Z_NEAR) {
			continue; // discard for now, fixed by clipping
		}
		// these vectors are in view space
		glm_mat4_mulv(camera->projection_perspective.project_matrix, a, a);
		glm_mat4_mulv(camera->projection_perspective.project_matrix, b, b);
		glm_mat4_mulv(camera->projection_perspective.project_matrix, c, c);
		//perspective divide
		glm_vec4_scale(a, 1.0 / a[3], a);
		glm_vec4_scale(b, 1.0 / b[3], b);
		glm_vec4_scale(c, 1.0 / c[3], c);
		vec2 a_prime;
		vec2 b_prime;
		vec2 c_prime;
		ndcToScreen(a, a_prime);
		ndcToScreen(b, b_prime);
		ndcToScreen(c, c_prime);
		int start_x = floor(max(0.0, min(a_prime[0], min(b_prime[0], c_prime[0]))));
		int end_x = (int)ceilf(min((float)(screen->width - 1), max(a_prime[0], max(b_prime[0], c_prime[0]))));
		int start_y = floor(max(0.0, min(a_prime[1], min(b_prime[1], c_prime[1]))));
		int end_y = (int)ceilf(min((float)(screen->height - 1), max(a_prime[1], max(b_prime[1], c_prime[1]))));
		vec2 v0;
		vec2 v1;
		glm_vec2_sub(b_prime, a_prime, v0);
		glm_vec2_sub(c_prime, a_prime, v1);
		if (wireframe_mode) {
			wireframeRender(a_prime, b_prime, c_prime, a_z, b_z, c_z);
			continue;
		}
		switch (screen->superSampleRate)
		{
		case 0:
			renderFace_smaa_mode_0(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
			break;
		case 1:
			renderFace_smaa_mode_1(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
			break;
		case 2:
			renderFace_smaa_mode_2(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
			break;
		default:
			renderFace_smaa_mode_0(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
			break;
		}
	}
	_aligned_free(transformed_verts);
	dynamicFaceArray_destroy(&face_array);
}

inline void render_faces_with_vertex_colors(vec4* transformed_verts, dynamicFaceArray* faces_array, object * obj) {
	// drawling process
	// continue drawling process
	// 3. after all, the above, take the subset of faces and continue draw process
	// 4. use 2d bounding box and check points using barycentric cords
	dynamicFaceArray face_array = *faces_array;
	for (int face_index = 0; face_index < face_array.size; face_index++) {
		vec4 a;
		vec4 b;
		vec4 c;
		ivec3 c_a;
		ivec3 c_b;
		ivec3 c_c;
		ivec3* face = face_array.data[face_index];
		float a_z, b_z, c_z;
		glm_vec4_copy(transformed_verts[(*face)[0]], a);
		glm_vec4_copy(transformed_verts[(*face)[1]], b);
		glm_vec4_copy(transformed_verts[(*face)[2]], c);
		// copy colors over
		glm_ivec3_copy(obj->vertex_color[(*face)[0]], c_a);
		glm_ivec3_copy(obj->vertex_color[(*face)[1]], c_b);
		glm_ivec3_copy(obj->vertex_color[(*face)[2]], c_c);
		a_z = a[2];
		b_z = b[2];
		c_z = c[2];
		if (max(max(a[2], b[2]), c[2]) > -Z_NEAR) { //swaped min with max (needed since we dont do clipping, thus producing projection issues) (if cliping is added this should be swaped to min instead of max)
			continue; // discard for now, fixed by clipping
		}
		// these vectors are in view space
		glm_mat4_mulv(camera->projection_perspective.project_matrix, a, a);
		glm_mat4_mulv(camera->projection_perspective.project_matrix, b, b);
		glm_mat4_mulv(camera->projection_perspective.project_matrix, c, c);
		//perspective divide
		glm_vec4_scale(a, 1.0 / a[3], a);
		glm_vec4_scale(b, 1.0 / b[3], b);
		glm_vec4_scale(c, 1.0 / c[3], c);
		vec2 a_prime;
		vec2 b_prime;
		vec2 c_prime;
		ndcToScreen(a, a_prime);
		ndcToScreen(b, b_prime);
		ndcToScreen(c, c_prime);
		int start_x = floor(max(0.0, min(a_prime[0], min(b_prime[0], c_prime[0]))));
		int end_x = (int)ceilf(min((float)(screen->width - 1), max(a_prime[0], max(b_prime[0], c_prime[0]))));
		int start_y = floor(max(0.0, min(a_prime[1], min(b_prime[1], c_prime[1]))));
		int end_y = (int)ceilf(min((float)(screen->height - 1), max(a_prime[1], max(b_prime[1], c_prime[1]))));
		vec2 v0;
		vec2 v1;
		glm_vec2_sub(b_prime, a_prime, v0);
		glm_vec2_sub(c_prime, a_prime, v1);
		if (wireframe_mode) {
			wireframeRender(a_prime, b_prime, c_prime, a_z, b_z, c_z);
			continue;
		}
		switch (screen->superSampleRate)
		{
		case 0:
			renderFace_smaa_mode_0(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
			break;
		case 1:
			renderFace_smaa_mode_1_with_vertex_colors(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z, c_a, c_b, c_c);
			break;
		case 2:
			renderFace_smaa_mode_2_with_vertex_colors(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z, c_a, c_b, c_c);
			break;
		default:
			renderFace_smaa_mode_0(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
			break;
		}
	}
	_aligned_free(transformed_verts);
	dynamicFaceArray_destroy(&face_array);
}


void draw()
{
	switch (screen->superSampleRate)
	{
		case 0:
			for (int i = 0; i < screen->width * screen->height; i++) {
				screen->z_buffer[i] = Z_MAX_DEPTH;
			}
			/*memset(screen->z_buffer, Z_MAX_DEPTH, sizeof(float) * screen->width * screen->height);*/
			memset(screen->drawlingBuffer, -1, sizeof(int) * screen->width * screen->height);
			break;
		case 1:
			for (int i = 0; i < screen->width * screen->height * 2 * 2; i++) {
				screen->z_buffer[i] = Z_MAX_DEPTH;
			}
			/*memset(screen->z_buffer, Z_MAX_DEPTH, sizeof(float) * screen->width * screen->height * 2 * 2);*/
			memset(screen->drawlingBuffer, 0, sizeof(int) * screen->width * screen->height * 2 * 2);
			break;
		case 2:
			for (int i = 0; i < screen->width * screen->height * 4 * 2; i++) {
				screen->z_buffer[i] = Z_MAX_DEPTH;
			}
			/*memset(screen->z_buffer, Z_MAX_DEPTH, sizeof(float) * screen->width * screen->height * 3 * 2);*/
			memset(screen->drawlingBuffer, 0, sizeof(int) * screen->width * screen->height * 4 * 2);
			break;
		default:
			for (int i = 0; i < screen->width * screen->height; i++) {
				screen->z_buffer[i] = Z_MAX_DEPTH;
			}
			/*memset(screen->z_buffer, Z_MAX_DEPTH, sizeof(float) * screen->width * screen->height);*/
			memset(screen->drawlingBuffer, -1, sizeof(int) * screen->width * screen->height);
			break;
	}
	memset(screen->pixelBuffer, 0, sizeof(int) * screen->width * screen->height);
	mat4 viewProj;
	mat4 viewMatrix;
	//mat4 rotationMatrix;
    mat4 MV;
	//vec3 cameraDirection;


	update_camera_vectors();

	//glm_mat4_identity(rotationMatrix);
	//glm_rotate(rotationMatrix, glm_rad(camera->pitch), (vec3) { 1.0f, 0.0f, 0.0f }); // rotate about x-axis
	//glm_rotate(rotationMatrix, glm_rad(camera->yaw), (vec3) { 0.0f, 1.0f, 0.0f }); // rotate about y-axis
	//glm_mat4_mul(camera->view_matrix, rotationMatrix, viewMatrix);
	glm_vec3_add(camera->pos, camera->forward, camera->target);
	glm_lookat(camera->pos, camera->target, camera->up, viewMatrix);
	glm_mat4_mul(camera->projection_perspective.project_matrix, viewMatrix, viewProj); // read it right to left, proj * view

	// extract view frustum from camera
	vec4 planes[6]; // 6 vec4 one for each plane
	glm_frustum_planes(viewProj, planes); // in world space
	//glm_mat4_mulv3(rotationMatrix, camera->forward, 1.0f, cameraDirection); // get rotated camera direction, need for back face culling // removed due to camera system update
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
			ivec3 face_vec;
			for (int face = 0; face < sceneObjects[i]->faceListLength; face++) {
				glm_ivec3_copy(sceneObjects[i]->faceList[face], face_vec);
				// do back face culling here
				vec3 norm;
				triangle triangle;
				glm_vec3_copy(transformed_verts[(int)face_vec[0]], triangle.v1);
				glm_vec3_copy(transformed_verts[(int)face_vec[1]], triangle.v2);
				glm_vec3_copy(transformed_verts[(int)face_vec[2]], triangle.v3);
                calculateNorm(&triangle, norm);
				#ifdef BACKFACE_CULL_ENABLED
					// do back face calc here 
					float value = glm_vec3_dot((vec3) { FORWARD }, norm);
					if (value > 0.0f) {
						// backface cull, disregard face
						continue;
					}
				#endif // BACKFACE_CULL_ENABLED
                // copies the address of the face into an array for later rendering
                dynamicFaceArray_add(&face_array,&(sceneObjects[i]->faceList[face]));
			}
            // about here implement clipping to clip triangles that arnt fully in view frustum
		}
		if (sceneObjects[i]->has_vertex_color > 0) {
			render_faces_with_vertex_colors(transformed_verts, &face_array, sceneObjects[i]);
		}
		else {
			render_faces(transformed_verts, &face_array);
		}
		// depercated by split function for those with vertex colors and those without
  //      // drawling process
		//// continue drawling process
		//// 3. after all, the above, take the subset of faces and continue draw process
		//// 4. use 2d bounding box and check points using barycentric cords
  //      for(int face_index = 0; face_index < face_array.size; face_index++){
  //          vec4 a;
  //          vec4 b;
  //          vec4 c;
  //          ivec3 * face = face_array.data[face_index];
  //          float a_z, b_z, c_z;
		//	glm_vec4_copy(transformed_verts[(*face)[0]], a);
		//	glm_vec4_copy(transformed_verts[(*face)[1]], b);
		//	glm_vec4_copy(transformed_verts[(*face)[2]], c);
  //          a_z = a[2];
  //          b_z = b[2];
  //          c_z = c[2];
  //          if(min(min(a[2],b[2]),c[2]) > -Z_NEAR){
  //              continue; // discard for now, fixed by clipping
  //          }
  //          // these vectors are in view space
  //          glm_mat4_mulv(camera->projection_perspective.project_matrix,a,a);
  //          glm_mat4_mulv(camera->projection_perspective.project_matrix,b,b);
  //          glm_mat4_mulv(camera->projection_perspective.project_matrix,c,c);
  //          //perspective divide
  //          glm_vec4_scale(a,1.0/a[3],a);
  //          glm_vec4_scale(b,1.0/b[3],b);
  //          glm_vec4_scale(c,1.0/c[3],c);
  //          vec2 a_prime;
  //          vec2 b_prime;
  //          vec2 c_prime;
  //          ndcToScreen(a,a_prime);
  //          ndcToScreen(b,b_prime);
  //          ndcToScreen(c,c_prime);
  //          int start_x = floor(max(0.0,min(a_prime[0],min(b_prime[0],c_prime[0]))));
		//	int end_x = (int)ceilf(min((float)(screen->width-1), max(a_prime[0], max(b_prime[0], c_prime[0]))));
  //          int start_y = floor(max(0.0,min(a_prime[1],min(b_prime[1],c_prime[1]))));
		//	int end_y = (int)ceilf(min((float)(screen->height-1), max(a_prime[1], max(b_prime[1], c_prime[1]))));
  //          vec2 v0;
  //          vec2 v1;
  //          glm_vec2_sub(b_prime,a_prime,v0);
  //          glm_vec2_sub(c_prime,a_prime,v1);
		//	if (wireframe_mode) {
		//		wireframeRender(a_prime, b_prime, c_prime, a_z, b_z, c_z);
		//		continue;
		//	}
		//	switch (screen->superSampleRate)
		//	{
		//	case 0:
		//		renderFace_smaa_mode_0(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
		//		break;
		//	case 1:
		//		renderFace_smaa_mode_1(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
		//		break;
		//	case 2:
		//		renderFace_smaa_mode_2(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
		//		break;
		//	default:
		//		renderFace_smaa_mode_0(start_x, end_x, start_y, end_y, v0, v1, a_prime, a_z, b_z, c_z);
		//		break;
		//	}
  //      }
  //      _aligned_free(transformed_verts);
  //      dynamicFaceArray_destroy(&face_array);
	}
	// flush information to screen
	if (!screen->superSampleRate) {
		// copy data over to pixel buffer
		for (int y_inter = 0; y_inter < screen->height; y_inter++) {
			for (int x_iter = 0; x_iter < screen->width; x_iter++) {
				screen->pixelBuffer[(y_inter)*screen->width + x_iter] = screen->drawlingBuffer[y_inter * screen->width + x_iter];
			}
		}
		draw_to_screen(screen->pixelBuffer, 0, 0, screen->width, screen->height);
	}
	else {
		if (screen->superSampleRate == 1) {
			downSample_smaa_mode1();
		}
		else {
			downSample_smaa_mode2();
		}
		draw_to_screen_unicode(cellUnicodeArray, screen->width, screen->height, screen->superSampleRate == 1 ? quater_block : braille);
	}
}


inline void rendermodeSuperSample1(vec2 v0, vec2 v1, vec2 a_prime, vec2 initPoint, float a_z, float b_z, float c_z, TriBaryCache * cache) {
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
			point_cpy[0] = initPoint[0] + (step * dx) + center;
			point_cpy[1] = initPoint[1] + (step * dy) + center;
			glm_vec2_sub(point_cpy, a_prime, v2);
			/*calculateBayCords(v0, v1, v2, bayValues);*/
			calculateBayCords_cache(v0, v1, v2, cache, bayValues);
			int inside =
				(bayValues[0] >= 0.0f) &
				(bayValues[1] >= 0.0f) &
				(bayValues[2] >= 0.0f);
			if (!inside) continue;
			float z_interp = bayValues[0] * a_z + bayValues[1] * b_z + bayValues[2] * c_z;
			float z_buffer_val = screen->z_buffer[(y * 2 + dy) * (screen->width*2) + x * 2 + dx];
			// todo check z interp value with buffer and overwrite if nesscary
			// also then overwrite value in the pixel buffer
			if (z_interp > z_buffer_val) {
				screen->z_buffer[(y * 2 + dy) * (screen->width * 2) + x * 2 + dx] = z_interp;
				screen->drawlingBuffer[(y * 2 + dy) * (screen->width * 2) + x * 2 + dx] = DEFAULT_COLOR;
			}
		}
	}
}

inline void rendermodeSuperSample1_with_vertex_colors(vec2 v0, vec2 v1, vec2 a_prime, vec2 initPoint, float a_z, float b_z, float c_z, ivec3 c_a, ivec3 c_b, ivec3 c_c, TriBaryCache * cache) {
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
			point_cpy[0] = initPoint[0] + (step * dx) + center;
			point_cpy[1] = initPoint[1] + (step * dy) + center;
			glm_vec2_sub(point_cpy, a_prime, v2);
			/*calculateBayCords(v0, v1, v2, bayValues);*/
			calculateBayCords_cache(v0, v1, v2, cache, bayValues);
			int inside =
				(bayValues[0] >= 0.0f) &
				(bayValues[1] >= 0.0f) &
				(bayValues[2] >= 0.0f);
			if (!inside) continue;
			float z_interp = bayValues[0] * a_z + bayValues[1] * b_z + bayValues[2] * c_z;
			vec3 c_a_scaled;
			vec3 c_b_scaled;
			vec3 c_c_scaled;

			glm_vec3_copy((vec3){c_a[0],c_a[1],c_a[2]}, c_a_scaled);
			glm_vec3_copy((vec3) { c_b[0], c_b[1], c_b[2] }, c_b_scaled);
			glm_vec3_copy((vec3) { c_c[0], c_c[1], c_c[2] }, c_c_scaled);

			glm_vec3_scale(c_a_scaled, bayValues[0], c_a_scaled);
			glm_vec3_scale(c_b_scaled, bayValues[1], c_b_scaled);
			glm_vec3_scale(c_c_scaled, bayValues[2], c_c_scaled);

			// add all colors into c_c_scaled
			glm_vec3_addadd(c_a_scaled, c_b_scaled, c_c_scaled);

			int color = 0;
			// set R G B channels
			color ^= ((int)(c_c_scaled[0])) << 16;
			color ^= ((int)(c_c_scaled[1])) << 8;
			color ^= ((int)(c_c_scaled[2]));

			float z_buffer_val = screen->z_buffer[(y * 2 + dy) * (screen->width * 2) + x * 2 + dx];
			// todo check z interp value with buffer and overwrite if nesscary
			// also then overwrite value in the pixel buffer
			if (z_interp > z_buffer_val) {
				screen->z_buffer[(y * 2 + dy) * (screen->width * 2) + x * 2 + dx] = z_interp;
				screen->drawlingBuffer[(y * 2 + dy) * (screen->width * 2) + x * 2 + dx] = color;
			}
		}
	}
}

inline void rendermodeSuperSample2(vec2 v0, vec2 v1, vec2 a_prime, vec2 initPoint, float a_z, float b_z, float c_z, TriBaryCache * cache) {
	static float x_step = 0.50;
	static float y_step = 1.0 / 4.0;
	static float x_center = 0.25;
	static float y_center = 1.0 / 8.0;
	static vec3 bayValues;
	vec2 point_cpy;
	vec2 v2;
	int x = (int)initPoint[0];
	int y = (int)initPoint[1];
	glm_vec2_zero(v2);
	glm_vec2_copy(initPoint, point_cpy);
	for (int dx = 0; dx < 2; dx++) {
		for (int dy = 0; dy < 4; dy++) {
			// todo finish braille code
			point_cpy[0] = initPoint[0] + (x_step * dx) + x_center;
			point_cpy[1] = initPoint[1] + (y_step * dy) + y_center;
			glm_vec2_sub(point_cpy, a_prime, v2);
			/*calculateBayCords(v0, v1, v2, bayValues);*/
			calculateBayCords_cache(v0, v1, v2, cache, bayValues);
			int inside =
				(bayValues[0] >= 0.0f) &
				(bayValues[1] >= 0.0f) &
				(bayValues[2] >= 0.0f);
			if (!inside) continue;
			float z_interp = bayValues[0] * a_z + bayValues[1] * b_z + bayValues[2] * c_z;
			float z_buffer_val = screen->z_buffer[(y * 4 + dy) * (screen->width * 2) + x * 2 + dx];
			if (z_interp > z_buffer_val) {
				screen->z_buffer[(y * 4 + dy) * (screen->width * 2) + x * 2 + dx] = z_interp;
				screen->drawlingBuffer[(y * 4 + dy) * (screen->width * 2) + x * 2 + dx] = DEFAULT_COLOR; // todo add vertex color support
			}
		}
	}
}

inline void rendermodeSuperSample2_with_vertex_color(vec2 v0, vec2 v1, vec2 a_prime, vec2 initPoint, float a_z, float b_z, float c_z, ivec3 c_a, ivec3 c_b, ivec3 c_c, TriBaryCache * cache) {
	static float x_step = 0.50;
	static float y_step = 1.0 / 4.0;
	static float x_center = 0.25;
	static float y_center = 1.0 / 8.0;
	static vec3 bayValues;
	vec2 point_cpy;
	vec2 v2;
	int x = (int)initPoint[0];
	int y = (int)initPoint[1];
	glm_vec2_zero(v2);
	glm_vec2_copy(initPoint, point_cpy);
	for (int dx = 0; dx < 2; dx++) {
		for (int dy = 0; dy < 4; dy++) {
			// todo finish braille code
			point_cpy[0] = initPoint[0] + (x_step * dx) + x_center;
			point_cpy[1] = initPoint[1] + (y_step * dy) + y_center;
			glm_vec2_sub(point_cpy, a_prime, v2);
			/*calculateBayCords(v0, v1, v2, bayValues);*/
		    calculateBayCords_cache(v0, v1, v2, cache, bayValues);
			int inside =
				(bayValues[0] >= 0.0f) &
				(bayValues[1] >= 0.0f) &
				(bayValues[2] >= 0.0f);
			if (!inside) continue;
			float z_interp = bayValues[0] * a_z + bayValues[1] * b_z + bayValues[2] * c_z;
			float z_buffer_val = screen->z_buffer[(y * 4 + dy) * (screen->width * 2) + x * 2 + dx];
			vec3 c_a_scaled;
			vec3 c_b_scaled;
			vec3 c_c_scaled;

			glm_vec3_copy((vec3) { c_a[0], c_a[1], c_a[2] }, c_a_scaled);
			glm_vec3_copy((vec3) { c_b[0], c_b[1], c_b[2] }, c_b_scaled);
			glm_vec3_copy((vec3) { c_c[0], c_c[1], c_c[2] }, c_c_scaled);

			glm_vec3_scale(c_a_scaled, bayValues[0], c_a_scaled);
			glm_vec3_scale(c_b_scaled, bayValues[1], c_b_scaled);
			glm_vec3_scale(c_c_scaled, bayValues[2], c_c_scaled);

			// add all colors into c_c_scaled
			glm_vec3_addadd(c_a_scaled, c_b_scaled, c_c_scaled);

			int color = 0;
			// set R G B channels
			color ^= ((int)(c_c_scaled[0])) << 16;
			color ^= ((int)(c_c_scaled[1])) << 8;
			color ^= ((int)(c_c_scaled[2]));

			if (z_interp > z_buffer_val) {
				screen->z_buffer[(y * 4 + dy) * (screen->width * 2) + x * 2 + dx] = z_interp;
				screen->drawlingBuffer[(y * 4 + dy) * (screen->width * 2) + x * 2 + dx] = color; // todo add vertex color support
			}
		}
	}
}

inline static void wireframePlotSample(int x, int y, float z, int sampleWidth, int sampleHeight, int color) {
	if (x < 0 || y < 0 || x >= sampleWidth || y >= sampleHeight) {
		return;
	}
	int idx = y * sampleWidth + x;
	if (z > screen->z_buffer[idx]) {
		screen->z_buffer[idx] = z;
		screen->drawlingBuffer[idx] = color;
	}
}

inline static void wireframeDrawLine(vec2 p0, vec2 p1, float z0, float z1) {
	int scale_x = 1;
	int scale_y = 1;
	int color = (int)' ';
	switch (screen->superSampleRate) {
	case 1:
		scale_x = 2;
		scale_y = 2;
		color = DEFAULT_COLOR;
		break;
	case 2:
		scale_x = 2;
		scale_y = 4;
		color = DEFAULT_COLOR;
		break;
	default:
		break;
	}

	float x0 = p0[0] * (float)scale_x;
	float y0 = p0[1] * (float)scale_y;
	float x1 = p1[0] * (float)scale_x;
	float y1 = p1[1] * (float)scale_y;

	float dx = x1 - x0;
	float dy = y1 - y0;
	float steps = fabsf(dx) > fabsf(dy) ? fabsf(dx) : fabsf(dy);
	if (steps < 1.0f) {
		int sampleWidth = screen->width * scale_x;
		int sampleHeight = screen->height * scale_y;
		wireframePlotSample((int)roundf(x0), (int)roundf(y0), z0, sampleWidth, sampleHeight, color);
		return;
	}
	float x_step = dx / steps;
	float y_step = dy / steps;
	float z_step = (z1 - z0) / steps;
	int sampleWidth = screen->width * scale_x;
	int sampleHeight = screen->height * scale_y;
	float x = x0;
	float y = y0;
	float z = z0;
	for (int i = 0; i <= (int)steps; i++) {
		wireframePlotSample((int)roundf(x), (int)roundf(y), z, sampleWidth, sampleHeight, color);
		x += x_step;
		y += y_step;
		z += z_step;
	}
}

inline void wireframeRender(vec2 a_prime, vec2 b_prime, vec2 c_prime, float a_z, float b_z, float c_z) {
	wireframeDrawLine(a_prime, b_prime, a_z, b_z);
	wireframeDrawLine(b_prime, c_prime, b_z, c_z);
	wireframeDrawLine(c_prime, a_prime, c_z, a_z);
}

inline void renderFace_smaa_mode_0(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z) {
	TriBaryCache cache = { 0.0f };
	compute_bary_cache(v0, v1, &cache);
	for (int x = start_x; x <= end_x; x++) {
		for (int y = start_y; y <= end_y; y++) {
			// need to get baycords
			vec2 v2;
			vec2 point;
			vec3 bayValues;
			point[0] = (float)x;
			point[1] = (float)y;
			glm_vec2_sub(point, a_prime, v2);
			/*calculateBayCords(v0, v1, v2, bayValues);*/
			calculateBayCords_cache(v0, v1, v2, &cache, bayValues);
			int inside =
				(bayValues[0] >= 0.0f) &
				(bayValues[1] >= 0.0f) &
				(bayValues[2] >= 0.0f);
			if (!inside) continue;
			float z_interp = bayValues[0] * a_z + bayValues[1] * b_z + bayValues[2] * c_z;
			float z_buffer_val = screen->z_buffer[y * screen->width + x];
			// do z_buffering here, the z value here should be negative since from the camera perspective
			// - z values are in front of it
			if (z_interp > z_buffer_val) {
				screen->z_buffer[y * screen->width + x] = z_interp;
				screen->drawlingBuffer[y * screen->width + x] = (int)' '; // todo make this repersent color instead of occupency (-1 should mean not occupied)
			}
		}
	}
}
inline void renderFace_smaa_mode_1(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z) {
	TriBaryCache cache = { 0.0f };
	compute_bary_cache(v0, v1, &cache);
	for (int x = start_x; x <= end_x; x++) {
		for (int y = start_y; y <= end_y; y++) {
			vec2 point;
			point[0] = (float)x;
			point[1] = (float)y;
			rendermodeSuperSample1(v0, v1, a_prime, point, a_z, b_z, c_z, &cache);
		}
	}
}
inline void renderFace_smaa_mode_2(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z) {
	TriBaryCache cache = { 0.0f };
	compute_bary_cache(v0, v1, &cache);
	for (int x = start_x; x <= end_x; x++) {
		for (int y = start_y; y <= end_y; y++) {
			vec2 point;
			point[0] = (float)x;
			point[1] = (float)y;
			rendermodeSuperSample2(v0, v1, a_prime, point, a_z, b_z, c_z, &cache);
		}
	}
}


inline void renderFace_smaa_mode_1_with_vertex_colors(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z, ivec3 c_a, ivec3 c_b, ivec3 c_c)
{
	TriBaryCache cache = { 0.0f };
	compute_bary_cache(v0, v1, &cache);
	for (int x = start_x; x <= end_x; x++) {
		for (int y = start_y; y <= end_y; y++) {
			vec2 point;
			point[0] = (float)x;
			point[1] = (float)y;
			rendermodeSuperSample1_with_vertex_colors(v0, v1, a_prime, point, a_z, b_z, c_z, c_a, c_b, c_c, &cache);
		}
	}
}


inline void renderFace_smaa_mode_2_with_vertex_colors(int start_x, int end_x, int start_y, int end_y, vec2 v0, vec2 v1, vec2 a_prime, float a_z, float b_z, float c_z, ivec3 c_a, ivec3 c_b, ivec3 c_c)
{
	TriBaryCache cache = { 0.0f };
	compute_bary_cache(v0,v1,&cache);
	for (int x = start_x; x <= end_x; x++) {
		for (int y = start_y; y <= end_y; y++) {
			vec2 point;
			point[0] = (float)x;
			point[1] = (float)y;
			rendermodeSuperSample2_with_vertex_color(v0, v1, a_prime, point, a_z, b_z, c_z, c_a, c_b, c_c, &cache);
		}
	}
}

inline void decomposeIntColor(int input, ivec3 output) {
	int channelR = (input >> 16) & 0xFF;
	int channelG = (input >> 8)  & 0xFF;
	int channelB = (input >> 0)  & 0xFF;
	output[0] = channelR;
	output[1] = channelG;
	output[2] = channelB;
}

inline void update_camera_vectors()
{
	// todo update camera vectors using pitch and yaw
	const float EPS = 0.0001f;
	if (fabsf(camera->yaw - camera->last_yaw) < EPS &&
		fabsf(camera->pitch - camera->last_pitch) < EPS)
	{
		return; // orientation unchanged
	}

	camera->last_yaw = camera->yaw;
	camera->last_pitch = camera->pitch;

	float yaw = glm_rad(camera->last_yaw);
	float pitch = glm_rad(camera->last_pitch);

	vec3 forward = {
		cosf(yaw) * cosf(pitch),
		sinf(pitch),
		sinf(yaw) * cosf(pitch)
	};
	glm_vec3_normalize(forward);
	glm_vec3_copy(forward, camera->forward);

	vec3 world_up = { UP };
	glm_vec3_crossn(camera->forward, world_up, camera->right);
	glm_vec3_crossn(camera->right, camera->forward, camera->up);
}

inline void downSample_smaa_mode1()
{
	
	for (int y = 0; y < screen->height; y++) {
		for (int x = 0; x < screen->width; x++) {
			ivec3 color = { 0,0,0 };
			// iterate over the cetroids of the sample points, get color average 
			unsigned int charCode = 0;
			for (int dy = 0; dy < 2; dy++) {
				for (int dx = 0; dx < 2; dx++) {
					ivec3 sampledColor = { 0,0,0 };
					decomposeIntColor(screen->drawlingBuffer[(y * 2 + dy) * (screen->width * 2) + x * 2 + dx], sampledColor);
					glm_ivec3_add(color, sampledColor, color);
					if (screen->z_buffer[(y * 2 + dy) * (screen->width * 2) + x * 2 + dx] != Z_MAX_DEPTH) {
						int idx = (y * 2 + dy) * (screen->width * 2) + x * 2 + dx;

						float chk = screen->z_buffer[(y * 2 + dy) * (screen->width * 2) + x * 2 + dx];

						charCode |= (1<<(dx+dy*2));
					}
				}
			}
			glm_ivec3_divs(color, 4, color);
			cellUnicodeArray[y * screen->width + x].characterCode = charCode;
			cellUnicodeArray[y * screen->width + x].Color.Channel.red = (char)color[0];
			cellUnicodeArray[y * screen->width + x].Color.Channel.green = (char)color[1];
			cellUnicodeArray[y * screen->width + x].Color.Channel.blue = (char)color[2];
		}
	}
}

inline void downSample_smaa_mode2()
{
	for (int x = 0; x < screen->width; x++) {
		for (int y = 0; y < screen->height; y++) {
			// iterate over the cetroids of the sample points, get color average
			ivec3 color = { 0,0,0 };
			unsigned int charCode = 0;
			for (int dy = 0; dy < 4; dy++) {
				for (int dx = 0; dx < 2; dx++) {
					ivec3 sampledColor = { 0, 0, 0 };
					decomposeIntColor(screen->drawlingBuffer[(y * 4 + dy) * (screen->width * 2) + x * 2 + dx], sampledColor);
					glm_ivec3_add(color, sampledColor, color);
					if (screen->z_buffer[(y * 4 + dy) * (screen->width * 2) + x * 2 + dx] != Z_MAX_DEPTH) {
						charCode |= (1 << (dx + dy * 2));
					}
				}
			}
			glm_ivec3_divs(color, 8, color);
			cellUnicodeArray[y * screen->width + x].characterCode = charCode;
			cellUnicodeArray[y * screen->width + x].Color.Channel.red = (char)color[0];
			cellUnicodeArray[y * screen->width + x].Color.Channel.green = (char)color[1];
			cellUnicodeArray[y * screen->width + x].Color.Channel.blue = (char)color[2];
		}
	}
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
	update_camera_vectors();
	if (!camera) {
		fprintf(stderr, "OOM inside render start up code\n");
		programExit(3);
	}
	cellUnicodeArray = malloc(sizeof(cellInfo) * width * height);
	if (!cellUnicodeArray) {
		fprintf(stderr, "OOM inside render start up code\n");
		programExit(3);
	}
}

void add_object_to_scene(object* obj) {
	int index = size_object_array;
	if (index < 10) {
        sceneObjects[index] = obj;
		size_object_array += 1;
	}
}

void remove_object_from_scene(object* obj) {
	if (!size_object_array) {
		return;
	}
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
	// note that currently this assumes that the object is centered around its own origin
	// in the future calculating a bound volume surface may be a better idea and uses it geometric center as the origin
	// i.e., rotate around the center of the bounding shape
	glm_rotate_z(model, glm_rad(obj->z_rotation), model);
	glm_rotate_y(model, glm_rad(obj->y_rotation), model);
	glm_rotate_x(model, glm_rad(obj->x_rotation), model);
	glm_scale(model, obj->scale); // scales the model first
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

Camera * getCamera()
{
	return camera;
}

void reset_scale_vector(object* obj)
{
	obj->scale[0] = 1.0;
	obj->scale[1] = 1.0;
	obj->scale[2] = 1.0;
}


