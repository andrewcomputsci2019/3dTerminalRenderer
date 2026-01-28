#pragma once
#include<stdlib.h>
#include<stdio.h>
#include<3dTerminal/events.h>





typedef struct InputBuffer_t{
	unsigned char* data;
	int size;
	int capacity;
	int head;
	int tail;
}InputBuffer;




typedef struct MOUSE_EVENT_QUEUE_t{
	EVENT_MOUSE* buffer;
	int size;
	int capacity;
	int head;
	int tail;
}MOUSE_EVENT_QUEUE;









bool addToBuffer(InputBuffer * queue, unsigned char data) {
	if (queue->capacity == queue->size) {
		return false;
	}
	queue->data[queue->tail] = data;
	queue->tail = (1 + queue->tail)  % queue->capacity;
	queue->size += 1;
	return true;
}


unsigned char getFirst(InputBuffer* queue) {
	if (queue->size == 0) {
		fprintf(stderr, "Input Buffer has empty queue, yet was accessed\n");
		return 0;
	}
	unsigned char val = queue->data[queue->head];
	queue->size -= 1;
	queue->head = (1 + queue->head) % queue->capacity;
	return val;
}

char peekFirst(InputBuffer* queue) {
	if (queue->size == 0) {
		fprintf(stderr, "Input Buffer has empty queue, yet was accessed\n");
		return 0;
	}
	return queue->data[queue->head];
}

bool isAvail(InputBuffer* queue) {
	return queue->size > 0;
}

void createBuffer(InputBuffer * buffer, int capacity) {
	if (buffer->data != NULL) {
		fprintf(stderr, "Input Buffer struct passed in has already been initalized\n");
	}
	buffer->data = (char*)malloc(sizeof(unsigned char) * capacity);
	buffer->size = 0;
	buffer->capacity = capacity;
	buffer->head = 0;
	buffer->tail = 0;
	return;
}


bool enqueue_mouse_event(MOUSE_EVENT_QUEUE* queue, EVENT_MOUSE event) {
	if (queue->size == queue->capacity) {
		return false;
	}
	queue->buffer[queue->tail] = event;
	queue->size += 1;
	queue->tail = (queue->tail + 1) % queue->capacity;
	return true;
}

EVENT_MOUSE pop_mouse_event(MOUSE_EVENT_QUEUE* queue) {
	if (queue->size == 0) {
		fprintf(stderr, "Mouse event does not have any data in it, yet it was accessed as if it did\n");
		programExit(2);
	}
	EVENT_MOUSE data = queue->buffer[queue->head];
	queue->head = (queue->head + 1) % queue->capacity;
	queue->size -= 1;
	return data;
}


EVENT_MOUSE peeek_mouse_event(MOUSE_EVENT_QUEUE* queue) {
	if (queue->size == 0) {
		fprintf(stderr, "Mouse event does not have any data in it, yet it was accessed as if it did\n");
		programExit(2);
	}
	return queue->buffer[queue->head];
}

bool MOUSE_EVENT_QUEUE_isEmpty(MOUSE_EVENT_QUEUE* queue) {
	return queue->size == 0;
}

void init_MOUSE_EVENT_QUEUE(MOUSE_EVENT_QUEUE* queue, int capacity) {
	queue->buffer = (EVENT_MOUSE*)malloc(sizeof(EVENT_MOUSE) * capacity);
	queue->size = 0;
	queue->capacity = capacity;
	queue->head = 0;
	queue->tail = 0;
}