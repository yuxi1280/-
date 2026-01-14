#include "mix_std_lib.h"

typedef char * __msl_va_list;

#define __msl_va_start(vl, arg) (vl = (__msl_va_list)&arg + sizeof(arg))
#define __msl_va_arg(vl, t) (*(t *)((vl += sizeof(t)) - sizeof(t)))
#define __msl_va_end(vl) (vl = (__msl_va_list)0)

#define __MSL_SYSTEM_BIT_32

#ifndef __MSL_SYSTEM_BIT_32
#define __MSL_SYSTEM_BIT_64
#endif

#ifdef __MSL_SYSTEM_BIT_32
	#define __msl_int64_t int32_t
	#define __msl_uint64_t uint32_t
#else
	#define __msl_int64_t int64_t
	#define __msl_uint64_t uint64_t
#endif

void __msl_memset(void *mem, uint8_t value, uint32_t len)
{
	uint8_t *p = (uint8_t *)mem;

	while (len--) {
		*(p++) = value;
	}
}

void __msl_memclr(void *mem, uint32_t len)
{
	uint8_t *p = (uint8_t *)mem;

	while (len--) {
		*(p++) = 0x00;
	}
}

void __msl_memcpy(void *dst, void *src, uint32_t len)
{
	uint8_t *d = (uint8_t *)dst;
	uint8_t *s = (uint8_t *)src;

	while (len--) {
		*(d++) = *(s++);
	}
}

int8_t __msl_memcmp(void *mem1, void *mem2, uint32_t len)
{
	uint8_t *d = (uint8_t *)mem1;
	uint8_t *s = (uint8_t *)mem2;

	while (len--) {
		if (*d > *s) {
			return 1;
		} else if (*d < *s) {
			return -1;
		}
		d++;
		s++;
	}

	return 0;
}

uint32_t __msl_strlen(char *str)
{
	uint32_t len;

	len = 0;
	while (*(str++) != '\0') {
		len++;
	}

	return len;
}

char * __msl_strstr(char *str1, char *str2)
{
	char *p1, *p2;

	if (__msl_strlen(str1) < __msl_strlen(str2)) {
		return NULL;
	}
	p2 = str2;
	while (*(str1) != '\0') {
		p1 = str1;
		while (*p1 == *p2 && *p1 && *p2) {
			p1++;
			p2++;
			if (*p2 == '\0') {
				return str1;
			}
		}
		p2 = str2;
		str1++;
	}

	return NULL;
}

///////////for printf////////////

#define __MSL_PRINTF_BUF_MAX 256

__attribute__((weak)) extern void __mslex_usart_write(uint8_t *dat);

// void __mslex_usart_write(uint8_t dat)
// {
//
// }

const char hex_char[16] = {
	'0', '1', '2', '3',
	'4', '5', '6', '7',
	'8', '9', 'A', 'B',
	'C', 'D', 'E', 'F'
};

enum
{
	_MSL_PRINTF_FLAG_ADD		= 0x01,
	_MSL_PRINTF_FLAG_SUB		= 0x02,
	_MSL_PRINTF_FLAG_SPACE		= 0x04,
	_MSL_PRINTF_FLAG_ZERO		= 0x08,
	_MSL_PRINTF_FLAG_POUND		= 0x10,
};

typedef struct
{
	uint32_t width;
	char length;
	uint8_t flag;
	uint32_t presition;
}__msl_printf_format_t;

typedef struct
{
	uint8_t dat[__MSL_PRINTF_BUF_MAX];
	uint32_t pos;
}__msl_printf_buf_t;

static void int_inverse(
		__msl_printf_buf_t *dst_buf,
		__msl_printf_format_t *p_fmt,
		uint32_t head_pos)
{
	uint32_t tmp_pos = dst_buf->pos - 1;
	if (p_fmt->flag & _MSL_PRINTF_FLAG_SUB) {
		while (dst_buf->dat[tmp_pos] == ' ') {
			tmp_pos--;
		}
	}
	char ch;
	while (tmp_pos > head_pos) {
		ch = dst_buf->dat[head_pos];
		dst_buf->dat[head_pos] = dst_buf->dat[tmp_pos];
		dst_buf->dat[tmp_pos] = ch;
		tmp_pos--;
		head_pos++;
	}
}

// extern u8 drv_uart_tx_start(u8 *data, u32 len);

void __msl_printf(const char *fmt_src, ...)
{
	__msl_printf_format_t p_fmt;
	__msl_printf_buf_t dst_buf;
	uint8_t *p;
	char *fmt;
//	uint32_t fmt_len;
	uint32_t head_pos;
	uint8_t f;
	__msl_va_list vl;

	__msl_va_start(vl, fmt_src);
	fmt = (char *)fmt_src;
	dst_buf.pos = 0;
	while (*fmt) {
		if (*fmt == '%') {
			fmt++;
			__msl_memclr(&p_fmt, sizeof(p_fmt));
			for (;;) {
				switch (*fmt) {
				case '+':
					p_fmt.flag |= _MSL_PRINTF_FLAG_ADD;
					break;
				case '-':
					p_fmt.flag |= _MSL_PRINTF_FLAG_SUB;
					break;
				case '#':
					p_fmt.flag |= _MSL_PRINTF_FLAG_POUND;
					break;
				case ' ':
					p_fmt.flag |= _MSL_PRINTF_FLAG_SPACE;
					break;
				case '0':
					p_fmt.flag |= _MSL_PRINTF_FLAG_ZERO;
					break;
				case 'l':
					if (*(fmt - 1) == 'l') {
						p_fmt.length = 'w';
					} else {
						p_fmt.length = 'l';
					}
					break;
				case 'h':
					p_fmt.length = 'h';
					break;
				case '1':case '2':case '3':case '4':case '5':
				case '6':case '7':case '8':case '9':case '.':case '*':
					if (*fmt == '*') {
						p_fmt.width = __msl_va_arg(vl, uint32_t);
						fmt++;
					}
					for (;;) {
						if (*fmt <= '9' && *fmt >= '0') {
							p_fmt.width = p_fmt.width * 10 + (*fmt - '0');
						} else if (*fmt == '.') {
							fmt++;
							if (*fmt == '*') {
								p_fmt.presition = __msl_va_arg(vl, uint32_t);
								fmt++;
							}
							for (;;) {
								if (*fmt <= '9' && *fmt >= '0') {
									p_fmt.presition = p_fmt.presition * 10 + (*fmt - '0');
								} else {
									fmt--;
									break;
								}
								fmt++;
							}
						} else {
							fmt--;
							break;
						}
						fmt++;
					}
					break;
				default:
					goto a1;
				}
				fmt++;
			}
a1:			f = 0;
			switch (*fmt) {
			case 'i':
			case 'd': 
			case 'u': {
				head_pos = dst_buf.pos;
				__msl_uint64_t d_dat = __msl_va_arg(vl, __msl_uint64_t);
				if (d_dat == 0) {
					dst_buf.dat[dst_buf.pos++] = '0';
				} else {
					if (*fmt == 'u') {
						uint32_t ud_32 = (uint32_t)d_dat;
						uint16_t ud_16 = (uint16_t)d_dat;
						switch (p_fmt.length) {
						case 'w':
							while (d_dat != 0) {
								dst_buf.dat[dst_buf.pos++] = (d_dat % 10) + '0';
								d_dat /= 10;
							}
							break;
						case 'h':
							while (ud_16 != 0) {
								dst_buf.dat[dst_buf.pos++] = (ud_16 % 10) + '0';
								ud_16 /= 10;
							}
							break;
						default:
							while (ud_32 != 0) {
								dst_buf.dat[dst_buf.pos++] = (ud_32 % 10) + '0';
								ud_32 /= 10;
							}
							break;
						}
					} else {
						__msl_int64_t d_64 = *((__msl_int64_t*)&d_dat);
						int32_t d_32 = *((int32_t*)&d_dat);
						int16_t d_16 = *((int16_t*)&d_dat);
						switch (p_fmt.length) {
						case 'w':
							if (d_64 < 0) {
								d_64 = -d_64;
								f = 1;
							}
							while (d_64 != 0) {
								dst_buf.dat[dst_buf.pos++] = (d_64 % 10) + '0';
								d_64 /= 10;
							}
							break;
						case 'h':
							if (d_16 < 0) {
								d_16 = -d_16;
								f = 1;
							}
							while (d_16 != 0) {
								dst_buf.dat[dst_buf.pos++] = (d_16 % 10) + '0';
								d_16 /= 10;
							}
							break;
						default:
							if (d_32 < 0) {
								d_32 = -d_32;
								f = 1;
							}
							while (d_32 != 0) {
								dst_buf.dat[dst_buf.pos++] = (d_32 % 10) + '0';
								d_32 /= 10;
							}
							break;
						}
					}
				}
				if (p_fmt.presition != 0) {
					while (dst_buf.pos - head_pos < p_fmt.presition) {
						dst_buf.dat[dst_buf.pos++] = '0';
					}
					p_fmt.flag &= ~_MSL_PRINTF_FLAG_ZERO;
				}
				if ((p_fmt.flag & _MSL_PRINTF_FLAG_ZERO)
						&& !(p_fmt.flag & _MSL_PRINTF_FLAG_SUB)) {
					while (dst_buf.pos - head_pos < p_fmt.width) {
						dst_buf.dat[dst_buf.pos++] = '0';
					}
					if (f) {
						dst_buf.dat[dst_buf.pos - 1] = '-';
					} else {
						if (p_fmt.flag & _MSL_PRINTF_FLAG_ADD) {
							dst_buf.dat[dst_buf.pos - 1] = '+';
						} else if (p_fmt.flag & _MSL_PRINTF_FLAG_SPACE) {
							dst_buf.dat[dst_buf.pos - 1] = ' ';
						}
					}
				} else {
					if (f) {
						dst_buf.dat[dst_buf.pos++] = '-';
					} else {
						if (p_fmt.flag & _MSL_PRINTF_FLAG_ADD) {
							dst_buf.dat[dst_buf.pos++] = '+';
						} else if (p_fmt.flag & _MSL_PRINTF_FLAG_SPACE) {
							dst_buf.dat[dst_buf.pos++] = ' ';
						}
					}
					while (dst_buf.pos - head_pos < p_fmt.width) {
						dst_buf.dat[dst_buf.pos++] = ' ';
					}
				}
				int_inverse(&dst_buf, &p_fmt, head_pos);
				break;
			}
			case 's': {
				char *str;
				uint32_t tmp_len;
				str = __msl_va_arg(vl, char*);
				tmp_len = __msl_strlen(str);
				if (!tmp_len) break;
				head_pos = dst_buf.pos;
				if (p_fmt.flag & _MSL_PRINTF_FLAG_SUB) {
					if (!p_fmt.presition) p_fmt.presition = 0xffffffff;
					while (*str != '\0' && p_fmt.presition--) {
						dst_buf.dat[dst_buf.pos++] = *(str++);
					}
					while (dst_buf.pos - head_pos < p_fmt.width) {
						dst_buf.dat[dst_buf.pos++] = ' ';
					}
				} else {
					int32_t tmp_w;
					__msl_int64_t tmp;
					if (p_fmt.presition) {
						tmp_w = tmp_len < p_fmt.presition ? tmp_len:p_fmt.presition;
					} else {
						tmp_w = tmp_len;
					}
					tmp = p_fmt.width;
					tmp_w = tmp - tmp_w;
					while ((__msl_int64_t)(dst_buf.pos - head_pos) < tmp_w) {
						dst_buf.dat[dst_buf.pos++] = ' ';
					}
					if (!p_fmt.presition) p_fmt.presition = 0xffffffff;
					while (*str != '\0' && p_fmt.presition--) {
						dst_buf.dat[dst_buf.pos++] = *(str++);
					}
				}
				break;
			}
			case 'x':
			case 'X':
			case 'o': {
				char flag_x;
				uint8_t div;
				uint8_t shift;
				flag_x = 0;
				div = 16;
				shift = 4;
				head_pos = dst_buf.pos;
				if (*fmt == 'x') {
					flag_x = 1;
				} else if (*fmt == 'o') {
					div = 8;
					shift = 3;
				}
				__msl_uint64_t d_dat = __msl_va_arg(vl, __msl_uint64_t);
				uint32_t d_32 = (uint32_t)d_dat;
				uint16_t d_16 = (uint16_t)d_dat;
				if (d_dat != 0) {
					switch (p_fmt.length) {
					case 'w':
						while (d_dat != 0) {
							dst_buf.dat[dst_buf.pos] = hex_char[d_dat % div];
							if (flag_x && dst_buf.dat[dst_buf.pos] >= 'A') {
								dst_buf.dat[dst_buf.pos] += 32;
							}
							dst_buf.pos++;
							d_dat = d_dat >> shift;
						}
						break;
					case 'h':
						while (d_16 != 0) {
							dst_buf.dat[dst_buf.pos] = hex_char[d_16 % div];
							if (flag_x && dst_buf.dat[dst_buf.pos] >= 'A') {
								dst_buf.dat[dst_buf.pos] += 32;
							}
							dst_buf.pos++;
							d_16 = d_16 >> shift;
						}
						break;
					default:
						while (d_32 != 0) {
							dst_buf.dat[dst_buf.pos] = hex_char[d_32 % div];
							if (flag_x && dst_buf.dat[dst_buf.pos] >= 'A') {
								dst_buf.dat[dst_buf.pos] += 32;
							}
							dst_buf.pos++;
							d_32 = d_32 >> shift;
						}
						break;
					}
				} else {
					dst_buf.dat[dst_buf.pos++] = '0';
				}
				if (p_fmt.presition != 0) {
					while (dst_buf.pos - head_pos < p_fmt.presition) {
						dst_buf.dat[dst_buf.pos++] = '0';
					}
					p_fmt.flag &= ~_MSL_PRINTF_FLAG_ZERO;
				}
				if ((p_fmt.flag & _MSL_PRINTF_FLAG_ZERO)
						&& !(p_fmt.flag & _MSL_PRINTF_FLAG_SUB)) {
					while (dst_buf.pos - head_pos < p_fmt.width) {
						dst_buf.dat[dst_buf.pos++] = '0';
					}
					if (p_fmt.flag & _MSL_PRINTF_FLAG_POUND) {
						dst_buf.dat[dst_buf.pos - 1] = '0';
						if (*fmt != 'o') {
							dst_buf.dat[dst_buf.pos - 2] = flag_x ? 'x' : 'X';
						}
					}
				} else {
					if (p_fmt.flag & _MSL_PRINTF_FLAG_POUND) {
						if (*fmt != 'o') {
							dst_buf.dat[dst_buf.pos++] = flag_x ? 'x' : 'X';
						}
						dst_buf.dat[dst_buf.pos++] = '0';
					}
					while (dst_buf.pos - head_pos < p_fmt.width) {
						dst_buf.dat[dst_buf.pos++] = ' ';
					}
				}
				int_inverse(&dst_buf, &p_fmt, head_pos);
				break;
			}
			case 'c': {
				char ch;
				ch = (char)__msl_va_arg(vl, uint32_t);
				head_pos = dst_buf.pos;
				if (p_fmt.flag & _MSL_PRINTF_FLAG_SUB) {
					dst_buf.dat[dst_buf.pos++] = ch;
					while (dst_buf.pos - head_pos < p_fmt.width) {
						dst_buf.dat[dst_buf.pos++] = ' ';
					}
				} else {
					if (p_fmt.width > 0) {
						p_fmt.width -= 1;
					}
					while (dst_buf.pos - head_pos < p_fmt.width) {
						dst_buf.dat[dst_buf.pos++] = ' ';
					}
					dst_buf.dat[dst_buf.pos++] = ch;
				}	
				break;
			}
			case 'f':
			case 'F':
			case 'e':
			case 'E':
			case 'g':
			case 'G': {
				uint64_t d_d;
				uint64_t fraction;
				uint64_t fraction_2_int;
				uint32_t integer;
				uint32_t d_d_h, d_d_l;
				uint16_t point;

				head_pos = dst_buf.pos;
				d_d = __msl_va_arg(vl, uint64_t);
				d_d_l = *((uint32_t *)&d_d);
				d_d_h = *(((uint32_t *)&d_d) + 1);

				f = (d_d_h & 0x80000000) ? 1 : 0;
				point = (d_d_h & 0x7ffffffff) >> 20;

				break;
			}
			case '%':
				dst_buf.dat[dst_buf.pos++] = '%';
				break;
			case 'p': {
				head_pos = dst_buf.pos;
#if defined(__MSL_SYSTEM_BIT_32)
				uint32_t addr;
				addr = (uint32_t)__msl_va_arg(vl, void *);
				f = 0;
#elif defined(__MSL_SYSTEM_BIT_64)
				__msl_uint64_t addr;
				addr = (__msl_uint64_t)__msl_va_arg(vl, void *);
				f = 1;
#endif
				while (addr != 0) {
					dst_buf.dat[dst_buf.pos++] = hex_char[addr % 16];
					addr = addr >> 4;
				}
				uint8_t w_b = f ? 16 : 8;
				while (dst_buf.pos - head_pos < w_b) {
					dst_buf.dat[dst_buf.pos++] = '0';
				}
				int_inverse(&dst_buf, &p_fmt, head_pos);
				break;
			}
			case 'n': {
				int32_t *p;
				p = __msl_va_arg(vl, int32_t *);
				*p = dst_buf.pos;
				break;
			}
			default:
				dst_buf.dat[dst_buf.pos++] = (uint8_t)*fmt;	
				if (dst_buf.pos >= __MSL_PRINTF_BUF_MAX)
					goto a2;
				break;
			}
		} else {
			dst_buf.dat[dst_buf.pos++] = (uint8_t)*fmt;	
			if (dst_buf.pos >= __MSL_PRINTF_BUF_MAX)
				break;
		}
		fmt++;
	}
a2:	p = dst_buf.dat;
	// drv_uart_tx_start((u8 *)p, dst_buf.pos);
	p[dst_buf.pos] = '\0';
	__mslex_usart_write(p);
	__msl_va_end(vl);
}
///////////for printf////////////






