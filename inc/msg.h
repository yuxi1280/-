#ifndef __MSG_H_
#define __MSG_H_

#include "system_hc32f460.h"

#define MSG_FUNC_MAX 128
#define MSG_ID_LIST_MAX 512 //must be 2^n

#if (MSG_FUNC_MAX > 256)
#define MSG_ID_SIZE uint16_t
#else
#define MSG_ID_SIZE uint8_t
#endif

enum {
	MSG_ID_NULL = 0,
	MSG_ID_DEBUG_RX_EXEC,
};

typedef void (*msg_exec_func_ptr)(void *arg);

typedef struct {
	MSG_ID_SIZE id;
	void *arg_ptr;
}msg_id_queue_t;

typedef struct {
	msg_id_queue_t queue[MSG_ID_LIST_MAX];
	uint32_t head;
	uint32_t tail;
}msg_id_list_t;

bool msg_queue_push(uint32_t id, void *arg);
msg_id_queue_t *msg_queue_pop(void);
bool msg_func_register(MSG_ID_SIZE id, msg_exec_func_ptr func);
void msg_poll(void);

#endif


