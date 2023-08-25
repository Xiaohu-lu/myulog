#ifndef __ULOG_TYPES_H
#define __ULOG_TYPES_H

typedef signed char 				ul_int8_t;			/**<  8bit integer type */
typedef signed short				ul_int16_t;			/**< 16bit integer type */
typedef signed int					ul_int32_t;			/**< 32bit integer type */
typedef unsigned char 				ul_uint8_t;			/**<  8bit unsigned integer type */
typedef unsigned short				ul_uint16_t;		/**< 16bit unsigned integer type */
typedef unsigned int				ul_uint32_t;		/**< 32bit unsigned integer type */

typedef int							ul_bool_t;			/**< boolean type */
typedef long						ul_base_t;			/**< Nbit CPU related date type */
typedef unsigned long				ul_ubase_t;			/**< Nbit unsigned CPU related data type */

typedef ul_base_t					ul_err_t;			/**< Type for error number */
typedef ul_uint32_t					ul_time_t;			/**< Type for time stamp */
typedef ul_uint32_t					ul_tick_t;			/**< Type for tick count */
typedef ul_base_t					ul_flag_t;			/**< Type for flags */
typedef ul_ubase_t					ul_size_t;			/**< Type for size number */
typedef ul_ubase_t					ul_dev_t;			/**< Type for device */
typedef ul_base_t					ul_off_t;			/**< Type for offset */

/*boolean type definitions*/
#define UL_TRUE						1		/*boolean true*/
#define UL_FALSE					0		/*boolean fails*/

#define UL_NULL						(0)		/*NULL*/

#define UL_ASSERT(EX)						





/**
 * Single List structure
 */
struct ul_slist_node
{
    struct ul_slist_node *next;                         /**< point to next node. */
};
typedef struct ul_slist_node ul_slist_t;                /**< Type for single list. */




#endif


