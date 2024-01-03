#ifndef RB_H
#define RB_H

typedef struct
{
	unsigned char *data;
	unsigned int item_size;
	unsigned int rb_size;
	unsigned int w_idx;
	unsigned int r_idx;
} rb_t;

#define rb_init(rb, type, data_buf, size) \
	do\
	{\
		(rb)->data = (unsigned char *)(data_buf); \
		(rb)->item_size = sizeof(type); \
		(rb)->rb_size = size; \
		(rb)->w_idx = 0; \
		(rb)->r_idx = 0; \
	} while (0)

#define rb_set_empty(rb) ((rb)->r_idx = (rb)->w_idx)
#define rb_is_empty(rb) ((rb)->r_idx == (rb)->w_idx)
#define rb_is_full(rb) (((rb)->w_idx + 1 == (rb)->r_idx) || (((rb)->r_idx == 0) && ((rb)->w_idx + 1 >= (rb)->rb_size)))
#define rb_len(rb) (((rb)->w_idx >= (rb)->r_idx) ? ((rb)->w_idx - (rb)->r_idx) : ((rb)->rb_size + (rb)->w_idx - (rb)->r_idx))
#define rb_r_idx_inc(rb) (((++(rb)->r_idx) >= (rb)->rb_size) ? ((rb)->r_idx = 0) : ((rb)->r_idx))
#define rb_w_idx_inc(rb) (((++(rb)->w_idx) >= (rb)->rb_size) ? ((rb)->w_idx = 0) : ((rb)->w_idx))
#define rb_peek_r_buff(rb) (&((rb)->data[(rb)->r_idx * (rb)->item_size]))
#define rb_peek_w_buff(rb) (&((rb)->data[(rb)->w_idx * (rb)->item_size]))

#endif
