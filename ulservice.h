/*
 * ulservice.h
 *
 *  Created on: Oct 8, 2023
 *      Author: hxd
 */

#ifndef ULSERVICE_H_
#define ULSERVICE_H_
#include "H01_Device.h"
#include "ulogdef.h"

/* 根据结构体的一个成员的地址返回结构体的首地址
 * ptr：结构体type的member成员的地址
 * type：要返回首地址的结构体
 * member：结构的一个成员
 * */
#define container_of(ptr,type,member)	\
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/* list_init
 * 双向链表节点初始化
 * */
__INLINE void list_init(list_t *l)
{
	l->next = l->prev = l;
}

/* list_insert_after
 * 在l节点后面插入n节点
 * l:节点
 * n:要插入的节点
 * */
__INLINE void list_insert_after(list_t *l, list_t *n)
{
	l->next->prev = n;
	n->next = l->next;

	l->next = n;
	n->prev = l;
}

/* list_insert_before
 * 在l节点前面插入n节点
 * l:节点
 * n:要插入的节点
 * */
__INLINE void list_insert_before(list_t *l, list_t *n)
{
	l->prev->next = n;
	n->prev = l->prev;

	l->prev = n;
	n->next = l;
}

/* list_remove
 * 将节点n移出链表
 * n:要移出的节点
 * */
__INLINE void list_remove(list_t *n)
{
	n->next->prev = n->prev;
	n->prev->next = n->next;

	n->next = n->prev = n;
}

/* list_isempty
 * 判断链表是否为空
 * l:要判断的链表的头节点
 * */
__INLINE int list_isempty(const list_t *l)
{
	return l->next == l;
}

/* list_len
 * 返回链表的节点个数
 * l:链表的头节点
 * */
__INLINE unsigned int list_len(const list_t *l)
{
	unsigned int len = 0;
	const list_t *p = l;
	while(p->next != l)
	{
		p = p->next;
		len ++;
	}
	return len;
}

/* 根据结构体的一个成员的地址返回结构体的首地址
 * node：结构体type的member成员的地址
 * type：要返回首地址的结构体
 * member：结构的一个成员
 * */
#define list_entry(node, type, member)	\
	container_of(node, type, member)

/* 遍历链表的所有节点
 * pos:用来遍历的节点指针
 * head:要遍历的链表头节点
 * */
#define list_for_each(pos, head)	\
	for (pos = (head)->next; pos != (head); pos = pos->next)

/* 遍历链表的所有节点
 * pos:用来遍历的节点指针
 * n:用来暂存的节点指针
 * head:要遍历的链表头节点
 * */
#define list_for_each_safe(pos, n, head)	\
	for (pos = (head)->next, n = pos->next; pos != (head);\
		pos = n, n = pos->next)

/* 根据给定的类型遍历链表
 * pos:用来遍历的节点指针
 * head:链表头
 * member:list_struct结构体在遍历结构体中的名称
 * */
#define list_for_each_entry(pos, head, member)	\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
		&pos->member != (head);	\
		pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)	\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
		&pos->member != (head);	\
		pos = n, n = list_entry(n->member.next, typeof(*n), member))

/* 根据节点返回该节点的第一个成员首地址
 * ptr:链表头节点
 * type:要返回首地址的结构体
 * member:结构体的一个成员
 * */
#define list_first_entry(ptr, type, member)	\
	list_entry((ptr)->next, type, member)

/**
 * @brief 初始化赋值一个链表对象。initialize a single list object
 */
#define SLIST_OBJECT_INIT(object) { NULL }

/* slist_init
 * 链表节点初始化
 * */
__INLINE void slist_init(slist_t *l)
{
	l->next = NULL;
}

/* slist_append
 * 在链表l的尾部添加节点n
 * l:要添加到的链表
 * n:要添加的节点
 * */
__INLINE void slist_append(slist_t *l,slist_t *n)
{
	struct slist_node *node;
	node = l;
	while(node->next)
		node = node->next;
	/*在尾部添加结点*/
	node->next = n;
	n->next = NULL;
}

/* slist_insert
 * 在节点l的后面添加节点n
 * l:要插入到那个节点的后面
 * n:要插入的节点
 * */
__INLINE void slist_insert(slist_t *l,slist_t *n)
{
	n->next = l->next;
	l->next = n;
}

/* slist_len
 * 返回当前链表的节点个数
 * l:链表头节点
 * */
__INLINE unsigned int slist_len(const slist_t *l)
{
	unsigned int len = 0;
	const slist_t *list = l->next;
	while(list != NULL)
	{
		list = list->next;
		len ++;
	}
	return len;
}

/* slist_remove
 * 从l链表中移出n节点
 * l:链表头节点
 * n:要移出的节点
 * */
__INLINE slist_t *slist_remove(slist_t *l, slist_t *n)
{
	/*remove slist head*/
	struct slist_node *node = l;
	while(node->next && node->next != n)
		node = node->next;
	/*remove node*/
	if(node->next != (slist_t *)0)
		node->next = node->next->next;
	return l;
}

/* slist_first
 * 返回链表的第一个节点
 * l:链表的头
 * */
__INLINE slist_t *slist_first(slist_t *l)
{
	return l->next;
}

/* slist_tail
 * 返回链表的尾节点
 * l:链表的头
 * */
__INLINE slist_t *slist_tail(slist_t *l)
{
	while(l->next)
		l = l->next;
	return l;
}

/* slist_next
 * 返回n节点的下一个节点
 * n:节点
 * */
__INLINE slist_t *slist_next(slist_t *n)
{
	return n->next;
}

/* slist_isempty
 * 判断链表是否为空
 * l:链表头节点
 * */
__INLINE int slist_isempty(slist_t *l)
{
	return l->next == NULL;
}



/* 根据结构体的一个成员的地址返回结构体的首地址
 * node：结构体type的member成员的地址
 * type：要返回首地址的结构体
 * member：结构的一个成员
 * */
#define slist_entry(node,type,member)	container_of(node,type,member)

/* 遍历单向链表
 * pos:用来遍历的节点指针
 * head:单向链表的头节点
 * */
#define slist_for_each(pos, head)	\
	for (pos = (head)->next; pos != NULL; pos = pos->next)

/* 根据给定的类型遍历链表
 * pos:用来遍历的节点指针
 * head:链表头
 * member:slist_struct结构体在遍历结构体中的名称
 * */
#define slist_for_each_entry(pos, head, member)	\
	for (pos = slist_entry((head)->next, typeof(*pos), member);	\
		&pos->member != (NULL);	\
		pos = slist_entry(pos->member.next, typeof(*pos), member))

/* 获取链表第一个节点的首地址
 * ptr:链表的头节点
 * type:结构体类型
 * member:结构体成员
 * */
#define slist_first_entry(ptr, type, member)	\
	slist_entry((ptr)->next, type, member)

/* 获取链表尾部节点的首地址
 * ptr:链表的头节点
 * type:结构体类型
 * member:结构体成员
 * */
#define slist_tail_entry(ptr, type, member)	\
	slist_entry(slist_tail(ptr), type, member)

#endif /* ULSERVICE_H_ */
