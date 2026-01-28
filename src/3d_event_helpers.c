#include <3dTerminal/3d_event_helpers.h>




static Camera* camera;

typedef struct Mouse_Tracker_t {
	float yaw;
	float pitch;
	float lastX;
	float lastY;
	short left_down;
	short right_down;
}Mouse_Tracker;

void set_camera(Camera* render_camera)
{
	if (render_camera)
		camera = render_camera;
}


// todo fix this to use computed values inside of camera object no need for inverse rotation matrix

void process_keyboard_input_movement(unsigned char input)
{
	if (!camera)
		return;

	const float cameraSpeed = 0.5f;
	vec3 delta = { 0.0f, 0.0f, 0.0f };

	if (input == VK_LEFT || input == 'A') {
		glm_vec3_scale(camera->right, -cameraSpeed, delta);
	}
	else if (input == VK_RIGHT || input == 'D') {
		glm_vec3_scale(camera->right, cameraSpeed, delta);
	}
	else if (input == VK_UP || input == 'W') {
		glm_vec3_scale(camera->forward, cameraSpeed, delta);
	}
	else if (input == VK_DOWN || input == 'S') {
		glm_vec3_scale(camera->forward, -cameraSpeed, delta);
	}
	else if (input == 'I') {
		delta[1] += cameraSpeed;
	}
	else if (input == 'J') {
		delta[1] -= cameraSpeed;
	}

	glm_vec3_add(camera->pos, delta, camera->pos);
}

void process_mouse_callback(EVENT_MOUSE* event_p)
{
	static Mouse_Tracker tracker;
	EVENT_MOUSE event = *event_p;
	if (!camera)
		return;
	// todo process mouse events to allow for camera drag
	if (event.event == CLICK) {
		if (event.button == LEFT) {
			tracker.left_down = !tracker.left_down;
		}
		else if (event.button == RIGHT) {
			tracker.right_down = !tracker.right_down;
		}
	}
	else if (event.event == MOVE) {

		float currentX = event.state.pos_x;
		float currentY = event.state.pos_y;

		float x_offset = currentX - tracker.lastX;
		float y_offset = currentY - tracker.lastY;

		tracker.lastX = currentX;
		tracker.lastY = currentY;

		if (!tracker.left_down) { // if left is not down dont move the camera but still track the mouse location
			return;
		}

		float sensitivity = 0.1f;
		
		x_offset *= -sensitivity;
		y_offset *= -sensitivity;

		tracker.yaw = camera->yaw;
		tracker.pitch = camera->pitch;

		tracker.yaw += x_offset;
		tracker.pitch += y_offset;

		if (tracker.pitch > 89.0f) {
			tracker.pitch = 89.0f;
		}
		if (tracker.pitch < -89.0f) {
			tracker.pitch = -89.0f;
		}

		camera->pitch = tracker.pitch;
		camera->yaw = tracker.yaw;
	}
}


void getInverseRotationMatrix(mat4 dest) {
	if (!camera)
		return;
	glm_mat4_identity(dest);
	glm_rotate(dest, glm_rad(camera->pitch), (vec3) { 1.0f, 0.0f, 0.0f }); // rotate about x-axis
	glm_rotate(dest, glm_rad(camera->yaw), (vec3) { 0.0f, 1.0f, 0.0f }); // rotate about y-axis
	glm_mat4_inv(dest, dest);
}

