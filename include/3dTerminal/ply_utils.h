#pragma once
#include <3dTerminal/render.h>

typedef struct {
	unsigned short has_vertex_colors; // > 0 for yes expects rgb in form of 0-255
}plyLayout;

object* get_object_from_ply(const char* plyFile, plyLayout def);
