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

#ifndef ULOG_NEWLINE_SIGN
#define ULOG_NEWLINE_SIGN			"\r\n"
#endif




/* tag's level filter */
struct ulog_tag_lvl_filter
{
	char tag[ULOG_FILTER_TAG_MAX_LEN + 1];
	ul_uint32_t level;
	ul_slist_t	list;
};
typedef struct ulog_tag_lvl_filter *ulog_tag_lvl_filter_t;



struct ulog_backend
{
	char name[paltform_NAME_MAX];
	ul_bool_t support_color;
	ul_uint32_t out_level;
	void (*init) (struct ulog_backend *backend);
	void (*output) (struct ulog_backend *backend, ul_uint32_t level, const char *tag, ul_bool_t is_raw, const char *log, ul_size_t len);
	void (*flush) (struct ulog_backend *backend);
	void (*deinit) (struct ulog_backend *backend);
	/* the filter will be call before output. it will return ture when the filter condition is math. */
	ul_bool_t (*filter) (struct ulog_backend *backend, ul_uint32_t level, const char *tag, ul_bool_t is_raw, const char *log, ul_size_t len);
	ul_slist_t list;
};

typedef struct ulog_backend *ulog_backend_t;
typedef rt_bool_t (*ulog_backend_filter_t)(struct ulog_backend *backend, rt_uint32_t level, const char *tag, rt_bool_t is_raw, const char *log, rt_size_t len);



#endif



