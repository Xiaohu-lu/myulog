/*
 * ulogdef.h
 *
 *  Created on: Oct 8, 2023
 *      Author: hxd
 */

#ifndef ULOGDEF_H_
#define ULOGDEF_H_


struct list_node
{
	struct list_node *next;
	struct list_node *prev;
};
typedef struct list_node list_t;


struct slist_node
{
	struct slist_node *next;
};
typedef struct slist_node slist_t;

#define RT_ALIGN_SIZE		8

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. RT_ALIGN(13, 4)
 * would return 16.
 */
#define RT_ALIGN(size,align)			(((size) + (align) - 1) & ~((align) - 1))


/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. RT_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define RT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))


#endif /* ULOGDEF_H_ */
