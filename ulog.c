#include "ulog_types.h"
#include "platform.h"


/* ulog object struct */
struct ul_ulog
{
	ul_bool_t init_ok;						/*初始化标志*/
	ul_bool_t output_lock_enable;			/*是否使能锁*/
	platform_mutex_t output_locker;			/*互斥量句柄*/
	/* all backends */
	ul_slist_t backend_list;
	/* the thread log's line buffer */
	char log_buf_th[ULOG_LINE_BUF_SIZE + 1];/*行缓冲区*/

#ifdef ULOG_USING_ISR_LOG
	ul_base_t output_locker_isr_lvl;
	char log_buf_isr[ULOG_LINE_BUF_SIZE + 1];
#endif	/*ULOG_USING_ISR_LOG*/

#ifdef ULOG_USING_ASYNC_OUTPUT
	ul_bool_t async_enabled;
	ul_rbb_t  async_rbb;
	/* ringbuffer for log_raw function only */
	struct ul_ringbuffer *async_rb;
	platform_thread_t async_th;
	platform_mutex_t async_notice;
#endif	/*ULOG_USING_ASYNC_OUTPUT*/

#ifdef ULOG_USING_FILTER
	struct 
	{
		ul_slist_t tag_lvl_list;
		ul_uint32_t level;
		char tag[ULOG_FILTER_TAG_MAX_LEN + 1];
		char keyword[ULOG_FILTER_KW_MAX_LEN + 1];
	}filter;
#endif /*ULOG_USING_FILTER*/

};



/* ulog local object */
static struct ul_ulog ulog = {0};




/* ulog_voutput
 * ulog输出
 * level:		输出等级
 * tag：			标签
 * newline：		是否有换行符
 * hex_buf:		hex数据缓冲区
 * hex_size:	hex数据大小
 * hex_width:	hexlog宽度,一行输出多少个hex
 * hex_addr:	hex数据地址
 * format：		输出格式
 * args:		可变形参列表
 */
void ulog_voutput(ul_uint32_t level, const char *tag, ul_bool_t newline, const ul_uint8_t *hex_buf, ul_size_t hex_size, ul_size_t hex_width, ul_base_t hex_addr, const char *format, va_list args)
{
	static ul_bool_t ulog_voutput_recursion = UL_FALSE;
	char *log_buf = UL_NULL;
	static ul_size_t log_len = 0;

	UL_ASSERT(tag);/*断言,tag不能为NULL*/
	UL_ASSERT((format && !hex_buf) || (!format && hex_buf));/*断言,format和hex_buf不能同时为NULL*/
#ifndef ULOG_USING_SYSLOG		/*不使用syslog*/
	UL_ASSERT(level <= LOG_LVL_DBG);
#else
	UL_ASSERT(LOG_PRI(level) <= LOG_DEBUG);
#endif

	if(!ulog.init_ok)/*ulog 为初始化*/
	{
		return;
	}

#ifdef ULOG_USING_FILTER
#ifndef ULOG_USING_SYSLOG	/*不使用syslog*/
	if(level > ulog.filter.level || level > ulog_tag_lvl_filter_get(tag))
	{
		return;
	}
#else
	if(((LOG_MASK(LOG_PRI(level)) & ulog.filter.level) == 0) || ((LOG_MASK(LOG_PRI(level)) & ulog_tag_lvl_filter_get(tag)) == 0))
	{
		return;
	}
#endif /*ULOG_USING_SYSLOG*/
	else if(!ul_strstr(tag, ulog.filter.tag))
	{
		return;
	}
#endif	/*ULOG_USING_FILTER*/
	


}



/* ulog_output
 * ulog输出
 * level:		输出等级
 * tag：			标签
 * newline：		是否有换行符
 * format：		输出格式
 */
void ulog_output(ul_uint32_t level, const char *tag, ul_bool_t newline, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ulog_voutput(level, tag, newline, UL_NULL, 0, 0, 0, format, args);

	va_end(args);
}







ul_uint32_t ulog_tag_lvl_filter_get(const char *tag)
{
	ul_slist_t *node;
	ulog_tag_lvl_filter_t tag_lvl = UL_NULL;
	ul_uint32_t level = LOG_FILTER_LVL_ALL;

	
}







