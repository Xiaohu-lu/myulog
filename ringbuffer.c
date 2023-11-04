/*
 * ringbuffer.c
 *
 *  Created on: Oct 10, 2023
 *      Author: hxd
 */
#include "Drivers.h"
#include "ringbuffer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* ringbuffer_status
 * 查询环形缓冲区的状态
 * rb:环形缓冲区对象句柄指针
 * */
__INLINE enum ringbuffer_state ringbuffer_status(struct ringbuffer *rb)
{
	if(rb->read_index == rb->write_index)
	{
		if(rb->read_mirror == rb->write_mirror)
		{
			return RT_RINGBUFFER_EMPTY;
		}
		else
		{
			return RT_RINGBUFFER_FULL;
		}
	}
	return RT_RINGBUFFER_HALFFULL;
}

/* ringbuffer_init
 * 环形缓冲区初始化
 * rb:环形缓冲区句柄指针
 * pool:环形缓冲区首地址
 * size:环形缓冲区大小
 * */
void ringbuffer_init(struct ringbuffer *rb, uint8_t *pool, int32_t size)
{
	UL_ASSERT(rb != NULL);
	UL_ASSERT(size > 0);

	/*initialize read and write index*/
	rb->read_mirror = rb->write_mirror = 0;
	rb->read_index = rb->write_index = 0;

	/*set buffer pool and size*/
	rb->buffer_ptr = pool;
	rb->buffer_size = size;
}

/* ringbuffer_put
 * 向环形缓冲区里面写数据
 * rb:环形缓冲区句柄指针
 * ptr:要写的数据指针
 * length:要写入的数据长度
 * */
size_t ringbuffer_put(struct ringbuffer *rb, const uint8_t *ptr, uint32_t length)
{
	uint32_t size;
	UL_ASSERT(rb != NULL);

	/*whether has enough space*/
	size = ringbuffer_space_len(rb);

	/*no space*/
	if(size == 0)
	{
		return 0;
	}

	/*drop some data*/
	if(size < length)
	{
		length = size;
	}
	/*环形缓冲区的空间够,直接写*/
	if(rb->buffer_size - rb->write_index > length)
	{
		/*read_index - write_index = empty space*/
		memcpy(&rb->buffer_ptr[rb->write_index], ptr, length);
		/*this should not cause overflow because there is enough space for length of data in current mirror*/
		rb->write_index += length;
		return length;
	}

	memcpy(&rb->buffer_ptr[rb->write_index], &ptr[0], rb->buffer_size - rb->write_index);
	memcpy(&rb->buffer_ptr[0], &ptr[rb->buffer_size - rb->write_index], length - (rb->buffer_size - rb->write_index));

	/*we are going into the other side of the mirror*/
	rb->write_mirror = ~rb->write_mirror;
	rb->write_index = length - (rb->buffer_size - rb->write_index);

	return length;
}

/* ringbuffer_put_force
 * 暴力写入数据到环形缓冲区
 * */
size_t ringbuffer_put_force(struct ringbuffer *rb, const uint8_t *ptr, uint32_t length)
{
	uint32_t space_length;
	UL_ASSERT(rb != NULL);

	space_length = ringbuffer_space_len(rb);

	/*写入数据长度>环形缓冲区大小
	 * 丢弃要写入的前面的数据,写入buffer_size大小数据
	 * */
	if(length > rb->buffer_size)
	{
		ptr = &ptr[length - rb->buffer_size];
		length = rb->buffer_size;
	}
	/*剩余空间够写入*/
	if(rb->buffer_size - rb->write_index > length)
	{
		/*read_index - write_index = empty space*/
		memcpy(&rb->buffer_ptr[rb->write_index], ptr, length);
		/*this should not cause overflow because there is enough space for length of data in current mirror*/
		rb->write_index += length;

		if(length > space_length)/*写入的数据大于ringbuf剩余的空间,说明满了*/
			rb->read_index = rb->write_index;
		return length;
	}

	memcpy(&rb->buffer_ptr[rb->write_index], &ptr[0], rb->buffer_size - rb->write_index);
	memcpy(&rb->buffer_ptr[0], &ptr[rb->buffer_size - rb->write_index], length - (rb->buffer_size - rb->write_index));

	/*we are going into the other side of the mirror*/
	rb->write_mirror = ~rb->write_mirror;
	rb->write_index = length - (rb->buffer_size - rb->write_index);

	if(length > space_length)/*写入的数据大于ringbuf剩余的空间,说明满了*/
	{
		if(rb->write_index <= rb->read_index)
		{
			rb->read_mirror = ~rb->read_mirror;
		}
		rb->read_index = rb->write_index;
	}
	return length;
}

/* ringbuffer_get
 * 从环形缓冲区读数据
 * rb:环形缓冲区句柄
 * ptr:读取数据存放地址
 * length:要读取的 长度
 * */
size_t ringbuffer_get(struct ringbuffer *rb, uint8_t *ptr, uint32_t length)
{
	size_t size;
	UL_ASSERT(rb != NULL);

	/*whether has enough data*/
	size = ringbuffer_data_len(rb);

	/*no data*/
	if(size == 0)
	{
		return 0;
	}
	/*less data*/
	if(size < length)
	{
		length = size;
	}
	if(rb->buffer_size - rb->read_index > length)
	{
		/*copy all of data*/
		memcpy(ptr, &rb->buffer_ptr[rb->read_index],length);
		/*this should not cause overflow because there is enough space for length of data in current mirror*/
		rb->read_index += length;
		return length;
	}
	memcpy(&ptr[0], &rb->buffer_ptr[rb->read_index], rb->buffer_size - rb->read_index);
	memcpy(&ptr[rb->buffer_size - rb->read_index], &rb->buffer_ptr[0], length - (rb->buffer_size - rb->read_index));

	/*we are going into the other side of the mirror*/
	rb->read_mirror = ~rb->read_mirror;
	rb->read_index = length - (rb->buffer_size - rb->read_index);

	return length;
}

/* ringbuffer_peek
 * 获取环形缓冲区第一个可以读的字节数
 * rb:环形缓冲区句柄
 * ptr:指向第一个可读的字节地址
 * return:返回第一个可读的字节数
 * */
size_t ringbuffer_peek(struct ringbuffer *rb, uint8_t **ptr)
{
	size_t size;
	UL_ASSERT(rb != NULL);

	*ptr = NULL;

	/*whether has enough data*/
	size = ringbuffer_data_len(rb);

	/*no data*/
	if(size == 0)
	{
		return 0;
	}

	*ptr = &rb->buffer_ptr[rb->read_index];

	if((size_t)(rb->buffer_size - rb->read_index) > size)
	{
		rb->read_index += size;
		return size;
	}

	size = rb->buffer_size - rb->read_index;

	/*we are going into the other side of the mirror*/
	rb->read_mirror = ~rb->read_mirror;
	rb->read_index = 0;
	return size;
}

/* ringbuffer_putchar
 * 向环形缓冲区写入一个字节
 * rb:要写入的环形缓冲区
 * ch:要写入的数据
 * */
size_t ringbuffer_putchar(struct ringbuffer *rb, const uint8_t ch)
{
	UL_ASSERT(rb != NULL);

	/*whether has enough space*/
	if(!ringbuffer_space_len(rb))
	{
		return 0;
	}

	rb->buffer_ptr[rb->write_index] = ch;

	/*flip mirror*/
	if(rb->write_index == rb->buffer_size - 1)
	{
		rb->write_mirror = ~rb->write_mirror;
		rb->write_index = 0;
	}
	else
	{
		rb->write_index ++;
	}
	return 1;
}

/* ringbuffer_putchar_force
 * 暴力向环形缓冲区写1字节数据,不管有没有空间直接写入1个字节,导致读的地方字节变了,所有readindex=write_index
 * rb:环形缓冲区句柄
 * ch:要写入的字节数据
 * */
size_t ringbuffer_putchar_force(struct ringbuffer *rb, const uint8_t ch)
{
	enum ringbuffer_state old_state;
	UL_ASSERT(rb != NULL);

	old_state = ringbuffer_status(rb);

	rb->buffer_ptr[rb->write_index] = ch;

	/*flip mirror,写到了缓冲区的最后,要反转*/
	if(rb->write_index == rb->buffer_size - 1)
	{
		rb->write_mirror = ~rb->write_mirror;
		rb->write_index = 0;
		if(old_state == RT_RINGBUFFER_FULL)/*缓冲区满了,要更新read_index*/
		{
			rb->read_mirror = ~rb->read_mirror;
			rb->read_index = rb->write_index;
		}
	}
	else
	{
		rb->write_index ++;
		if(old_state == RT_RINGBUFFER_FULL)/*缓冲区满了,要更新read_index=write_index*/
		{
			rb->read_index = rb->write_index;
		}
	}
	return 1;
}

/* ringbuffer_getchar
 * 从环形缓冲区获取一个字节
 * rb:环形缓冲区句柄指针
 * ch:存在字符地址
 * */
size_t ringbuffer_getchar(struct ringbuffer *rb, uint8_t *ch)
{
	UL_ASSERT(rb != NULL);

	/*ringbuffer is empty*/
	if(!ringbuffer_data_len(rb))
	{
		return 0;
	}

	/*put byte*/
	*ch = rb->buffer_ptr[rb->read_index];

	if(rb->read_index == rb->buffer_size - 1)
	{
		rb->read_mirror = ~rb->read_mirror;
		rb->read_index = 0;
	}
	else
	{
		rb->read_index ++;
	}
	return 1;
}

/* ringbuffer_data_len
 * 获取环形缓冲区有效数据个数
 * rb:皇兄缓冲区句柄指针
 * */
size_t ringbuffer_data_len(struct ringbuffer *rb)
{
	size_t wi = rb->write_index, ri = rb->read_index;
	switch(ringbuffer_status(rb))
	{
	case RT_RINGBUFFER_EMPTY:
		return 0;
	case RT_RINGBUFFER_FULL:
		return rb->buffer_size;
	case RT_RINGBUFFER_HALFFULL:
	default:
	{
		if(wi > ri)
		{
			return wi - ri;
		}
		else
		{
			return rb->buffer_size - (ri - wi);
		}
	}
	}
}

/* ringbuffer_reset
 * 环形缓冲区复位
 * rb:环形缓冲区句柄指针
 * */
void ringbuffer_reset(struct ringbuffer *rb)
{
	UL_ASSERT(rb != NULL);

	rb->read_mirror = 0;
	rb->read_index = 0;
	rb->write_mirror = 0;
	rb->write_index = 0;
}

/* ringbuffer_create
 * 创建环形缓冲区对象
 * size:环形缓冲区大小
 * */
struct ringbuffer *ringbuffer_create(uint32_t size)
{
	struct ringbuffer *rb;
	uint8_t *pool;

	UL_ASSERT(size > 0);
	/*向下字节对齐*/
	size = RT_ALIGN_DOWN(size, RT_ALIGN_SIZE);

	rb = (struct ringbuffer *)pvPortMalloc(sizeof(struct ringbuffer));
	if(rb == NULL)
	{
		goto exit;
	}

	pool = (uint8_t *)pvPortMalloc(size);
	if(pool == NULL)
	{
		vPortFree(rb);
		rb = NULL;
		goto exit;
	}

	ringbuffer_init(rb, pool, size);

exit:
	return rb;
}

/* ringbuffer_destroy
 * 销毁ring环形缓冲区对象
 * rb:环形缓冲区对象
 * */
void ringbuffer_destroy(struct ringbuffer *rb)
{
	UL_ASSERT(rb != NULL);
	vPortFree(rb->buffer_ptr);
	vPortFree(rb);
}






