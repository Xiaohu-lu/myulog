/*
 * ulog_def.h
 *
 *  Created on: Sep 19, 2023
 *      Author: hxd
 */

#ifndef ULOG_DEF_H_
#define ULOG_DEF_H_
#include <stdint.h>
#include "ulogdef.h"
#include "uprintf.h"


#ifndef UL_NAME_MAX
#define UL_NAME_MAX					12
#endif


#define LOG_LVL_ASSERT				0
#define LOG_LVL_ERROR				3
#define LOG_LVL_WARNING				4
#define LOG_LVL_INFO				6
#define LOG_LVL_DBG					7

#define LOG_FILTER_LVL_SULENT		1
#define LOG_FILTER_LVL_ALL			255



#define DBG_ERROR					LOG_LVL_ERROR
#define DBG_WARNING					LOG_LVL_WARNING
#define DBG_INFO					LOG_LVL_INFO
#define DBG_LOG						LOG_LVL_DBG
#define dbg_log(level, ...)	\
	if((level) <= LOG_LVL)	\
	{						\
		ulog_output(level, LOG_TAG, FALSE,##__VA_ARGS__);\
	}


#ifndef ULOG_OUTPUT_LVL
#define ULOG_OUTPUT_LVL                LOG_LVL_DBG
#endif

#if !defined(LOG_TAG)
	#if defined(DBG_TAG)
		#define LOG_TAG		DBG_TAG
	#elif defined(DBG_SECTION_NAME)
		#define LOG_TAG		DBG_SECTION_NAME
	#else
		#define LOG_TAG		"NO_TAG"
	#endif
#endif/*!define(LOG_TAG)*/


#if !defined(LOG_LVL)
	#if defined(DBG_LVL)
		#define LOG_LVL			DBG_LVL
	#elif defined(DBG_LEVEL)
		#define LOG_LVL			DBG_LEVEL
	#else
		#define LOG_LVL			LOG_LVL_DBG
	#endif
#endif/*!defined(LOG_LVL)*/


#if (LOG_LVL >= LOG_LVL_DBG) && (ULOG_OUTPUT_LVL >= LOG_LVL_DBG)
	#define ulog_d(TAG, ...)		ulog_output(LOG_LVL_DBG, TAG, TRUE, ##__VA_ARGS__)
#else
	#define ulog_d(TAG, ...)
#endif/*(LOG_LVL >= LOG_LVL_DBG) && (ULOG_OUTPUT_LVL >= LOG_LVL_DBG)*/

#if (LOG_LVL >= LOG_LVL_INFO) && (ULOG_OUTPUT_LVL >= LOG_LVL_INFO)
	#define ulog_i(TAG, ...)		ulog_output(LOG_LVL_INFO, TAG, TRUE, ##__VA_ARGS__)
#else
	#define ulog_i(TAG, ...)
#endif/*(LOG_LVL >= LOG_LVL_INFO) && (ULOG_OUTPUT_LVL >= LOG_LVL_INFO)*/

#if (LOG_LVL >= LOG_LVL_WARNING) && (ULOG_OUTPUT_LVL >= LOG_LVL_WARNING)
	#define ulog_w(TAG, ...)		ulog_output(LOG_LVL_WARNING, TAG, TRUE, ##__VA_ARGS__)
#else
	#define ulog_w(TAG, ...)
#endif/*(LOG_LVL >= LOG_LVL_WARNING) && (ULOG_OUTPUT_LVL >= LOG_LVL_WARNING)*/

#if (LOG_LVL >= LOG_LVL_ERROR) && (ULOG_OUTPUT_LVL >= LOG_LVL_ERROR)
	#define ulog_e(TAG, ...)		ulog_output(LOG_LVL_ERROR, TAG, TRUE, ##__VA_ARGS__)
#else
	#define ulog_e(TAG, ...)
#endif/*(LOG_LVL >= LOG_LVL_ERROR) && (ULOG_OUTPUT_LVL >= LOG_LVL_ERROR)*/

#if (LOG_LVL >= LOG_LVL_DBG) && (ULOG_OUTPUT_LVL >= LOG_LVL_DBG)
    #define ulog_hex(TAG, width, buf, size)     ulog_hexdump(TAG, width, buf, size)
#else
    #define ulog_hex(TAG, width, buf, size)
#endif /* (LOG_LVL >= LOG_LVL_DBG) && (ULOG_OUTPUT_LVL >= LOG_LVL_DBG) */




#ifdef ULOG_ASSERT_ENABLE
	#define ULOG_ASSERT(EXPR)		\
	if(!(EXPR))						\
	{								\
		ulog_output(LOG_LVL_ASSERT, LOG_TAG, TRUE,"(%s) has assert failed at %s:%ld.",#EXPR, __FUNCTION__, __LINE__);\
		ulog_flush()				\
		while(1)					\
	}
#else
	#define ULOG_ASSERT(EXPR)
#endif/*ULOG_ASSERT_ENABLE*/


#if !defined(ASSERT)
	#define ASSERT				ULOG_ASSERT
#endif/*!defined(ASSERT)*/

extern void debug_print(const char* fmt, ...);
//extern void uprintf(const char* fmt, ...);
//#define uprintf(fmt,...)		debug_print(fmt, ##__VA_ARGS__)

#ifndef UL_ASSERT
#define UL_ASSERT(EX)			\
	if(!(EX))					\
	{							\
		uprintf("%s[%d]£ºASSERT ERROR\n", __FUNCTION__, __LINE__);						\
		while(1);					\
	}
#endif/*UL_ASSERT*/


/*buffer size for every line's log*/
#ifndef ULOG_LINE_BUF_SIZE
#define ULOG_LINE_BUF_SIZE			128
#endif

/*output filter's tag max length*/
#ifndef ULOG_FILTER_TAG_MAX_LEN
#define ULOG_FILTER_TAG_MAX_LEN		23
#endif

/*output filter's keyword max length*/
#ifndef ULOG_FILTER_KW_MAX_LEN
#define ULOG_FILTER_KW_MAX_LEN		15
#endif

#ifndef ULOG_NEWLINE_SIGN
#define ULOG_NEWLINE_SIGN			"\r\n"
#endif

#define ULOG_FRAME_MAGIC			0x10

/*tag's level filter*/
struct ulog_tag_lvl_filter
{
	char tag[ULOG_FILTER_TAG_MAX_LEN + 1];
	uint32_t level;
	slist_t list;
};
typedef struct ulog_tag_lvl_filter *ulog_tag_lvl_filter_t;

struct ulog_frame
{
	/*magic word is 0x10('lo')*/
	uint32_t magic:8;
	uint32_t is_raw:1;
	uint32_t log_len:23;
	uint32_t level;
	const char *log;
	const char *tag;
};
typedef struct ulog_frame *ulog_frame_t;

struct ulog_backend
{
	char	name[UL_NAME_MAX];
	bool	support_color;
	uint32_t	out_level;
	void (*init) (struct ulog_backend *backend);
	void (*output) (struct ulog_backend *backend,uint32_t level,const char *tag,bool is_raw,const char *log,size_t len);
	void (*flush) (struct ulog_backend *backend);
	void (*deinit) (struct ulog_backend *backend);
	/* The filter will be call before output. It will return TRUE when the filter condition is math. */
	bool (*filter) (struct ulog_backend *backend,uint32_t level,const char *tag,bool is_raw,const char *log,size_t len);
	slist_t list;

};
typedef struct ulog_backend *ulog_backend_t;
typedef bool (*ulog_backend_filter_t)(struct ulog_backend *backend, uint32_t level, const char *tag, bool is_raw, const char *log, size_t len);




#endif /* ULOG_DEF_H_ */
