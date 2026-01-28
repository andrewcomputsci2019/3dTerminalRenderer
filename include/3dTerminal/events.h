#pragma once
#include<3dTerminal/utils.h>




enum EVENT_MOUSE {
	MOVE, CLICK, SCROLL, IGNORE_VALUE
};

enum BUTTON_MOUSE {
	LEFT,RIGHT
};


typedef struct MOUSE_DEVICE_t {
	bool left_click_pressed;
	bool right_click_pressed;
	int pos_x;
	int pos_y;
}MOUSE_DEVICE;

typedef struct EVENT_MESSAGE_MOUSE {
	enum EVENT_MOUSE event;
	MOUSE_DEVICE state;
	enum BUTTON_MOUSE button;
	int scrollDirection;
}EVENT_MOUSE;

// similar to glfw checks for updates on events look resize and keyboard input
void pollEvents();

int getKeyState(int key);

int getScanCodeState(int scanCode);


void setMousePositionCallBack(void (*func)(MOUSE_DEVICE* state, int x, int y));

void setMouseCallBackClickEvent(void(*func)(enum BUTTON_MOUSE button, MOUSE_DEVICE* state));

void setMouseCallBackScrollEvent(void(*func)(MOUSE_DEVICE* state, int scrollDirection));

// for all events of the mouse
void setMouseCallBack(void (*func)(EVENT_MOUSE * event));


bool shouldExit();

void setShouldExit(bool exit);


bool consoleInputAvailable();

unsigned char getConsoleInput();

MOUSE_DEVICE getMouse();