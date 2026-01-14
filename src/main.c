#include "system_hc32f460.h"
#include "io.h"
#include "rs485.h"
#include "encoder.h"
#include "moto.h"
#include "pid_pos.h"
#include "soft_timer.h"

void _pid_update(void)
{
	int32_t encoder_e;
	float u;

	encoder_e = encoder_get_e();
	pid_pos_update_pos(encoder_e);
	u = pid_pos_cal();
	moto_set_pwm((int32_t)u);
}

int main(void)
{
	stimer_t st;

	stimer_init();
	io_init();
	rs485_init();
	// rs485_tx_str("Hello world!\r\n");
	encoder_init();
	moto_init();
	pid_pos_init();

	st.id = 1;
	st.statu = 0;
	st.period = 100;
	st.handle = _pid_update;
	stimer_register(&st);
	stimer_ctrl(1, true, STIMER_STATU_POS_ENABLE | STIMER_STATU_POS_PERIOD);

	for (;;) {
		rs485_poll();
		stimer_poll();
	}
}

