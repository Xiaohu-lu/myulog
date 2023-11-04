/*
 * ringbuffer.h
 *
 *  Created on: Oct 9, 2023
 *      Author: hxd
 */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_
#include <stdint.h>
#include <stddef.h>
#include "H01_Device.h"
#include "ulog_def.h"
/*ring buffer*/
struct ringbuffer
{
	uint8_t *buffer_ptr;
	/* use the msb of the {read/write}_index as mirror bit. you can see this as
	 * if the buffer adds a virtual mirror and the pointers point either to the normal
	 * or to the mirrored buffer. if the write_index has the same value with the read_index,
	 * but in a different mirror, the buffer is full. while if the write_index and the read_index
	 * are the same and within the same mirror,the buffer is empty.
	 * the ASCII art of the ringbuffer is:
	 *          mirror = 0                    mirror = 1
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * | 0 | 1 | 2 | 3 | 4 | 5 | 6 ||| 0 | 1 | 2 | 3 | 4 | 5 | 6 | Full
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     *  read_idx-^                   write_idx-^
     *
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * | 0 | 1 | 2 | 3 | 4 | 5 | 6 ||| 0 | 1 | 2 | 3 | 4 | 5 | 6 | Empty
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * read_idx-^ ^-write_idx
	 * */
	uint32_t read_mirror : 1;
	uint32_t read_index : 31;
	uint32_t write_mirror : 1;
	uint32_t write_index : 31;
	/*as we use msb of index as mirror bit, the size should be signed and could only be positive(正)*/
	int32_t buffer_size;
};

enum ringbuffer_state
{
	RT_RINGBUFFER_EMPTY,
	RT_RINGBUFFER_FULL,
	/*half full is neither full nor empty*/
	RT_RINGBUFFER_HALFFULL,
};



void ringbuffer_init(struct ringbuffer *rb, uint8_t *pool, int32_t size);
size_t ringbuffer_put(struct ringbuffer *rb, const uint8_t *ptr, uint32_t length);
size_t ringbuffer_put_force(struct ringbuffer *rb, const uint8_t *ptr, uint32_t length);
size_t ringbuffer_get(struct ringbuffer *rb, uint8_t *ptr, uint32_t length);
size_t ringbuffer_peek(struct ringbuffer *rb, uint8_t **ptr);
size_t ringbuffer_putchar(struct ringbuffer *rb, const uint8_t ch);
size_t ringbuffer_putchar_force(struct ringbuffer *rb, const uint8_t ch);
size_t ringbuffer_getchar(struct ringbuffer *rb, uint8_t *ch);
size_t ringbuffer_data_len(struct ringbuffer *rb);
void ringbuffer_reset(struct ringbuffer *rb);
struct ringbuffer *ringbuffer_create(uint32_t size);
void ringbuffer_destroy(struct ringbuffer *rb);

/* ringbuffer_get_size
 * 获取环形缓冲区大小
 * rb:环形缓冲区句柄
 * */
__INLINE uint32_t ringbuffer_get_size(struct ringbuffer *rb)
{
	UL_ASSERT(rb != NULL);
	return rb->buffer_size;
}

/*return the size of empty space in rb*/
#define ringbuffer_space_len(rb)	((rb)->buffer_size - ringbuffer_data_len(rb))

#endif /* RINGBUFFER_H_ */
