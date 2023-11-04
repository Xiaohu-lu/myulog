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

/* ���ݽṹ���һ����Ա�ĵ�ַ���ؽṹ����׵�ַ
 * ptr���ṹ��type��member��Ա�ĵ�ַ
 * type��Ҫ�����׵�ַ�Ľṹ��
 * member���ṹ��һ����Ա
 * */
#define container_of(ptr,type,member)	\
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/* list_init
 * ˫������ڵ��ʼ��
 * */
__INLINE void list_init(list_t *l)
{
	l->next = l->prev = l;
}

/* list_insert_after
 * ��l�ڵ�������n�ڵ�
 * l:�ڵ�
 * n:Ҫ����Ľڵ�
 * */
__INLINE void list_insert_after(list_t *l, list_t *n)
{
	l->next->prev = n;
	n->next = l->next;

	l->next = n;
	n->prev = l;
}

/* list_insert_before
 * ��l�ڵ�ǰ�����n�ڵ�
 * l:�ڵ�
 * n:Ҫ����Ľڵ�
 * */
__INLINE void list_insert_before(list_t *l, list_t *n)
{
	l->prev->next = n;
	n->prev = l->prev;

	l->prev = n;
	n->next = l;
}

/* list_remove
 * ���ڵ�n�Ƴ�����
 * n:Ҫ�Ƴ��Ľڵ�
 * */
__INLINE void list_remove(list_t *n)
{
	n->next->prev = n->prev;
	n->prev->next = n->next;

	n->next = n->prev = n;
}

/* list_isempty
 * �ж������Ƿ�Ϊ��
 * l:Ҫ�жϵ������ͷ�ڵ�
 * */
__INLINE int list_isempty(const list_t *l)
{
	return l->next == l;
}

/* list_len
 * ��������Ľڵ����
 * l:�����ͷ�ڵ�
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

/* ���ݽṹ���һ����Ա�ĵ�ַ���ؽṹ����׵�ַ
 * node���ṹ��type��member��Ա�ĵ�ַ
 * type��Ҫ�����׵�ַ�Ľṹ��
 * member���ṹ��һ����Ա
 * */
#define list_entry(node, type, member)	\
	container_of(node, type, member)

/* ������������нڵ�
 * pos:���������Ľڵ�ָ��
 * head:Ҫ����������ͷ�ڵ�
 * */
#define list_for_each(pos, head)	\
	for (pos = (head)->next; pos != (head); pos = pos->next)

/* ������������нڵ�
 * pos:���������Ľڵ�ָ��
 * n:�����ݴ�Ľڵ�ָ��
 * head:Ҫ����������ͷ�ڵ�
 * */
#define list_for_each_safe(pos, n, head)	\
	for (pos = (head)->next, n = pos->next; pos != (head);\
		pos = n, n = pos->next)

/* ���ݸ��������ͱ�������
 * pos:���������Ľڵ�ָ��
 * head:����ͷ
 * member:list_struct�ṹ���ڱ����ṹ���е�����
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

/* ���ݽڵ㷵�ظýڵ�ĵ�һ����Ա�׵�ַ
 * ptr:����ͷ�ڵ�
 * type:Ҫ�����׵�ַ�Ľṹ��
 * member:�ṹ���һ����Ա
 * */
#define list_first_entry(ptr, type, member)	\
	list_entry((ptr)->next, type, member)

/**
 * @brief ��ʼ����ֵһ���������initialize a single list object
 */
#define SLIST_OBJECT_INIT(object) { NULL }

/* slist_init
 * ����ڵ��ʼ��
 * */
__INLINE void slist_init(slist_t *l)
{
	l->next = NULL;
}

/* slist_append
 * ������l��β����ӽڵ�n
 * l:Ҫ��ӵ�������
 * n:Ҫ��ӵĽڵ�
 * */
__INLINE void slist_append(slist_t *l,slist_t *n)
{
	struct slist_node *node;
	node = l;
	while(node->next)
		node = node->next;
	/*��β����ӽ��*/
	node->next = n;
	n->next = NULL;
}

/* slist_insert
 * �ڽڵ�l�ĺ�����ӽڵ�n
 * l:Ҫ���뵽�Ǹ��ڵ�ĺ���
 * n:Ҫ����Ľڵ�
 * */
__INLINE void slist_insert(slist_t *l,slist_t *n)
{
	n->next = l->next;
	l->next = n;
}

/* slist_len
 * ���ص�ǰ����Ľڵ����
 * l:����ͷ�ڵ�
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
 * ��l�������Ƴ�n�ڵ�
 * l:����ͷ�ڵ�
 * n:Ҫ�Ƴ��Ľڵ�
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
 * ��������ĵ�һ���ڵ�
 * l:�����ͷ
 * */
__INLINE slist_t *slist_first(slist_t *l)
{
	return l->next;
}

/* slist_tail
 * ���������β�ڵ�
 * l:�����ͷ
 * */
__INLINE slist_t *slist_tail(slist_t *l)
{
	while(l->next)
		l = l->next;
	return l;
}

/* slist_next
 * ����n�ڵ����һ���ڵ�
 * n:�ڵ�
 * */
__INLINE slist_t *slist_next(slist_t *n)
{
	return n->next;
}

/* slist_isempty
 * �ж������Ƿ�Ϊ��
 * l:����ͷ�ڵ�
 * */
__INLINE int slist_isempty(slist_t *l)
{
	return l->next == NULL;
}



/* ���ݽṹ���һ����Ա�ĵ�ַ���ؽṹ����׵�ַ
 * node���ṹ��type��member��Ա�ĵ�ַ
 * type��Ҫ�����׵�ַ�Ľṹ��
 * member���ṹ��һ����Ա
 * */
#define slist_entry(node,type,member)	container_of(node,type,member)

/* ������������
 * pos:���������Ľڵ�ָ��
 * head:���������ͷ�ڵ�
 * */
#define slist_for_each(pos, head)	\
	for (pos = (head)->next; pos != NULL; pos = pos->next)

/* ���ݸ��������ͱ�������
 * pos:���������Ľڵ�ָ��
 * head:����ͷ
 * member:slist_struct�ṹ���ڱ����ṹ���е�����
 * */
#define slist_for_each_entry(pos, head, member)	\
	for (pos = slist_entry((head)->next, typeof(*pos), member);	\
		&pos->member != (NULL);	\
		pos = slist_entry(pos->member.next, typeof(*pos), member))

/* ��ȡ�����һ���ڵ���׵�ַ
 * ptr:�����ͷ�ڵ�
 * type:�ṹ������
 * member:�ṹ���Ա
 * */
#define slist_first_entry(ptr, type, member)	\
	slist_entry((ptr)->next, type, member)

/* ��ȡ����β���ڵ���׵�ַ
 * ptr:�����ͷ�ڵ�
 * type:�ṹ������
 * member:�ṹ���Ա
 * */
#define slist_tail_entry(ptr, type, member)	\
	slist_entry(slist_tail(ptr), type, member)

#endif /* ULSERVICE_H_ */
