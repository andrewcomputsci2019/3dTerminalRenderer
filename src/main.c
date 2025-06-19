#include <3dTerminal/3dterminal.h>
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
	initApplication();
	show_program_info();
    setMousePositionCallBack(posCallBack);
	int rows, cols;
	get_console_dim(&rows, &cols);
	printf("Console width is %d , and height is %d\n", rows, cols);
	object obj;
	obj.mesh = _aligned_malloc(sizeof(vec3) * 3, 16);
	obj.faceList = _aligned_malloc(sizeof(vec3), 16);
	// counter clockwise winding
	obj.mesh[0][0] = 0.0;
	obj.mesh[0][1] = 2.0;
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
	obj.postion[0] = 0.0;
	obj.postion[1] = 0.0f;
	obj.postion[2] = -5.0f;
	clock_t start, end, lastTime, currTime;
	double elapsed_time;
	double frames = 0;
	updateObjectModelMat(&obj);
	setBoundingBox(&obj);
	add_object_to_scene(&obj);
	start = clock();
	lastTime = clock();
	double targetDelta = 1 / 1500.0; // 120 fps
	while (!shouldExit()) {
		currTime = clock();
		double delta = (double)(currTime - lastTime) / CLOCKS_PER_SEC;
		pollEvents();
		if (consoleInputAvailable()) {
			char c = getConsoleInput();
		}
		lastTime = currTime;
		draw();
		frames++;
		obj.z_rotation = (float)fmod(obj.z_rotation + 72.0f*(delta), 360.0);
		updateObjectModelMat(&obj);
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