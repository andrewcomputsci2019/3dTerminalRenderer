#include <3dTerminal/ply_utils.h>
#include <rply.h>
#include <stdlib.h>

typedef struct Parser_Wrapper_t {
	object* ply_object;
	size_t vertex_idx;
	size_t face_idx;
}Parser_Wrapper;

enum COLOR_e {
	RED,
	GREEN,
	BLUE
};

enum VERTEX_e {
	X,
	Y,
	Z
};



#define EOL_FLAG_PLY (1<<7)


// callback functions
static int vertex_cb(p_ply_argument argument) {
	Parser_Wrapper* wrapper = NULL;
	long arg;
	ply_get_argument_user_data(argument, &wrapper, &arg);
	if (!wrapper) {
		return 0;
	}
	unsigned int eol_marker = arg & EOL_FLAG_PLY;
	enum VERTEX_e vertex = 0;
	vertex = arg & (0b11);
	double value = ply_get_argument_value(argument);
	switch (vertex) {
	case X: 
		wrapper->ply_object->mesh[wrapper->vertex_idx][0] = (float)value;
		break;
	case Y:
		wrapper->ply_object->mesh[wrapper->vertex_idx][1] = (float)value;
		break;
	case Z:
		wrapper->ply_object->mesh[wrapper->vertex_idx][2] = (float)value;
		break;
	}
	if (eol_marker) {
		wrapper->vertex_idx++;
	}
	return 1;
}

static int vertex_colors_cb(p_ply_argument argument) {
	Parser_Wrapper* wrapper = NULL;
	long arg;
	ply_get_argument_user_data(argument, &wrapper, &arg);
	if (!wrapper) {
		return 0;
	}
	unsigned int eol_marker = arg & EOL_FLAG_PLY;
	enum COLOR_e color;
	color = arg & (0b11);
	double value = ply_get_argument_value(argument);
	switch (color) {
	case RED:
		wrapper->ply_object->vertex_color[wrapper->vertex_idx][0] = (int)value;
		break;
	case GREEN:
		wrapper->ply_object->vertex_color[wrapper->vertex_idx][1] = (int)value;
		break;
	case BLUE:
		wrapper->ply_object->vertex_color[wrapper->vertex_idx][2] = (int)value;
		break;
	}
	if (eol_marker) {
		wrapper->vertex_idx++;
	}
	return 1;
}

static int face_cb(p_ply_argument argument) {
	long length, value_index;
	double value;
	Parser_Wrapper* wrapper = NULL;
	ply_get_argument_user_data(argument, &wrapper, NULL);
	if (!wrapper) {
		return 0;
	}
	ply_get_argument_property(argument, NULL, &length, &value_index);
	value = ply_get_argument_value(argument);
	switch (value_index) {
	case 0:
	case 1:
		wrapper->ply_object->faceList[wrapper->face_idx][value_index] = (int)value;
		break;
	case 2:
		wrapper->ply_object->faceList[wrapper->face_idx][value_index] = (int)value;
		wrapper->face_idx++;
	default:
		break;
	}
	return 1;
}



object* get_object_from_ply(const char* plyFile, plyLayout def)
{
	object* ply_object = (object*)_aligned_malloc(sizeof(object),16);
	if (!ply_object) {
		return NULL;
	}
	reset_scale_vector(ply_object);
	Parser_Wrapper wrapper = { 0 };
	wrapper.ply_object = ply_object;
	wrapper.face_idx = 0;
	wrapper.vertex_idx = 0;
	p_ply ply = ply_open(plyFile, NULL, 0, NULL);
	if (!ply) return NULL;
	if (!ply_read_header(ply)) return NULL;
	long nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, &wrapper, X);
	ply_set_read_cb(ply, "vertex", "y", vertex_cb, &wrapper, Y);
	ply_set_read_cb(ply, "vertex", "z", vertex_cb, &wrapper, def.has_vertex_colors > 0 ? Z : Z | EOL_FLAG_PLY);
	if (def.has_vertex_colors) {
		ply_set_read_cb(ply, "vertex", "red", vertex_colors_cb,  &wrapper, RED);
		ply_set_read_cb(ply, "vertex", "green", vertex_colors_cb,&wrapper, GREEN);
		ply_set_read_cb(ply, "vertex", "blue", vertex_colors_cb, &wrapper, BLUE | EOL_FLAG_PLY);
	}
	long nfaces = ply_set_read_cb(ply, "face", "vertex_indices", face_cb, &wrapper, 0);
	ply_object->faceListLength = (size_t)nfaces;
	ply_object->meshSize = (size_t)nvertices;
	ply_object->faceList = _aligned_malloc(sizeof(ivec3) * (size_t)nfaces, 16);
	ply_object->mesh = _aligned_malloc(sizeof(vec3) * (size_t)nvertices, 16);
	ply_object->has_vertex_color = def.has_vertex_colors;
	if (ply_object->has_vertex_color) {
		ply_object->vertex_color = _aligned_malloc(sizeof(ivec3) * (size_t)nvertices, 16);
	}
	if (!ply_read(ply)) {
		return NULL;
	}
	ply_close(ply);
	return ply_object;
}
