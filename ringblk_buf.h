/*
 * ringblk_buf.h
 *
 *  Created on: Oct 9, 2023
 *      Author: hxd
 */

#ifndef RINGBLK_BUF_H_
#define RINGBLK_BUF_H_
#include <stdint.h>
#include <stddef.h>
#include "ulogdef.h"

enum rbb_status
{
	/*unused status when first initialize or after blk_free()*/
	RT_RBB_BLK_UNUSED,
	/*initialized status after blk_alloc()*/
	RT_RBB_BLK_INITED,
	/*put status after blk_put()*/
	RT_RBB_BLK_PUT,
	/*get status after blk_get()*/
	RT_RBB_BLK_GET
};
typedef enum rbb_status rbb_status_t;

/* the block of rbb
 * */
struct rbb_blk
{
	rbb_status_t status :8;
	/*less then 2^24*/
	size_t size :24;
	uint8_t *buf;
	slist_t list;
};
typedef struct rbb_blk *rbb_blk_t;

/* rbb block queue:the blocks(from block1->buf to blockn->buf)memory which on this queue is continuous(Á¬Ðø)
 * */
struct rbb_blk_queue
{
	rbb_blk_t blocks;
	size_t blk_num;
};
typedef struct rbb_blk_queue *rbb_blk_queue_t;

/* ring block buffer
 * */
struct rbb
{
	uint8_t *buf;
	size_t buf_size;
	/*all of blocks*/
	rbb_blk_t blk_set;
	size_t blk_max_num;
	/*saved the initialized and put status blocks*/
	slist_t blk_list;
	/*point to tail node*/
	slist_t *tail;
	/*free node list*/
	slist_t free_list;
};
typedef struct rbb *rbb_t;


void rbb_init(rbb_t rbb, uint8_t *buf, size_t buf_size, rbb_blk_t block_set, size_t blk_max_num);
rbb_t rbb_create(size_t buf_size, size_t blk_max_num);
void rbb_destroy(rbb_t rbb);

rbb_blk_t rbb_blk_alloc(rbb_t rbb, size_t blk_size);
void rbb_blk_put(rbb_blk_t block);
rbb_blk_t rbb_blk_get(rbb_t rbb);
size_t rbb_blk_size(rbb_blk_t block);
uint8_t *rbb_blk_buf(rbb_blk_t block);
void rbb_blk_free(rbb_t rbb, rbb_blk_t block);

#endif /* RINGBLK_BUF_H_ */
