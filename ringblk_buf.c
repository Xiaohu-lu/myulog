/*
 * ringblk_buf.c
 *
 *  Created on: Oct 9, 2023
 *      Author: hxd
 */
#include "ringblk_buf.h"
#include <stddef.h>
#include "ulservice.h"
#include "ulog_def.h"
#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"
#include "semphr.h"


#define RT_USING_HEAP

/* rbb_init
 * 环形块缓冲区对象初始化
 * rbb:环形块缓冲区对象句柄
 * buf:缓冲区地址
 * buf_size:缓冲区大小
 * block_set:块对象数组
 * blk_max_num:最大块数
 * */
void rbb_init(rbb_t rbb, uint8_t *buf, size_t buf_size, rbb_blk_t block_set, size_t blk_max_num)
{
	size_t i;

	UL_ASSERT(rbb);
	UL_ASSERT(buf);
	UL_ASSERT(block_set);

	rbb->buf = buf;
	rbb->buf_size = buf_size;
	rbb->blk_set = block_set;
	rbb->blk_max_num = blk_max_num;
	rbb->tail = &rbb->blk_list;

	slist_init(&rbb->blk_list);
	slist_init(&rbb->free_list);
	/*initialize block status*/
	for(i = 0; i < blk_max_num; i++)
	{
		block_set[i].status = RT_RBB_BLK_UNUSED;
		slist_init(&block_set[i].list);
		slist_insert(&rbb->free_list, &block_set[i].list);/*插入块到空闲链表*/
	}
}

#ifdef RT_USING_HEAP

/* rbb_create
 * 创建环形块缓冲区
 * buf_size:环形块缓冲区的大小
 * blk_max_num:最大块数
 * */
rbb_t rbb_create(size_t buf_size, size_t blk_max_num)
{
	rbb_t rbb = NULL;
	uint8_t *buf;
	rbb_blk_t blk_set;

	/*申请环形块缓存区对象空间*/
	rbb = (rbb_t)pvPortMalloc(sizeof(struct rbb));
	if(!rbb)
	{
		return NULL;
	}
	/*申请缓冲区*/
	buf = (uint8_t *)pvPortMalloc(buf_size);
	if(!buf)
	{
		vPortFree(rbb);
		return NULL;
	}
	/*申请块对象空间,一个环形缓冲区可以分为blk_max_num个块*/
	blk_set = (rbb_blk_t)pvPortMalloc(sizeof(struct rbb_blk) * blk_max_num);
	if(!blk_set)
	{
		vPortFree(buf);
		vPortFree(rbb);
		return NULL;
	}
	rbb_init(rbb, buf, buf_size, blk_set, blk_max_num);
	return rbb;
}

/* rbb_destroy
 * 销毁环形块缓冲区
 * rbb:要销毁的环形块
 * */
void rbb_destroy(rbb_t rbb)
{
	UL_ASSERT(rbb);
	vPortFree(rbb->buf);
	vPortFree(rbb->blk_set);
	vPortFree(rbb);
}


#endif

/* find_empty_blk_in_set
 * 找一个空块,从rbb的free_list链表中找
 * rbb:rbb_t句柄
 * */
static rbb_blk_t find_empty_blk_in_set(rbb_t rbb)
{
	struct rbb_blk *blk;
	UL_ASSERT(rbb);

	/*如果free_list为空,表明没有空闲的块了*/
	if(slist_isempty(&rbb->free_list))
	{
		return NULL;
	}
	/* 从free_list的第一个节点取出来作为返回
	 * 根据list地址找到rbb_blk的首地址
	 * */
	blk = slist_first_entry(&rbb->free_list, struct rbb_blk, list);
	/* 把该块移出free链表
	 * */
	slist_remove(&rbb->free_list, &blk->list);
	UL_ASSERT(blk->status == RT_RBB_BLK_UNUSED);
	return blk;
}

/* rbb_list_append
 * 在链表尾部添加节点
 * rbb:要添加的链表
 * n:要添加的节点
 * */
__INLINE void rbb_list_append(rbb_t rbb, slist_t *n)
{
	/*append the node to the tail*/
	rbb->tail->next = n;
	n->next = NULL;
	rbb->tail = n;
}

/* rbb_list_remove
 * 从链表中删除一个节点
 * rbb:rbb_t句柄
 * n:要删除的节点
 * */
__INLINE slist_t *rbb_list_remove(rbb_t rbb, slist_t *n)
{
	slist_t *l = &rbb->blk_list;
	struct slist_node *node = l;

	/*remove slist head*/
	while(node->next && node->next != n) node = node->next;
	/*remove node*/
	if(node->next != (slist_t *)0)
	{
		node->next = node->next->next;
		n->next = NULL;
		/*update tail node*/
		if(rbb->tail == n)
			rbb->tail = node;
	}
	return l;
}

/* rbb_blk_alloc
 * 从块缓冲区里面申请一个空闲块
 * rbb:环形块缓冲区句柄指针
 * blk_size:要申请的块的大小
 * */
rbb_blk_t rbb_blk_alloc(rbb_t rbb, size_t blk_size)
{
	size_t empty1 = 0, empty2 = 0;
	rbb_blk_t head, tail, new_rbb = NULL;
	UL_ASSERT(rbb);
	UL_ASSERT(blk_size < (1L << 24));
	taskENTER_CRITICAL();
	/*从空闲链表的前面找一个空闲的块*/
	new_rbb = find_empty_blk_in_set(rbb);

	if(new_rbb)
	{
		/*blk_list不为空
		 * */
		if(slist_isempty(&rbb->blk_list) == 0)
		{
			/* 找到blk_list的第一个节点就是头
			 * */
			head = slist_first_entry(&rbb->blk_list, struct rbb_blk, list);
			/*get tail rbb blk object
			 * 尾由rbb->tail指向
			 * */
			tail = slist_entry(rbb->tail, struct rbb_blk, list);
			if(head->buf <= tail->buf)
			{
				/**
				 *  head = tail
				 * +--------------------------------------+-----------------+------------------+
				 * | block1 |                    empty1										   |
				 * +--------------------------------------+-----------------+------------------+
				 * 0																		  rbb->buf_size-1
				 *                            rbb->buf
				 */
				/* 当只有一个块时,头和尾指向同一个块
				 * empty1 = (rbb->buf + rbb->buf_size) - (tail->buf + tail->size); = rbb->buf_size - tail->size;
				 * empty2 = head->buf - rbb->buf; = 0
				 * */

				/**
				 *                      head                     tail
				 * +--------------------------------------+-----------------+------------------+
				 * |      empty2     | block1 |   block2  |      block3     |       empty1     |
				 * +--------------------------------------+-----------------+------------------+
				 *                            rbb->buf
				 */
				empty1 = (rbb->buf + rbb->buf_size) - (tail->buf + tail->size);
				empty2 = head->buf - rbb->buf;

				/* 先从rbb->buf的后面找空间
				 * */
				if(empty1 >= blk_size)
				{
					/* 将rbb->tail->next = new_rbb->list;
					 * new_rbb->list->next = NULL;
					 * rbb->tail = new_rbb;
					 * */
					/* 假设已经有1个block1
					 * 添加1个block2
					 * 之前rbb->blk_list.next = block1.list;
					 * rbb->tail = &block1.list;
					 * blk_list->block1
					 * 添加后则
					 * block1.list->next = block2.list;
					 * rbb->tail = block2.list;
					 * 之后rbb->blk_list.next = block1.list;
					 * rbb->tail = &block2.list
					 * blk_list->block1->block2
					 * */
					rbb_list_append(rbb, &new_rbb->list);
					new_rbb->status = RT_RBB_BLK_INITED;
					new_rbb->buf = tail->buf + tail->size;
					new_rbb->size = blk_size;
				}
				else if(empty2 >= blk_size)/*后面空间不足,判断前面空间*/
				{
					rbb_list_append(rbb, &new_rbb->list);
					new_rbb->status = RT_RBB_BLK_INITED;
					new_rbb->buf = rbb->buf;
					new_rbb->size = blk_size;
				}
				else
				{
					/*no space*/
					new_rbb = NULL;
				}
			}
			else/*到这里说明前面添加的一个块,添加到buf的前面*/
			{
				/**
				 *        tail                                              head
				 * +----------------+-------------------------------------+--------+-----------+
				 * |     block3     |                empty1               | block1 |  block2   |
				 * +----------------+-------------------------------------+--------+-----------+
				 *                            rbb->buf
				 */
				empty1 = head->buf - (tail->buf + tail->size);

				if(empty1 >= blk_size)
				{
					rbb_list_append(rbb, &new_rbb->list);
					new_rbb->status = RT_RBB_BLK_INITED;
					new_rbb->buf = tail->buf + tail->size;
					new_rbb->size = blk_size;
				}
				else
				{
					/*no space*/
					new_rbb = NULL;
				}
			}
		}
		else/*向blk_list添加第一个使用的块*/
		{
			/* the list is empty
			 * rbb->tail->next = n;
			 * n->next = NULL;
			 * rbb->tail = n;
			 * 刚初始化的时候rbb->tail = &rbb->blk_list.
			 * 则现在rbb->blk_list.next = n;
			 * rbb->tail = n;
			 * */

			/**
			 *
			 * +----------------+-------------------------------------+--------+-----------+
			 * |     newblock   |                empty1               					   |
			 * +----------------+-------------------------------------+--------+-----------+
			 * 0																			rbb->buf_size-1
			 *                            rbb->buf
			 */

			rbb_list_append(rbb, &new_rbb->list);
			/*块状态变成初始化过*/
			new_rbb->status = RT_RBB_BLK_INITED;
			/*该块的缓冲区首地址=rbb缓冲区的首地址第一个块*/
			new_rbb->buf = rbb->buf;
			new_rbb->size = blk_size;
		}
	}
	else
	{
		new_rbb = NULL;
	}
	taskEXIT_CRITICAL();
	return new_rbb;
}

/* rbb_blk_put
 * 将块放入环形缓冲区对象
 * block:要放入的块
 * */
void rbb_blk_put(rbb_blk_t block)
{
	UL_ASSERT(block);
	UL_ASSERT(block->status == RT_RBB_BLK_INITED);

	block->status = RT_RBB_BLK_PUT;
}

/* rbb_blk_get
 * 从环形缓冲区对象获取一个块
 * rbb:环形缓冲区对象
 * */
rbb_blk_t rbb_blk_get(rbb_t rbb)
{
	rbb_blk_t block = NULL;
	slist_t *node;

	UL_ASSERT(rbb);

	if(slist_isempty(&rbb->blk_list))
	{
		return 0;
	}
	taskENTER_CRITICAL();

	for(node = slist_first(&rbb->blk_list); node; node = slist_next(node))
	{
		block = slist_entry(node, struct rbb_blk, list);
		if(block->status == RT_RBB_BLK_PUT)
		{
			block->status = RT_RBB_BLK_GET;
			goto __exit;
		}
	}
	/*not found*/
	block = NULL;
__exit:

	taskEXIT_CRITICAL();
	return block;
}

/* rbb_blk_size
 * 获取块的大小
 * block:要获取的块
 * */
size_t rbb_blk_size(rbb_blk_t block)
{
	UL_ASSERT(block);
	return block->size;
}

/* rbb_blk_buf
 * 获取块的缓冲区
 * */
uint8_t *rbb_blk_buf(rbb_blk_t block)
{
	UL_ASSERT(block);
	return block->buf;
}

/* rbb_blk_free
 * 释放块
 * rbb:环形块缓冲区句柄指针
 * block:要释放的块
 * */
void rbb_blk_free(rbb_t rbb, rbb_blk_t block)
{
	UL_ASSERT(rbb);
	UL_ASSERT(block);
	UL_ASSERT(block->status != RT_RBB_BLK_UNUSED);

	taskENTER_CRITICAL();
	/*remove it on rbb block list
	 * 将块移出rbb->blk_list链表,
	 * 如果删除的是尾节点则更新rbb->tail
	 * */
	rbb_list_remove(rbb, &block->list);
	block->status = RT_RBB_BLK_UNUSED;
	/*将释放的节点添加的空闲列表*/
	slist_insert(&rbb->free_list, &block->list);

	taskEXIT_CRITICAL();
}

/* rbb_blk_queue_get
 *
 * */
size_t rbb_blk_queue_get(rbb_t rbb, size_t queue_data_len, rbb_blk_queue_t blk_queue)
{
	size_t data_total_size = 0;
	slist_t *node, *tmp = NULL;
	rbb_blk_t last_block = NULL, block;

	UL_ASSERT(rbb);
	UL_ASSERT(blk_queue);

	if(slist_isempty(&rbb->blk_list))
	{
		return 0;
	}

	taskENTER_CRITICAL();

	/* 找到blk_list的第一个节点
	 * */
	node = slist_first(&rbb->blk_list);
	if(node != NULL)
	{
		tmp = slist_next(node);
	}

	for(; node; node = tmp, tmp = slist_next(node))
	{
		if(!last_block)
		{
			last_block = slist_entry(node, struct rbb_blk, list);
			if(last_block->status == RT_RBB_BLK_PUT)
			{
				/*save the first put status block to queue*/
				blk_queue->blocks = last_block;
				blk_queue->blk_num = 0;
			}
			else
			{
				/*the first block must be put status*/
				last_block = NULL;
				continue;
			}
		}
		else
		{
			block = slist_entry(node, struct rbb_blk, list);
			/* these following conditions will break the loop:
			 * 1. the current block is not put status
			 * 2. the last block and current block is not continuous
			 * 3. the data_total_size will out of range
			 * */
			if(block->status != RT_RBB_BLK_PUT || last_block->buf > block->buf || data_total_size + block->size > queue_data_len)
			{
				break;
			}
			/*backup last block*/
			last_block = block;
		}
		/*remove current block*/
		data_total_size += last_block->size;
		last_block->status = RT_RBB_BLK_GET;
		blk_queue->blk_num++;
	}

	taskEXIT_CRITICAL();
	return data_total_size;
}

/* rbb_blk_queue_len
 *
 * */
size_t rbb_blk_queue_len(rbb_blk_queue_t blk_queue)
{
	size_t i = 0, data_total_size = 0;
	rbb_blk_t blk;
	UL_ASSERT(blk_queue);

	for(blk = blk_queue->blocks; i < blk_queue->blk_num; i++)
	{
		data_total_size += blk->size;
		blk = slist_entry(blk->list.next, struct rbb_blk, list);
	}
	return data_total_size;
}

/* rbb_blk_queue_buf
 * */
uint8_t *rbb_blk_queue_buf(rbb_blk_queue_t blk_queue)
{
	UL_ASSERT(blk_queue);
	return blk_queue->blocks[0].buf;
}

/*
 * */
void rbb_blk_queue_free(rbb_t rbb, rbb_blk_queue_t blk_queue)
{
	size_t i = 0;
	rbb_blk_t blk, next_blk;

	UL_ASSERT(rbb);
	UL_ASSERT(blk_queue);

	for(blk = blk_queue->blocks; i < blk_queue->blk_num; i++)
	{
		next_blk = slist_entry(blk->list.next, struct rbb_blk, list);
		rbb_blk_free(rbb, blk);
		blk = next_blk;
	}
}

/*
 * */
size_t rbb_next_blk_queue_len(rbb_t rbb)
{
	size_t data_len = 0;
	slist_t *node;
	rbb_blk_t last_block = NULL, block;

	UL_ASSERT(rbb);

	if(slist_is_empty(&rbb->blk_list))
	{
		return 0;
	}
	taskENTER_CRITICAL();

	for(node = slist_first(&rbb->blk_list); node; node = slist_next(node))
	{
		if(!last_block)
		{
			last_block = slist_entry(node, struct rbb_blk, list);
			if(last_block->status != RT_RBB_BLK_PUT)
			{
				/*the first block must be put status*/
				last_block = NULL;
				continue;
			}
		}
		else
		{
			block = slist_entry(node, struct rbb_blk, list);
			/* these following conditions will break the loop:
			 * 1. the current block is not put status
			 * 2. the last block and current block is not continous
			 * */
			if(block->status != RT_RBB_BLK_PUT || last_block->buf > block->buf)
			{
				break;
			}
			/*backup last block*/
			last_block = block;
		}
		data_len += last_block->size;
	}

	taskEXIT_CRITICAL();
	return data_len;
}

/*
 * */
size_t rbb_get_buf_size(rbb_t rbb)
{
	UL_ASSERT(rbb);
	return rbb->buf_size;
}
