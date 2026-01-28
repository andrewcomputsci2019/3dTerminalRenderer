#pragma once

#include <3dTerminal/events.h>
#include <3dTerminal/render.h>



void set_camera(Camera* render_camera);

void process_keyboard_input_movement(unsigned char input);

void process_mouse_callback(EVENT_MOUSE* event_p);


void getInverseRotationMatrix(mat4 dest);