#include <3dTerminal/3dterminal.h>
#include <3dTerminal/3d_event_helpers.h>
#include<time.h>

int main() {
	initApplication(2);
	object cube;
	cube.y_rotation = 0.0f;
	cube.x_rotation = 0.0f;
	cube.z_rotation = 0.0f;
	setWireframeMode(0);

	cube.meshSize = 8;
	cube.faceListLength = 12;

	reset_scale_vector(&cube);

	cube.mesh = _aligned_malloc(sizeof(vec3) * 8, 16);
	cube.faceList = _aligned_malloc(sizeof(ivec3) * 12, 16);
	cube.vertex_color = _aligned_malloc(sizeof(ivec3) * 8, 16);
	cube.has_vertex_color = 1;

	// vertex 0
	cube.mesh[0][0] = -0.492931f;
	cube.mesh[0][1] = 0.128937f;
	cube.mesh[0][2] = -10.741019f;
	cube.vertex_color[0][0] = 51;
	cube.vertex_color[0][1] = 51;
	cube.vertex_color[0][2] = 51;

	// Vertex 1
	cube.mesh[1][0] = 0.025111f;
	cube.mesh[1][1] = -1.847709f;
	cube.mesh[1][2] = -9.584261f;
	cube.vertex_color[1][0] = 204;
	cube.vertex_color[1][1] = 51;
	cube.vertex_color[1][2] = 51;

	// Vertex 2
	cube.mesh[2][0] = -1.253942f;
	cube.mesh[2][1] = -1.113622f;
	cube.mesh[2][2] = -7.757049f;
	cube.vertex_color[2][0] = 51;
	cube.vertex_color[2][1] = 204;
	cube.vertex_color[2][2] = 51;

	// Vertex 3
	cube.mesh[3][0] = -1.771980f;
	cube.mesh[3][1] = 0.863028f;
	cube.mesh[3][2] = -8.913807f;
	cube.vertex_color[3][0] = 51;
	cube.vertex_color[3][1] = 51;
	cube.vertex_color[3][2] = 204;

	// Vertex 4
	cube.mesh[4][0] = 0.645860f;
	cube.mesh[4][1] = -0.080392f;
	cube.mesh[4][2] = -6.842286f;
	cube.vertex_color[4][0] = 51;
	cube.vertex_color[4][1] = 204;
	cube.vertex_color[4][2] = 204;

	// Vertex 5
	cube.mesh[5][0] = 0.127818f;
	cube.mesh[5][1] = 1.896260f;
	cube.mesh[5][2] = -7.999044f;
	cube.vertex_color[5][0] = 204;
	cube.vertex_color[5][1] = 51;
	cube.vertex_color[5][2] = 204;

	// Vertex 6
	cube.mesh[6][0] = 1.924910f;
	cube.mesh[6][1] = -0.814484f;
	cube.mesh[6][2] = -8.669510f;
	cube.vertex_color[6][0] = 204;
	cube.vertex_color[6][1] = 204;
	cube.vertex_color[6][2] = 51;

	// Vertex 7
	cube.mesh[7][0] = 1.406871f;
	cube.mesh[7][1] = 1.162166f;
	cube.mesh[7][2] = -9.826268f;
	cube.vertex_color[7][0] = 204;
	cube.vertex_color[7][1] = 204;
	cube.vertex_color[7][2] = 204;

	// Face 0: 3 0 1 2
	cube.faceList[0][0] = 0;
	cube.faceList[0][1] = 1;
	cube.faceList[0][2] = 2;

	// Face 1: 3 0 2 3
	cube.faceList[1][0] = 0;
	cube.faceList[1][1] = 2;
	cube.faceList[1][2] = 3;

	// Face 2: 3 3 2 4
	cube.faceList[2][0] = 3;
	cube.faceList[2][1] = 2;
	cube.faceList[2][2] = 4;

	// Face 3: 3 3 4 5
	cube.faceList[3][0] = 3;
	cube.faceList[3][1] = 4;
	cube.faceList[3][2] = 5;

	// Face 4: 3 5 4 6
	cube.faceList[4][0] = 5;
	cube.faceList[4][1] = 4;
	cube.faceList[4][2] = 6;

	// Face 5: 3 5 6 7
	cube.faceList[5][0] = 5;
	cube.faceList[5][1] = 6;
	cube.faceList[5][2] = 7;

	// Face 6: 3 7 6 1
	cube.faceList[6][0] = 7;
	cube.faceList[6][1] = 6;
	cube.faceList[6][2] = 1;

	// Face 7: 3 7 1 0
	cube.faceList[7][0] = 7;
	cube.faceList[7][1] = 1;
	cube.faceList[7][2] = 0;

	// Face 8: 3 3 5 7
	cube.faceList[8][0] = 3;
	cube.faceList[8][1] = 5;
	cube.faceList[8][2] = 7;

	// Face 9: 3 3 7 0
	cube.faceList[9][0] = 3;
	cube.faceList[9][1] = 7;
	cube.faceList[9][2] = 0;

	// Face 10: 3 4 2 1
	cube.faceList[10][0] = 4;
	cube.faceList[10][1] = 2;
	cube.faceList[10][2] = 1;

	// Face 11: 3 4 1 6
	cube.faceList[11][0] = 4;
	cube.faceList[11][1] = 1;
	cube.faceList[11][2] = 6;

	cube.postion[0] = 0.0f;
	cube.postion[1] = 0.0f;
	cube.postion[2] = -9.5f;

	vec3 centroid;
	glm_vec3_zero(centroid);
	for (size_t i = 0; i < cube.meshSize; i++) {
		centroid[0] += cube.mesh[i][0];
		centroid[1] += cube.mesh[i][1];
		centroid[2] += cube.mesh[i][2];
	}

	glm_vec3_scale(centroid, 1.0f / cube.meshSize, centroid);

	for (size_t i = 0; i < cube.meshSize; i++) {
		glm_vec3_sub(cube.mesh[i], centroid, cube.mesh[i]);
	}

	// hooks up mouse to the fp camera
	//set_camera(getCamera());
	//setMouseCallBack(process_mouse_callback);
	Camera* camera = getCamera();
	updateObjectModelMat(&cube);
	setBoundingBox(&cube);
 
	float baseRadius = 6.0f;

	float zoomAmp = 1.0f;
	float zoomSpeed = 0.5f;


	clock_t lastTime, currTime;
	double total_time = 0.0;
	lastTime = clock();
	add_object_to_scene(&cube);
	int wireframe = 0;
	float rotation_speed = 2.0f;
	float start_pos = cube.postion[1];

	// orbit camera system
	float camera_rotation_speed = 10.0f;
	float yawRad;
	float pitchRad;
	vec3 orbitTarget;
	glm_vec3_copy(cube.postion, orbitTarget);
	while (!shouldExit()) {
		currTime = clock();
		double delta = (double)(currTime - lastTime) / CLOCKS_PER_SEC;
		total_time += delta;
		pollEvents();
		if (consoleInputAvailable()) {
			char c = getConsoleInput();
			if (c == 'Q') {
				wireframe = !wireframe;
				setWireframeMode(wireframe);
			}
			//process_keyboard_input_movement(c);
		}

		float radius =  baseRadius + sinf(total_time * zoomSpeed) * zoomAmp;
		camera->yaw += camera_rotation_speed * delta;
		camera->yaw = fmodf(camera->yaw, 360.0f);
		yawRad = glm_rad(camera->yaw);
		pitchRad = glm_rad(camera->pitch);

		camera->pos[0] = orbitTarget[0] - cosf(yawRad) * cosf(pitchRad) * radius;
		camera->pos[1] = orbitTarget[1] - sinf(pitchRad) * radius;
		camera->pos[2] = orbitTarget[2] - sinf(yawRad) * cosf(pitchRad) * radius;
		
		//printf(
		//	"cam: (%.3f %.3f %.3f)  target: (%.3f %.3f %.3f)  dist: %.3f\n",
		//	camera->pos[0], camera->pos[1], camera->pos[2],
		//	orbitTarget[0], orbitTarget[1], orbitTarget[2],
		//	glm_vec3_distance(camera->pos, target)
		//);

		lastTime = currTime;
		draw();
		cube.z_rotation = fmodf(total_time * rotation_speed, 360.0f);
		cube.postion[1] = start_pos - 0.25f * sin(total_time);
		updateObjectModelMat(&cube);
	}

}
