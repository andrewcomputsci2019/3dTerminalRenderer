#include<3dTerminal/events.h>
#include<3dTerminal/render.h>
#include<3dTerminal/datastructures/queue.h>
#include<time.h>
MOUSE_DEVICE mouse = { false,false,0,0 };

#ifdef _WIN32
	#include<3dTerminal/handles.h>
	#define INPUT_BUFFER_SIZE 10
	extern void display_error_message();
	BOOL exitPrimed = FALSE;

	BYTE keyboard_state[256];

	InputBuffer console_inputs_buffer;

	MOUSE_EVENT_QUEUE mouse_events_queue;


	// call back function pointers, static to prevent sharing outside translation unit
	static void(*MOUSECallBackPositionFunction)(MOUSE_DEVICE* state, int x, int y) = NULL;
	static void(*MOUSECallBackClickFunction)(enum BUTTON_MOUSE button, MOUSE_DEVICE* state) = NULL;
	static void(*MOUSECallBackScrollFunction)(MOUSE_DEVICE* state, int scrollDirection) = NULL;
	static void(*MOUSECallBackFunction)(EVENT_MOUSE* event) = NULL;


	void init_input_buffer() {
		createBuffer(&console_inputs_buffer, INPUT_BUFFER_SIZE);
	}

	void init_mouse_event_queue() {
		init_MOUSE_EVENT_QUEUE(&mouse_events_queue, INPUT_BUFFER_SIZE);
	}

	void init_events_code() {
		init_input_buffer();
		init_mouse_event_queue();
	}




	void detectInputRecords() {
		DWORD windowEvents;
		static INPUT_RECORD inputRecord[10];
		GetNumberOfConsoleInputEvents(handle_stdin, &windowEvents);
		if (windowEvents > 0) {
			if (!ReadConsoleInput(handle_stdin, inputRecord, windowEvents, &windowEvents)) {
				display_error_message();
			}
			for (size_t i = 0; i < windowEvents; i++) {
				switch (inputRecord[i].EventType)
				{
					case WINDOW_BUFFER_SIZE_EVENT:
						COORD newSize = inputRecord[i].Event.WindowBufferSizeEvent.dwSize;
						//printf("Window changed size, now %d by %d\n", newSize.X, newSize.Y);
						updateScreenSize(newSize.X, newSize.Y);
						break;
					case KEY_EVENT:
						// only add keys that are pressed not released
						if(inputRecord[i].Event.KeyEvent.bKeyDown)
							addToBuffer(&console_inputs_buffer, inputRecord[i].Event.KeyEvent.wVirtualKeyCode);
						break;
					case MOUSE_EVENT:
						EVENT_MOUSE message = { .event = IGNORE_VALUE, .button = LEFT, .scrollDirection = 0 };
						// https://learn.microsoft.com/en-us/windows/console/mouse-event-record-str
						bool left_pressed = (bool)(inputRecord[i].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED);
						bool right_pressed = (bool)(inputRecord[i].Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED);
						switch (inputRecord[i].Event.MouseEvent.dwEventFlags)
						{
							case 0: // button press or release
								message.event = CLICK;
								if (mouse.left_click_pressed != left_pressed) {
									message.button = LEFT;
								}
								if (mouse.right_click_pressed != right_pressed) {
									message.button = RIGHT;
								}
								break;
							case MOUSE_MOVED:
								message.event = MOVE;
								break;
							case MOUSE_WHEELED:
								// this one is a bit strange we need to extract the higher order byte
								message.event = SCROLL;
								// get the high word out of the DWORD, last 16 bits
								DWORD scrollValue = (inputRecord[i].Event.MouseEvent.dwButtonState >> 16);
								if (scrollValue > 0) {
									//forward, away from the user
									message.scrollDirection = 1;
								}
								else {
									//towards the user
									message.scrollDirection = -1;
								}
								break;
							default:
								break;
						}
						mouse.left_click_pressed =  left_pressed;
						mouse.right_click_pressed = right_pressed;
						mouse.pos_x = inputRecord[i].Event.MouseEvent.dwMousePosition.X;
						mouse.pos_y = inputRecord[i].Event.MouseEvent.dwMousePosition.Y;
						memcpy_s(&(message.state), sizeof(MOUSE_DEVICE), &mouse, sizeof(MOUSE_DEVICE));
						if (message.event != IGNORE_VALUE) {
							enqueue_mouse_event(&mouse_events_queue, message);
						}
						break;
					default:
						break;
				}
			}
		}
	}
	void getKeyboardState() {
		// update once every 100ms
		static clock_t last_call;
		clock_t now;
		now = clock();
		if ((double)(now - last_call) / CLOCKS_PER_SEC < 0.016) {
			return;	
		}
		last_call = now;
		// only gets the subset of the keyboard this function
		// has fairly large over head from profiling
		for (int i = VK_LEFT; i < VK_LWIN; i++) {
			// from gamedev form, good way of getting keystrokes
			// get upper order of short, tells if key is down
			//https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate
			keyboard_state[i] = (BYTE)(GetAsyncKeyState(i) >> 8);
		}
	}


	void callMousePositionCallBack(EVENT_MOUSE* event) {
		if (MOUSECallBackPositionFunction) {
			(*MOUSECallBackPositionFunction)(&(event->state), event->state.pos_x, event->state.pos_y);
		}
	}

	void callMouseClickCallBack(EVENT_MOUSE* event) {
		if (MOUSECallBackClickFunction) {
			(*MOUSECallBackClickFunction)(event->button, &(event->state));
		}
	}

	void callMouseScrollCallBack(EVENT_MOUSE* event) {
		if (MOUSECallBackScrollFunction) {
			(*MOUSECallBackScrollFunction)(&(event->state), event->scrollDirection);
		}
	}

	void callMouseCallBack(EVENT_MOUSE * event) {
		if (MOUSECallBackFunction) {
			(*MOUSECallBackFunction)(event);
		}
	}

	

	void dispatchMouseEvents() {
		while (!MOUSE_EVENT_QUEUE_isEmpty(&mouse_events_queue)) {
			EVENT_MOUSE event;
			event = pop_mouse_event(&mouse_events_queue);
			// in general all events should be passed to the General Mouse call Back Function
			callMouseCallBack(&event);
			// then need to find what type of event to be routed correctly
			switch (event.event)
			{
				case MOVE:
                    callMousePositionCallBack(&event);
					break;
				case CLICK:
					callMouseClickCallBack(&event);
					break;
				case SCROLL:
					callMouseScrollCallBack(&event);
					break;
				case IGNORE_VALUE:
				default:
					break;
			}
		}
    }

	void dispatchEvents() {
		dispatchMouseEvents();
	}
	void pollEvents() {
		getKeyboardState();
		detectInputRecords();
		dispatchEvents();
	}
	// todo
	int getKeyState(int key) {
		return 0;
	}
	// todo
	int getScanCodeState(int scanCode) {
		return 0;
	}

	void setMousePositionCallBack(void (*func)(MOUSE_DEVICE* state, int x, int y))
	{
        MOUSECallBackPositionFunction = func;
	}

	void setMouseCallBackClickEvent(void(*func)(enum BUTTON_MOUSE button, MOUSE_DEVICE* state))
	{
		MOUSECallBackClickFunction = func;
	}

	void setMouseCallBackScrollEvent(void(*func)(MOUSE_DEVICE* state, int scrollDirection))
	{
		MOUSECallBackScrollFunction = func;
	}

	void setMouseCallBack(void (*func)(EVENT_MOUSE* event))
	{
		MOUSECallBackFunction = func;
	}


	bool shouldExit()
	{
		return exitPrimed == TRUE;
	}
	void setShouldExit(bool exit)
	{
		exitPrimed = (BOOL)exit;
	}

	bool consoleInputAvailable()
	{
		return isAvail(&console_inputs_buffer);
	}

	unsigned char getConsoleInput()
	{

		return getFirst(&console_inputs_buffer);
	}

	MOUSE_DEVICE getMouse()
	{
		// get last state of the mouse
		return (MOUSE_DEVICE) { mouse.left_click_pressed, mouse.right_click_pressed, mouse.pos_x, mouse.pos_y };
	}

#endif