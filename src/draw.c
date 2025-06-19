#include <3dTerminal/draw.h>
#include <stdio.h>
#include <time.h>
#ifdef _WIN32
	#include <3dTerminal/handles.h>
extern HANDLE handle_stdin, handle_stdout;
#endif // _WIN32


typedef struct {
    char red;
    char green;
    char blue;
    int character; // this the character to be written out
}cellInfo;

#ifdef _WIN32
typedef struct {
    CHAR_INFO* char_buffer;
    size_t size;
}screenBuffer;
#endif

void clearScreen() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SMALL_RECT scrollRect;
    COORD scrollTarget;
    CHAR_INFO fill;

    // Get the number of character cells in the current buffer.
    if (!GetConsoleScreenBufferInfo(handle_stdout, &csbi))
    {
        return;
    }

    scrollRect.Left = 0;
    scrollRect.Top = 0;
    scrollRect.Right = csbi.dwSize.X;
    scrollRect.Bottom = csbi.dwSize.Y;

    scrollTarget.X = 0;
    scrollTarget.Y = (SHORT)(0 - csbi.dwSize.Y);

    fill.Char.UnicodeChar = TEXT(' ');
    fill.Attributes = csbi.wAttributes;

    ScrollConsoleScreenBuffer(handle_stdout, &scrollRect, NULL, scrollTarget, &fill);

    csbi.dwCursorPosition.X = 0;
    csbi.dwCursorPosition.Y = 0;

    SetConsoleCursorPosition(handle_stdout, csbi.dwCursorPosition);
}

void draw_to_screen(int* buffer, int objColor, int bgColor, int width, int height)
{
    static clock_t first;
    static clock_t currTime;
    static int frames = 0;
    static int fps = 0;
    static screenBuffer screen;
    //if (first == 0) {
    //    first = clock();
    //}
    frames++;
    //clearScreen(); // causes flickering not really ideal, instead drawling entire buffer bascially scrolls it down
    if (screen.char_buffer == NULL) {
        screen.char_buffer = malloc((size_t)width * height * sizeof(CHAR_INFO));
        screen.size = (size_t)width * height;
    }
    if (screen.size < (size_t)width * height) {
        void* ptr = realloc(screen.char_buffer, sizeof(CHAR_INFO) * ((size_t)width * height));
        if (!ptr) {
            fprintf(stderr, "OOM in DRAW to screen call\n"); \
            exit(5);
            return;
        }
        screen.char_buffer = ptr;
        screen.size = (size_t)width * height;
    }
    CHAR_INFO* char_buffer = screen.char_buffer;
    if (!char_buffer) {
        fprintf(stderr, "OOM in draw to screen call\n");
        exit(5);
        return;
    }
    for (int i = 0; i < width * height; i++) {
        char_buffer[i].Char.AsciiChar = ' ';
        if (buffer[i] != 0) {
            char_buffer[i].Attributes = (BACKGROUND_BLUE | BACKGROUND_RED | BACKGROUND_GREEN); // white
        }
        else {
            char_buffer[i].Attributes = 0; // black
        }
    }
    currTime = clock();
    if ((double)(currTime - first) / CLOCKS_PER_SEC >= 1.0) {
        // take upper right corner and add fps:000
        fps = frames;
        frames = 0;
        first = currTime;
    }
    if (width >= 8) {
        char conv[10];
        snprintf(conv, sizeof(conv), "%04d", fps);
        char_buffer[width - 1 - 7].Char.AsciiChar = 'f';
        char_buffer[width - 1 - 6].Char.AsciiChar = 'p';
        char_buffer[width - 1 - 5].Char.AsciiChar = 's';
        char_buffer[width - 1 - 4].Char.AsciiChar = ':';
        char_buffer[width - 1 - 3].Char.AsciiChar = conv[0];
        char_buffer[width - 1 - 2].Char.AsciiChar = conv[1];
        char_buffer[width - 1 - 1].Char.AsciiChar = conv[2];
        char_buffer[width - 1 - 0].Char.AsciiChar = conv[3];
        char_buffer[width - 1 - 7].Attributes = FOREGROUND_BLUE;
        char_buffer[width - 1 - 6].Attributes = FOREGROUND_BLUE;
        char_buffer[width - 1 - 5].Attributes = FOREGROUND_BLUE;
        char_buffer[width - 1 - 4].Attributes = FOREGROUND_BLUE;
        char_buffer[width - 1 - 3].Attributes = FOREGROUND_BLUE;
        char_buffer[width - 1 - 2].Attributes = FOREGROUND_BLUE;
        char_buffer[width - 1 - 1].Attributes = FOREGROUND_BLUE;
        char_buffer[width - 1 - 0].Attributes = FOREGROUND_BLUE;
    }
    COORD bufferSize = { width, height };
    COORD loc = { 0,0 };
    SMALL_RECT region = { 0,0,0 + width - 1,0 + height - 1 };
    WriteConsoleOutput(handle_stdout, char_buffer, bufferSize, loc, &region);
}
