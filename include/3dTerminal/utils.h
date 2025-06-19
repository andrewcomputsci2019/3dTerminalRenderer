#pragma once
// Windows includes
#ifdef _WIN32
#include <conio.h>
#include <Windows.h>
#endif

#include <stdbool.h>
#include <stdio.h>

// POSIX includes Darwin and Linux/unix

/*
	Function for detecing if input exist in buffer
	returns such character if there is one
	d_flag is set to true if input is detected
	and s_flag is set to true if it is a special character
	deprecated by consoleInputAvailable
*/
//char detect_console_input(bool * d_lag, bool * s_flag);
/*
	gets the dimension of the console back to the user
*/
void get_console_dim(int* width, int* height);

/*
	Sets the console to be of width: width and height: height
*/
void set_console_dim(int width, int height);

/*
	Gets the Program compiled information
*/
void show_program_info();


// anything that needs to be inited should be in here on appliaction startup
void initApplication();


void programExit(unsigned short value);