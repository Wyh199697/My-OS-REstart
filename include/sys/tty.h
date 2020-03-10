#ifndef _ORANGES_TTY_H_
#define _ORANGES_TTY_H_

#define TTY_IN_BYTES	256
#define TTY_OUT_BUF_LEN		2	/* tty output buffer size */

struct s_console;

typedef struct s_tty{
	u32		in_buf[TTY_IN_BYTES];//ibuf
	u32*	p_inbuf_head;	//ibuf_head
	u32*	p_inbuf_tail;	//ibuf_tail
//	int		in_inbuf_tail;	
	int		inbuf_count;	//ibuf_cnt

	int tty_caller;
	int tty_procnr;
	void* tty_req_buf;
	int tty_left_cnt;
	int tty_trans_cnt;

	struct s_console*	p_console;
}TTY;

#endif
