#include <3dTerminal/3dterminal.h>
#include <3dTerminal/3d_event_helpers.h>
#include <3dTerminal/ply_utils.h>



#include <time.h>


int main() {
	initApplication(2);
	short wireFrameMode = 0;
	short selector = 0; // selects what model to render
	setWireframeMode(wireFrameMode);
	object * teapot_obj = get_object_from_ply("teapot.ply", (plyLayout) {1});
	object * bunny_obj = get_object_from_ply("bunny.ply", (plyLayout) { 1 });
	object * dragon_obj = get_object_from_ply("dragon.ply", (plyLayout) { 1 });
	if (!teapot_obj) {
		printf("Failed to produce ply object\n");
		return 1;
	}
	printf("Ply data: Had %llu vertices and %llu faces, has vertex color %s\n", teapot_obj->meshSize,teapot_obj->faceListLength, teapot_obj->has_vertex_color ? "YES": "NO");

	Camera* cam;
	cam = getCamera();
	cam->pos[2] = 30.0f;
	set_camera(cam);
	setMouseCallBack(process_mouse_callback);
	// model updates
	teapot_obj->scale[0] = 0.15f;
	teapot_obj->scale[1] = 0.15f;
	teapot_obj->scale[2] = 0.15f;
	teapot_obj->x_rotation = -90.0f;
	updateObjectModelMat(teapot_obj);
	add_object_to_scene(teapot_obj);


	bunny_obj->scale[0] = 22;
	bunny_obj->scale[1] = 22;
	bunny_obj->scale[2] = 22;
	bunny_obj->x_rotation = -90.0f;
	updateObjectModelMat(bunny_obj);

	dragon_obj->scale[0] = 0.45f;
	dragon_obj->scale[1] = 0.45f;
	dragon_obj->scale[2] = 0.45f;

	updateObjectModelMat(dragon_obj);


	while (!shouldExit()) {
		pollEvents(); // dont forget this, otherwise screen, kb, and mouse all wont work.
		if (consoleInputAvailable()) {
			unsigned char c = getConsoleInput();
			if (c == 'Q') {
				wireFrameMode = !wireFrameMode;
				setWireframeMode(wireFrameMode);
			}
			if (c == 'T') {
				switch (selector) {
				case 0: 
					remove_object_from_scene(teapot_obj);
					break;	
				case 1:
					remove_object_from_scene(bunny_obj);
					break;
				case 2:
					remove_object_from_scene(dragon_obj);
					break;
				}
				selector = (selector + 1) % 3;
				switch (selector) {
				case 0:
					add_object_to_scene(teapot_obj);
					break;
				case 1:
					add_object_to_scene(bunny_obj);
					break;
				case 2:
					add_object_to_scene(dragon_obj);
					break;
				}
			}
			process_keyboard_input_movement(c);
		}
		draw();
	}
	_aligned_free(teapot_obj);
	return 0;
}
