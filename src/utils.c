#include <3dTerminal/utils.h>
#include <cglm/version.h>
// utilty for turning macro values into const strings
#define STRINGFY(s) #s
#define STRING(x) STRINGFY(x)
// WINDOWS implentation
#ifdef _WIN32
#include<3dTerminal/handles.h>
extern BOOL exitPrimed;
// deprecated by  consoleInputAvailable

//char detect_console_input(bool * d_flag, bool *s_flag) {
//	if (_kbhit()) { //if keystroke present return true
//		*d_flag = true;
//		//char data = _getch();
//		//if (data == 0) {
//		//	*s_flag = true;
//		//	data = _getch();
//		//}
//		char data;
//		DWORD count = 0;
//		ReadConsole(handle_stdin, &data, 1, &count, NULL);
//		s_flag = FALSE;
//		return data;
//	}
//	return 0;
//}

void show_program_info() {
	printf("Terminal3D \t version %d.%d.%d, \t Platform %s \t CGLM VERSION %d.%d.%d \n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, STRING(WINDOWS), CGLM_VERSION_MAJOR, CGLM_VERSION_MINOR, CGLM_VERSION_PATCH);
}

// windows only implementation
void get_console_dim(int* width, int* height) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(handle_stdout, &csbi);
	*width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	*height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}


// this isnt really that useful in all reality but does do what its intended
void set_console_dim(int width, int height) {
	HANDLE out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD dwSize = { width,height };
	SetConsoleScreenBufferSize(out_handle, dwSize);
	SMALL_RECT wsize = { 0,0,width - 1,height - 1 };
	SetConsoleWindowInfo(out_handle, TRUE, &wsize);
}


void display_error_message() {
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL
	);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	ExitProcess(dw);
}
static BOOL sysKillSignalHandler(DWORD crtlType) {
	if (crtlType == CTRL_C_EVENT) {
		// in the future maybe make this more like glfw 
		// so that on detection of this we process it after the last frame
		printf("CTRL-C detected exit is now primed\n");
		exitPrimed = TRUE;
		// programExit(0);
		return TRUE;
	}
	else if (crtlType == CTRL_CLOSE_EVENT) {
		printf("Close detected exit is now primed\n");
		exitPrimed = TRUE;
		return TRUE;
	}
	return FALSE;
}

// used to init input buffer inside events.h
extern void init_events_code();
extern void initRenderCode(int width, int height,int smaa_mode);
void initApplication(int smaa_mode) {
	handle_stdin = GetStdHandle(STD_INPUT_HANDLE);
	handle_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	DWORD mode;
	mode = GetConsoleMode(handle_stdin, &mode);
	// get updates from window ie resize and mouse click etc
	mode &= ~ENABLE_ECHO_INPUT;
	mode &= ~ENABLE_LINE_INPUT;
	mode &= ~ENABLE_QUICK_EDIT_MODE;
	if (!SetConsoleMode(handle_stdin, (mode | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_EXTENDED_FLAGS))) {
		display_error_message();
	}
	mode = 0;
	if (!GetConsoleMode(handle_stdout, &mode)) {
		display_error_message();
	}
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(handle_stdout, mode)) {
		display_error_message();
	}
	if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)sysKillSignalHandler, TRUE))
		display_error_message();
	init_events_code();
	int width;
	int height;
	get_console_dim(&width, &height);
	initRenderCode(width, height, smaa_mode);
}

void initApplication_bg_mode(){
	initApplication(0);
}

void initApplication_unicode_1(){
	initApplication(1);
}

void initApplication_unicode_2(){
	initApplication(2);
}

void programExit(unsigned short value)
{
	printf("Killing program....\n");
	ExitProcess(value);
}

#endif // _WIN32


// Apple implentation
#ifdef __APPLE__
#include <TargetConditionals.h>
#ifdef TARGET_OS_MAC
bool detect_console_input() {
	// todo
	return false
}
#elseif
	#error Cannot compile for anything but macos on Apple device
#endif


#endif // __APPLE__

// Linux implentation
#ifdef __linux
bool detect_console_input() {
	// todo
	return false
}
#endif // __linux

