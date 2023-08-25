#ifndef __ULOG_DEF_H
#define __ULOG_DEF_H


/* logger level, the number is compatible for syslog */
#define LOG_LVL_ASSERT				0
#define LOG_LVL_ERROR				3
#define LOG_LVL_WARNING				4
#define LOG_LVL_INFO				6
#define LOG_LVL_DBG					7


/* the output silent level and all level for filter setting */
#ifndef ULOG_USING_SYSLOG
#define LOG_FILTER_LVL_SILENT		0
#define LOG_FILTER_LVL_ALL			7
#else
#define LOG_FILTER_LVL_SILENT		1
#define LOG_FILTER_LVL_ALL			255
#endif	/*ULOG_USING_SYSLOG*/


/* compatible for rtdbg */
#undef LOG_D
#undef LOG_I
#undef LOG_W
#undef LOG_E
#undef LOG_RAW
#undef DBG_ERROR
#undef DBG_WARNING
#undef DBG_INFO
#undef DBG_LOG
#undef dbg_log
#define DBG_ERROR                      LOG_LVL_ERROR
#define DBG_WARNING                    LOG_LVL_WARNING
#define DBG_INFO                       LOG_LVL_INFO
#define DBG_LOG                        LOG_LVL_DBG



















/* buffer size for erery line's log */
#ifndef ULOG_LINE_BUF_SIZE
#define ULOG_LINE_BUF_SIZE			128
#endif	/*ULOG_LINE_BUF_SIZE*/

/* output filter's tag max length */
#ifndef ULOG_FILTER_TAG_MAX_LEN
#define ULOG_FILTER_TAG_MAX_LEN		23
#endif  /*ULOG_FILTER_TAG_MAX_LEN*/

/* output filter's keyword max length */
#ifndef ULOG_FILTER_KW_MAX_LEN
#define ULOG_FILTER_KW_MAX_LEN		15
#endif	/*ULOG_FILTER_KW_MAX_LEN*/





/* tag's level filter */
struct ulog_tag_lvl_filter
{
	char tag[ULOG_FILTER_TAG_MAX_LEN + 1];
	ul_uint32_t level;
	ul_slist_t	list;
};
typedef struct ulog_tag_lvl_filter *ulog_tag_lvl_filter_t;




#endif



