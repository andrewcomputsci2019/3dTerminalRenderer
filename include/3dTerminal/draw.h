#pragma once

#ifdef _WIN32
#include<Windows.h>
void draw_to_screen(int* buffer, int color, int backGroundColor,int width, int height);
#endif // _WIN32


enum text_mode {
    quater_block,
    braille
};

typedef struct {
    unsigned int characterCode;
    union {
        int color : 24;
        struct {
            char red;
            char green;
            char blue;
        }Channel;
    }Color;
}cellInfo;

/*
    SQUARE PATTERN

    1   2
    4   8
*/

#define QUATER_SQUARE_TOP_LEFT 1
#define QUATER_SQUARE_TOP_RIGHT 2
#define QUATER_SQUARE_BOTTOM_LEFT 4
#define QUATER_SQUARE_BOTTOM_RIGHT 8

/*
    BRAILLE PATTERN LOOKS LIKE THE FOLLOWING
    1   2
    3   4
    5   6
    7   8
*/
#define BRAILLE_1 1
#define BRAILLE_2 2
#define BRAILLE_3 4
#define BRAILLE_4 8
#define BRAILLE_5 16
#define BRAILLE_6 32
#define BRAILLE_7 64
#define BRAILLE_8 128



void draw_to_screen_unicode(cellInfo* buffer, int width, int height, enum text_mode mode);