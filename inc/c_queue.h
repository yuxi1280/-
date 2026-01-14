#ifndef __C_QUEUE_H_
#define __C_QUEUE_H_

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"
#include "mix_std_lib.h"

/*
 * 1、使用流程：先定义结构体type，用C_QUEUE_DECLARE声明，用c_queue_[type]_t实例化对象，调用c_queue_[type]_init初始化后可使用。
 * 2、结构体type中不建议有指针，有的话需要注意浅拷贝的问题（原指针和拷贝的指针指向同一目标，发生同步更改）。
 */

enum {
	C_QUEUE_OK = 0,
	C_QUEUE_ERROR,
	C_QUEUE_POPING_EMPTY,
	C_QUEUE_PUSHING_FULL,
	C_QUEUE_PUSHING_SIZE_ERROR,
	C_QUEUE_EMPTY,
};

#define C_QUEUE_DECLARE(type, capacity) \
typedef struct { \
	type data[capacity]; \
	volatile uint32_t head; \
	volatile uint32_t tail; \
	volatile uint32_t size; \
}c_queue_##type##_t; \
\
void c_queue_##type##_init(c_queue_##type##_t *q) \
{ \
	__msl_memclr(q->data, sizeof(q->data)); \
	q->head = 0; \
	q->tail = 0; \
	q->size = 0; \
} \
\
int32_t c_queue_##type##_empty(c_queue_##type##_t *q) \
{ \
    return q->size == 0; \
} \
\
int32_t c_queue_##type##_full(c_queue_##type##_t *q) \
{ \
    return capacity == q->size; \
} \
\
int32_t c_queue_##type##_push(c_queue_##type##_t *q, type *value) \
{ \
	if (c_queue_##type##_full(q)) { \
		return C_QUEUE_PUSHING_FULL; \
	} \
	q->data[q->tail] = *value; \
	q->tail = (q->tail + 1) % capacity; \
	q->size++; \
	return C_QUEUE_OK; \
} \
\
int32_t c_queue_##type##_pop(c_queue_##type##_t *q) \
{ \
	if (c_queue_##type##_empty(q)) { \
		return C_QUEUE_POPING_EMPTY; \
	} \
	q->head = (q->head + 1) % capacity; \
	q->size--; \
	return C_QUEUE_OK; \
} \
\
int32_t c_queue_##type##_front(c_queue_##type##_t *q, type *p) \
{ \
	if (c_queue_##type##_empty(q)) { \
		return C_QUEUE_EMPTY; \
	} \
	*p = q->data[q->head]; \
	return C_QUEUE_OK; \
} \
\
int32_t c_queue_##type##_size(c_queue_##type##_t *q) \
{ \
	return (q->tail - q->head + capacity) % capacity; \
}


#endif


