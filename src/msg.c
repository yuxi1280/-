#include "msg.h"

static msg_id_list_t msg_id_l = {0};
static msg_exec_func_ptr msg_func_list[MSG_FUNC_MAX] = {0};

bool msg_queue_push(uint32_t id, void *arg)
{
	uint32_t tail;

	tail = (msg_id_l.tail + 1) & (MSG_ID_LIST_MAX - 1);
	if (tail == msg_id_l.head) {
		return false;
	}
	msg_id_l.tail = tail;
	msg_id_l.queue[tail].id = id;
	msg_id_l.queue[tail].arg_ptr = arg;

	return true;
}

msg_id_queue_t *msg_queue_pop(void)
{
	uint32_t head;

	if (msg_id_l.head == msg_id_l.tail) {
		return NULL;
	}
	head = msg_id_l.head;
	msg_id_l.head = ((head + 1) & (MSG_ID_LIST_MAX - 1));
	
	return &msg_id_l.queue[head];
}

bool msg_func_register(MSG_ID_SIZE id, msg_exec_func_ptr func)
{
	if (id >= MSG_FUNC_MAX || (msg_func_list[id] != NULL)) {
		return false;
	}
	msg_func_list[id] = func;

	return true;
}

void msg_poll(void)
{
	msg_id_queue_t *msg;
	uint32_t id;

	for (;;) {
		msg = msg_queue_pop();
		id = msg->id;
		if (msg && (id < MSG_FUNC_MAX)) {
			if (msg_func_list[id] != NULL) {
				msg_func_list[id](msg->arg_ptr);
			}
		} else {
			break;
		}
	}
}


