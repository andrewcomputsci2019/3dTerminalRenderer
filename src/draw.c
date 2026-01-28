#include <3dTerminal/draw.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
	#include <3dTerminal/handles.h>
extern HANDLE handle_stdin, handle_stdout;
#endif // _WIN32


#ifdef _WIN32
typedef struct {
    CHAR_INFO* char_buffer;
    size_t size;
}screenBuffer;
#endif

typedef struct {
    char* data;
    size_t len;
    size_t cap;
}text_buffer;

static void ensure_text_capacity(text_buffer* buf, size_t needed)
{
    if (needed <= buf->cap) {
        return;
    }
    size_t new_cap = buf->cap ? buf->cap : 256;
    while (new_cap < needed) {
        new_cap *= 2;
    }
    char* tmp = (char*)realloc(buf->data, new_cap);
    if (!tmp) {
        fprintf(stderr, "OOM in draw_to_screen_unicode\n");
        exit(5);
    }
    buf->data = tmp;
    buf->cap = new_cap;
}

static void append_bytes(text_buffer* buf, const char* src, size_t count)
{
    ensure_text_capacity(buf, buf->len + count + 1);
    memcpy(buf->data + buf->len, src, count);
    buf->len += count;
    buf->data[buf->len] = '\0';
}

static void append_char(text_buffer* buf, char value)
{
    ensure_text_capacity(buf, buf->len + 2);
    buf->data[buf->len++] = value;
    buf->data[buf->len] = '\0';
}

static void append_utf8(text_buffer* buf, unsigned int codepoint)
{
    char tmp[4];
    size_t count = 0;
    if (codepoint <= 0x7Fu) {
        tmp[count++] = (char)codepoint;
    }
    else if (codepoint <= 0x7FFu) {
        tmp[count++] = (char)(0xC0u | (codepoint >> 6));
        tmp[count++] = (char)(0x80u | (codepoint & 0x3Fu));
    }
    else if (codepoint <= 0xFFFFu) {
        tmp[count++] = (char)(0xE0u | (codepoint >> 12));
        tmp[count++] = (char)(0x80u | ((codepoint >> 6) & 0x3Fu));
        tmp[count++] = (char)(0x80u | (codepoint & 0x3Fu));
    }
    else {
        tmp[count++] = (char)(0xF0u | (codepoint >> 18));
        tmp[count++] = (char)(0x80u | ((codepoint >> 12) & 0x3Fu));
        tmp[count++] = (char)(0x80u | ((codepoint >> 6) & 0x3Fu));
        tmp[count++] = (char)(0x80u | (codepoint & 0x3Fu));
    }
    append_bytes(buf, tmp, count);
}

static void append_color_sequence(text_buffer* buf, unsigned char r, unsigned char g, unsigned char b)
{
    char tmp[32];
    int count = snprintf(tmp, sizeof(tmp), "\x1b[38;2;%u;%u;%um", (unsigned int)r, (unsigned int)g, (unsigned int)b);
    if (count > 0) {
        append_bytes(buf, tmp, (size_t)count);
    }
}

static unsigned int quadrant_codepoint(unsigned int mask)
{
    static const unsigned int map[16] = {
        0x20u,   /* empty */
        0x2598u, /* upper left */
        0x259Du, /* upper right */
        0x2580u, /* upper half */
        0x2596u, /* lower left */
        0x258Cu, /* left half */
        0x259Eu, /* upper right + lower left */
        0x259Bu, /* upper left + upper right + lower left */
        0x2597u, /* lower right */
        0x259Au, /* upper left + lower right */
        0x2590u, /* right half */
        0x259Cu, /* upper left + upper right + lower right */
        0x2584u, /* lower half */
        0x2599u, /* upper left + lower left + lower right */
        0x259Fu, /* upper right + lower left + lower right */
        0x2588u  /* full block */
    };
    return map[mask & 0xFu];
}

static unsigned int braille_codepoint_from_mask(unsigned int mask)
{
    if ((mask & 0xFFu) == 0) {
        return 0x20u;
    }
    unsigned int dots = 0;
    if (mask & (1u << 0)) {
        dots |= BRAILLE_1;
    }
    if (mask & (1u << 1)) {
        dots |= BRAILLE_4;
    }
    if (mask & (1u << 2)) {
        dots |= BRAILLE_2;
    }
    if (mask & (1u << 3)) {
        dots |= BRAILLE_5;
    }
    if (mask & (1u << 4)) {
        dots |= BRAILLE_3;
    }
    if (mask & (1u << 5)) {
        dots |= BRAILLE_6;
    }
    if (mask & (1u << 6)) {
        dots |= BRAILLE_7;
    }
    if (mask & (1u << 7)) {
        dots |= BRAILLE_8;
    }
    return 0x2800u + dots;
}

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
        if (buffer[i] > 0) {
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




void draw_to_screen_unicode_mode1(cellInfo* buffer, int width, int height) {
    static text_buffer out;
    static clock_t first;
    static clock_t currTime;
    static int frames = 0;
    static int fps = 0;
    char fps_text[9];
    size_t approx = (size_t)width * (size_t)height * 12 + (size_t)height + 64;
    if (approx > out.cap) {
        ensure_text_capacity(&out, approx);
    }
    out.len = 0;
    append_bytes(&out, "\x1b[H", 3);

    frames++;
    if (first == 0) {
        first = clock();
    }
    currTime = clock();
    if ((double)(currTime - first) / CLOCKS_PER_SEC >= 1.0) {
        fps = frames;
        frames = 0;
        first = currTime;
    }
    if (width >= 8) {
        snprintf(fps_text, sizeof(fps_text), "fps:%04d", fps);
    }

    int last_color = -1;
    const unsigned int fps_color = 0x0000FFu;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (width >= 8 && y == 0 && x >= (width - 8)) {
                if ((int)fps_color != last_color) {
                    append_color_sequence(&out, 0, 0, 255);
                    last_color = (int)fps_color;
                }
                append_char(&out, fps_text[x - (width - 8)]);
                continue;
            }
            cellInfo* cell = &buffer[y * width + x];
            unsigned int mask = cell->characterCode & 0xFu;
            if (mask == 0) {
                append_char(&out, ' ');
                continue;
            }
            unsigned int color = ((unsigned int)cell->Color.color) & 0xFFFFFFu;
            if ((int)color != last_color) {
                append_color_sequence(&out,
                    (unsigned char)cell->Color.Channel.red,
                    (unsigned char)cell->Color.Channel.green,
                    (unsigned char)cell->Color.Channel.blue);
                last_color = (int)color;
            }
            append_utf8(&out, quadrant_codepoint(mask));
        }
        append_char(&out, '\n');
    }
    append_bytes(&out, "\x1b[0m", 4);
    fwrite(out.data, 1, out.len, stdout);
    fflush(stdout);
}

void draw_to_screen_unicode_mode2(cellInfo* buffer, int width, int height) {
    static text_buffer out;
    static clock_t first;
    static clock_t currTime;
    static int frames = 0;
    static int fps = 0;
    char fps_text[9];
    size_t approx = (size_t)width * (size_t)height * 12 + (size_t)height + 64;
    if (approx > out.cap) {
        ensure_text_capacity(&out, approx);
    }
    out.len = 0;
    append_bytes(&out, "\x1b[H", 3);
    frames++;
    if (first == 0) {
        first = clock();
    }
    currTime = clock();
    if ((double)(currTime - first) / CLOCKS_PER_SEC >= 1.0) {
        fps = frames;
        frames = 0;
        first = currTime;
    }
    if (width >= 8) {
        snprintf(fps_text, sizeof(fps_text), "fps:%04d", fps);
    }

    int last_color = -1;
    const unsigned int fps_color = 0x0000FFu;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (width >= 8 && y == 0 && x >= (width - 8)) {
                if ((int)fps_color != last_color) {
                    append_color_sequence(&out, 0, 0, 255);
                    last_color = (int)fps_color;
                }
                append_char(&out, fps_text[x - (width - 8)]);
                continue;
            }
            cellInfo* cell = &buffer[y * width + x];
            unsigned int mask = cell->characterCode & 0xFFu;
            if (mask == 0) {
                append_char(&out, ' ');
                continue;
            }
            unsigned int color = ((unsigned int)cell->Color.color) & 0xFFFFFFu;
            if ((int)color != last_color) {
                append_color_sequence(&out,
                    (unsigned char)cell->Color.Channel.red,
                    (unsigned char)cell->Color.Channel.green,
                    (unsigned char)cell->Color.Channel.blue);
                last_color = (int)color;
            }
            append_utf8(&out, braille_codepoint_from_mask(mask));
        }
        append_char(&out, '\n');
    }
    append_bytes(&out, "\x1b[0m", 4);
    fwrite(out.data, 1, out.len, stdout);
    fflush(stdout);
}

void draw_to_screen_unicode(cellInfo* buffer, int width, int height, enum text_mode mode)
{
    switch (mode) {
    case quater_block:
        draw_to_screen_unicode_mode1(buffer, width, height);
        break;
    case braille:
        draw_to_screen_unicode_mode2(buffer, width, height);
        break;
    default:
        draw_to_screen_unicode_mode1(buffer, width, height);
    }
}
