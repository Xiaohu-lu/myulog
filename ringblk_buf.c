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
 * ���ο黺���������ʼ��
 * rbb:���ο黺����������
 * buf:��������ַ
 * buf_size:��������С
 * block_set:���������
 * blk_max_num:������
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
		slist_insert(&rbb->free_list, &block_set[i].list);/*����鵽��������*/
	}
}

#ifdef RT_USING_HEAP

/* rbb_create
 * �������ο黺����
 * buf_size:���ο黺�����Ĵ�С
 * blk_max_num:������
 * */
rbb_t rbb_create(size_t buf_size, size_t blk_max_num)
{
	rbb_t rbb = NULL;
	uint8_t *buf;
	rbb_blk_t blk_set;

	/*���뻷�ο黺��������ռ�*/
	rbb = (rbb_t)pvPortMalloc(sizeof(struct rbb));
	if(!rbb)
	{
		return NULL;
	}
	/*���뻺����*/
	buf = (uint8_t *)pvPortMalloc(buf_size);
	if(!buf)
	{
		vPortFree(rbb);
		return NULL;
	}
	/*��������ռ�,һ�����λ��������Է�Ϊblk_max_num����*/
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
 * ���ٻ��ο黺����
 * rbb:Ҫ���ٵĻ��ο�
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
 * ��һ���տ�,��rbb��free_list��������
 * rbb:rbb_t���
 * */
static rbb_blk_t find_empty_blk_in_set(rbb_t rbb)
{
	struct rbb_blk *blk;
	UL_ASSERT(rbb);

	/*���free_listΪ��,����û�п��еĿ���*/
	if(slist_isempty(&rbb->free_list))
	{
		return NULL;
	}
	/* ��free_list�ĵ�һ���ڵ�ȡ������Ϊ����
	 * ����list��ַ�ҵ�rbb_blk���׵�ַ
	 * */
	blk = slist_first_entry(&rbb->free_list, struct rbb_blk, list);
	/* �Ѹÿ��Ƴ�free����
	 * */
	slist_remove(&rbb->free_list, &blk->list);
	UL_ASSERT(blk->status == RT_RBB_BLK_UNUSED);
	return blk;
}

/* rbb_list_append
 * ������β����ӽڵ�
 * rbb:Ҫ��ӵ�����
 * n:Ҫ��ӵĽڵ�
 * */
__INLINE void rbb_list_append(rbb_t rbb, slist_t *n)
{
	/*append the node to the tail*/
	rbb->tail->next = n;
	n->next = NULL;
	rbb->tail = n;
}

/* rbb_list_remove
 * ��������ɾ��һ���ڵ�
 * rbb:rbb_t���
 * n:Ҫɾ���Ľڵ�
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
 * �ӿ黺������������һ�����п�
 * rbb:���ο黺�������ָ��
 * blk_size:Ҫ����Ŀ�Ĵ�С
 * */
rbb_blk_t rbb_blk_alloc(rbb_t rbb, size_t blk_size)
{
	size_t empty1 = 0, empty2 = 0;
	rbb_blk_t head, tail, new_rbb = NULL;
	UL_ASSERT(rbb);
	UL_ASSERT(blk_size < (1L << 24));
	taskENTER_CRITICAL();
	/*�ӿ��������ǰ����һ�����еĿ�*/
	new_rbb = find_empty_blk_in_set(rbb);

	if(new_rbb)
	{
		/*blk_list��Ϊ��
		 * */
		if(slist_isempty(&rbb->blk_list) == 0)
		{
			/* �ҵ�blk_list�ĵ�һ���ڵ����ͷ
			 * */
			head = slist_first_entry(&rbb->blk_list, struct rbb_blk, list);
			/*get tail rbb blk object
			 * β��rbb->tailָ��
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
				/* ��ֻ��һ����ʱ,ͷ��βָ��ͬһ����
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

				/* �ȴ�rbb->buf�ĺ����ҿռ�
				 * */
				if(empty1 >= blk_size)
				{
					/* ��rbb->tail->next = new_rbb->list;
					 * new_rbb->list->next = NULL;
					 * rbb->tail = new_rbb;
					 * */
					/* �����Ѿ���1��block1
					 * ���1��block2
					 * ֮ǰrbb->blk_list.next = block1.list;
					 * rbb->tail = &block1.list;
					 * blk_list->block1
					 * ��Ӻ���
					 * block1.list->next = block2.list;
					 * rbb->tail = block2.list;
					 * ֮��rbb->blk_list.next = block1.list;
					 * rbb->tail = &block2.list
					 * blk_list->block1->block2
					 * */
					rbb_list_append(rbb, &new_rbb->list);
					new_rbb->status = RT_RBB_BLK_INITED;
					new_rbb->buf = tail->buf + tail->size;
					new_rbb->size = blk_size;
				}
				else if(empty2 >= blk_size)/*����ռ䲻��,�ж�ǰ��ռ�*/
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
			else/*������˵��ǰ����ӵ�һ����,��ӵ�buf��ǰ��*/
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
		else/*��blk_list��ӵ�һ��ʹ�õĿ�*/
		{
			/* the list is empty
			 * rbb->tail->next = n;
			 * n->next = NULL;
			 * rbb->tail = n;
			 * �ճ�ʼ����ʱ��rbb->tail = &rbb->blk_list.
			 * ������rbb->blk_list.next = n;
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
			/*��״̬��ɳ�ʼ����*/
			new_rbb->status = RT_RBB_BLK_INITED;
			/*�ÿ�Ļ������׵�ַ=rbb���������׵�ַ��һ����*/
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
 * ������뻷�λ���������
 * block:Ҫ����Ŀ�
 * */
void rbb_blk_put(rbb_blk_t block)
{
	UL_ASSERT(block);
	UL_ASSERT(block->status == RT_RBB_BLK_INITED);

	block->status = RT_RBB_BLK_PUT;
}

/* rbb_blk_get
 * �ӻ��λ����������ȡһ����
 * rbb:���λ���������
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
 * ��ȡ��Ĵ�С
 * block:Ҫ��ȡ�Ŀ�
 * */
size_t rbb_blk_size(rbb_blk_t block)
{
	UL_ASSERT(block);
	return block->size;
}

/* rbb_blk_buf
 * ��ȡ��Ļ�����
 * */
uint8_t *rbb_blk_buf(rbb_blk_t block)
{
	UL_ASSERT(block);
	return block->buf;
}

/* rbb_blk_free
 * �ͷſ�
 * rbb:���ο黺�������ָ��
 * block:Ҫ�ͷŵĿ�
 * */
void rbb_blk_free(rbb_t rbb, rbb_blk_t block)
{
	UL_ASSERT(rbb);
	UL_ASSERT(block);
	UL_ASSERT(block->status != RT_RBB_BLK_UNUSED);

	taskENTER_CRITICAL();
	/*remove it on rbb block list
	 * �����Ƴ�rbb->blk_list����,
	 * ���ɾ������β�ڵ������rbb->tail
	 * */
	rbb_list_remove(rbb, &block->list);
	block->status = RT_RBB_BLK_UNUSED;
	/*���ͷŵĽڵ���ӵĿ����б�*/
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

	/* �ҵ�blk_list�ĵ�һ���ڵ�
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
