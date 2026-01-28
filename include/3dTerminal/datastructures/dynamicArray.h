#pragma once
#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
#include<3dTerminal/render.h>


typedef struct dynamicFaceArray_t {
	size_t size;
	size_t cap;
	ivec3** data;
}dynamicFaceArray;

inline void dynamicFaceArray_add(dynamicFaceArray * array, ivec3* data) {
	if (array->cap == array->size) {
		ivec3** tmp = (ivec3**)realloc(array->data, sizeof(ivec3*) * array->cap * 2);
		if (!tmp) {
			fprintf(stderr,"OOM inside dynamic array was unable to expand dyanmic array\n");
			exit(4);
		}
		array->data = tmp; // set data to point to a new memory region
		array->cap *= 2;
	}
	array->data[array->size] = data;
	array->size += 1;
}

inline void dynamicFaceArray_create(dynamicFaceArray* array) {
	array->cap = 10;
	array->size = 0;
	array->data = (ivec3**)malloc(sizeof(ivec3*) * 10);
	if (!array->data) {
		fprintf(stderr,"OOM inside dynamic array was unable to expand dyanmic array\n");
		exit(4);
	}
}
// frees only internal structure memory, user allocated memory will need to be free by themselves
inline void dynamicFaceArray_destroy(dynamicFaceArray* array) {
	free(array->data);
	array->data = NULL;
}