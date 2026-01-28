#include <3dTerminal/3dterminal.h>
#include <3dTerminal/3d_event_helpers.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include<time.h>
/*
* ________________________________________________________________
* GOALS
* 1. GET cglm Linked -------------------- DONE
* 2. get console width and height --------------- DONE
* 3. able to adjust console width and height --> done
* 4. set call backs to mouse inputs : done
* 5. figure what characters to use for drawling : kinda done
* 6. implement basic structures for 3d objects, triangle mesh idea, bounding boxes etc --> done
* 7. implement 3d math to go from world space to camera to clip --> done
* 8. implement backface culling, z-buffer, and frustum culling ==> done
* 9. Lighting, post processing etc --> todo
* 10. Camera movment, using mouse or key strokes
*/


void posCallBack(MOUSE_DEVICE* state, int x, int y) {
	printf("mouse location is %d, %d\n", x, y);
}



int main() {
	initApplication(2);
	show_program_info();
    /*setMousePositionCallBack(posCallBack);*/  // changed with mouse and keyboard handler uncomment to have original code
	setWireframeMode(0);
	int rows, cols;
	get_console_dim(&rows, &cols);
	printf("Console width is %d , and height is %d\n", rows, cols);
	object obj;
	obj.mesh = _aligned_malloc(sizeof(vec3) * 3, 16);
	obj.faceList = _aligned_malloc(sizeof(ivec3), 16);
	reset_scale_vector(&obj);
	// counter clockwise winding
	// triangle
	obj.mesh[0][0] = 0.0;
	obj.mesh[0][1] = 4.0;
	obj.mesh[0][2] = -1.0;
	obj.mesh[1][0] = -5.0;
	obj.mesh[1][1] = -2.0;
	obj.mesh[1][2] = -3.0;
	obj.mesh[2][0] = 4.0;
	obj.mesh[2][1] = 0.0;
	obj.mesh[2][2] = -2.0;
	obj.meshSize = 3;
	obj.faceListLength = 1;
	obj.faceList[0][0] = 0;
	obj.faceList[0][1] = 1;
	obj.faceList[0][2] = 2;
	obj.x_rotation = 0;
	obj.y_rotation = 0;
	obj.z_rotation = 0;
	obj.postion[0] = 2.0f;
	obj.postion[1] = 2.5f;
	obj.postion[2] = -15.5f;
	obj.has_vertex_color = 1;
	obj.vertex_color = _aligned_malloc(sizeof(ivec3) * 3, 16);
	if (!obj.vertex_color) {
		return 1;
	}
	glm_ivec3_copy((ivec3) { 255, 0, 0 }, obj.vertex_color[0]);
	glm_ivec3_copy((ivec3) { 0, 255, 0 }, obj.vertex_color[1]);
	glm_ivec3_copy((ivec3) { 0, 0, 255 }, obj.vertex_color[2]);

	object cube;
	cube.mesh = _aligned_malloc(sizeof(vec3) * 8, 16);
	cube.faceList = _aligned_malloc(sizeof(ivec3) * 12, 16);
	reset_scale_vector(&cube);

	/*
	   Cube centered at origin, side length = 2
	   Vertices:
		 0: (-1,-1,-1)
		 1: ( 1,-1,-1)
		 2: ( 1, 1,-1)
		 3: (-1, 1,-1)
		 4: (-1,-1, 1)
		 5: ( 1,-1, 1)
		 6: ( 1, 1, 1)
		 7: (-1, 1, 1)
	*/

	// ---- vertices ----
	cube.mesh[0][0] = -1.0f; cube.mesh[0][1] = -1.0f; cube.mesh[0][2] = -1.0f;
	cube.mesh[1][0] = 1.0f; cube.mesh[1][1] = -1.0f; cube.mesh[1][2] = -1.0f;
	cube.mesh[2][0] = 1.0f; cube.mesh[2][1] = 1.0f; cube.mesh[2][2] = -1.0f;
	cube.mesh[3][0] = -1.0f; cube.mesh[3][1] = 1.0f; cube.mesh[3][2] = -1.0f;

	cube.mesh[4][0] = -1.0f; cube.mesh[4][1] = -1.0f; cube.mesh[4][2] = 1.0f;
	cube.mesh[5][0] = 1.0f; cube.mesh[5][1] = -1.0f; cube.mesh[5][2] = 1.0f;
	cube.mesh[6][0] = 1.0f; cube.mesh[6][1] = 1.0f; cube.mesh[6][2] = 1.0f;
	cube.mesh[7][0] = -1.0f; cube.mesh[7][1] = 1.0f; cube.mesh[7][2] = 1.0f;

	cube.meshSize = 8;
	cube.faceListLength = 12;

	// ---- faces (12 triangles) CCW winding from outside ----

	// Front (+Z): 4,5,6,7
	cube.faceList[0][0] = 4; cube.faceList[0][1] = 5; cube.faceList[0][2] = 6;
	cube.faceList[1][0] = 4; cube.faceList[1][1] = 6; cube.faceList[1][2] = 7;

	// Back (-Z): 0,1,2,3
	cube.faceList[2][0] = 0; cube.faceList[2][1] = 2; cube.faceList[2][2] = 1;
	cube.faceList[3][0] = 0; cube.faceList[3][1] = 3; cube.faceList[3][2] = 2;

	// Left (-X): 0,3,7,4
	cube.faceList[4][0] = 0; cube.faceList[4][1] = 7; cube.faceList[4][2] = 3;
	cube.faceList[5][0] = 0; cube.faceList[5][1] = 4; cube.faceList[5][2] = 7;

	// Right (+X): 1,2,6,5
	cube.faceList[6][0] = 1; cube.faceList[6][1] = 2; cube.faceList[6][2] = 6;
	cube.faceList[7][0] = 1; cube.faceList[7][1] = 6; cube.faceList[7][2] = 5;

	// Top (+Y): 3,2,6,7
	cube.faceList[8][0] = 3; cube.faceList[8][1] = 6; cube.faceList[8][2] = 2;
	cube.faceList[9][0] = 3; cube.faceList[9][1] = 7; cube.faceList[9][2] = 6;

	// Bottom (-Y): 0,1,5,4
	cube.faceList[10][0] = 0; cube.faceList[10][1] = 5; cube.faceList[10][2] = 1;
	cube.faceList[11][0] = 0; cube.faceList[11][1] = 4; cube.faceList[11][2] = 5;

	cube.x_rotation = 0;
	cube.y_rotation = 0;
	cube.z_rotation = 0;
	cube.postion[0] = -4.5f;
	cube.postion[1] = 0.5f;
	cube.postion[2] = -18.5f;

	float start_x = cube.postion[0];
	float start_y = cube.postion[1];
	float start_z = cube.postion[2];


	/*
		keyboard and mouse camera handler stuff
	*/

	set_camera(getCamera());
	setMouseCallBack(process_mouse_callback);
	/*
		end additions
	*/

	clock_t start, end, lastTime, currTime;
	double elapsed_time;
	double frames = 0;
	updateObjectModelMat(&obj);
	setBoundingBox(&obj);
	add_object_to_scene(&obj);
	updateObjectModelMat(&cube);
	setBoundingBox(&cube);
	add_object_to_scene(&cube);
	start = clock();
	lastTime = clock();
	double targetDelta = 1 / 1500.0; // 120 fps
	double total_time = 0.0;
	int wireframe = 0;
	while (!shouldExit()) {
		currTime = clock();
		double delta = (double)(currTime - lastTime) / CLOCKS_PER_SEC;
		total_time += delta;
		pollEvents();
		if (consoleInputAvailable()) {
			char c = getConsoleInput(); // gives characters as vk codes
			/*
				keyboard and mouse camera handler stuff
			*/
			if (c == 'Q') {
				wireframe = !wireframe;
				setWireframeMode(wireframe);
			}
			process_keyboard_input_movement(c);
			/*
				end additions
			*/
		}
		lastTime = currTime;
		draw();
		frames++;
		obj.z_rotation = (float)fmod(obj.z_rotation + 72.0f*(delta), 360.0);
		cube.y_rotation = (float)fmod(obj.z_rotation + 72.0f * (delta), 360.0);
		/*cube.z_rotation = (float)fmod(obj.z_rotation + 36.0f * (delta), 360.0);*/
		cube.postion[0] = start_x - 2 * sin(total_time);
		cube.postion[2] = start_z + 2.5f * cos(total_time);
		updateObjectModelMat(&obj);
		updateObjectModelMat(&cube);
	}
	end = clock();
	elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;
	double fps = frames / elapsed_time;
	//printf("FPS WAS %lf\n", fps);
	get_console_dim(&rows, &cols);
	printf("Console width is %d , and height is %d\n", rows, cols);
	MOUSE_DEVICE mouse = getMouse();
	printf("last mouse location is %d, %d\n", mouse.pos_x, mouse.pos_y);
	programExit(0);
}