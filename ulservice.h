#ifndef __ULSERVICE_H
#define __ULSERVICE_H
#include "ulog_types.h"

/* return the member address of ptr, if the type of ptr is the struct type 
 */
#define ul_container_of(ptr, type, member) ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))


/* ul_slist_first
 * 返回链表的第一个节点
 * l:		链表头节点
 * return：	链表第一个节点
 */
ul_inline ul_slist_t *ul_slist_first(ul_slist_t *l)
{
	return l->next;
}

/* ul_slist_tail
 * 返回链表的第一个节点
 * l:		链表头节点
 * return：	链表最后一个节点
 */
ul_inline ul_slist_t *ul_slist_tail(ul_slist_t *l)
{
	while(l->next) l = l->next;
	return l;
}

/* ul_slist_first
 * 返回链表的第一个节点
 * n:		链表节点
 * return：	链表下一个节点
 */
ul_inline ul_slist_t *ul_slist_next(ul_slist_t *n)
{
	return n->next;
}

/* ul_slist_isempty
 * 判断链表是否为空
 * l:		链表根节点
 * return：	为空返回1,否则返回0
 */
ul_inline int ul_slist_isempty(ul_slist_t *l)
{
	return l->next == UL_NULL;
}

/* ul_slist_entry
 * 根据结构体的一个成员获取该结构体的首地址
 * node：结构体成员地址
 * type：结构体类型
 * member：结构体的一个成员
 */
#define ul_slist_entry(node, type, member)	ul_container_of(node, type, name)

#endif


