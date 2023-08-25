#include "ulservice.h"
#include "platform.h"

#ifdef ULOG_USING_COLOR
/**
 * CSI(Control Sequence Introducer/Initiator) sign
 * more information on https://en.wikipedia.org/wiki/ANSI_escape_code
 */
#define CSI_START			"\033["
#define CSI_END				"\033[0m"
/* output log front color */
#define F_BLACK				"30m"
#define F_RED				"31m"
#define F_GREEN				"32m"
#define F_YELLOW			"33m"
#define F_BLUE				"34m"
#define F_MAGENTA			"35m"
#define F_CYAN				"36m"
#define F_WHITE				"37m"

/* output log default color definition */
#ifndef ULOG_COLOR_DEBUG
#define ULOG_COLOR_DEBUG               RT_NULL
#endif
#ifndef ULOG_COLOR_INFO
#define ULOG_COLOR_INFO                (F_GREEN)
#endif
#ifndef ULOG_COLOR_WARN
#define ULOG_COLOR_WARN                (F_YELLOW)
#endif
#ifndef ULOG_COLOR_ERROR
#define ULOG_COLOR_ERROR               (F_RED)
#endif
#ifndef ULOG_COLOR_ASSERT
#define ULOG_COLOR_ASSERT              (F_MAGENTA)
#endif
#endif /* ULOG_USING_COLOR */

#if ULOG_LINE_BUF_SIZE < 80
#error "the log line buffer size must more than 80"
#endif	/*ULOG_USING_COLOR*/




/* ulog object struct */
struct ul_ulog
{
	ul_bool_t init_ok;						/*初始化标志*/
	ul_bool_t output_lock_enabled;			/*是否使能锁*/
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


/* level output info */
static const char * const level_output_info[] = 
{
	"A/",
	UL_NULL,
	UL_NULL,
	"E/",
	"W/",
	UL_NULL,
	"I/",
	"D/",
};
	

#ifdef ULOG_USING_COLOR
/* color output info */
static const char * const color_output_info[] = 
{
	ULOG_COLOR_ASSERT,
	UL_NULL,
	UL_NULL,
	ULOG_COLOR_ERROR,
	ULOG_COLOR_WARN,
	UL_NULL,
	ULOG_COLOR_INFO,
	ULOG_COLOR_DEBUG,
};
#endif /*ULOG_USING_COLOR*/


/* ulog local object */
static struct ul_ulog ulog = {0};


/* ulog_strcpy
 * 将src数据拷贝到dst中,最大拷贝(128-cur_len字节)
 * cur_len:		
 * dst:		目标地址
 * src：		源数据地址
 * return:	返回拷贝了多少数据(字节)
 */
ul_size_t ulog_strcpy(ul_size_t cur_len, char *dst, const char *src)
{
    const char *src_old = src;

    UL_ASSERT(dst);/*断言,判断地址不为NULL*/
    UL_ASSERT(src);

    while (*src != 0)
    {
        /* make sure destination has enough space */
        if (cur_len++ < ULOG_LINE_BUF_SIZE)
        {
            *dst++ = *src++;
        }
        else
        {
            break;
        }
    }
    return src - src_old;
}

/* ulog_ultoa
 * 将一个无符号整数转换为字符串
 * s:	转换后字符串存放地址
 * n:	要转换的整形数
 * return:	转换后字符串长度,多一个'\0'
 */
ul_size_t ulog_ultoa(char *s, unsigned long int n)
{
    ul_size_t i = 0, j = 0, len = 0;
    char swap;

    do
    {
        s[len++] = n % 10 + '0';
    } while (n /= 10);
    s[len] = '\0';
    /* reverse string */
    for (i = 0, j = len - 1; i < j; ++i, --j)
    {
        swap = s[i];
        s[i] = s[j];
        s[j] = swap;
    }
    return len;
}


/* output_unlock
 * log输出资源释放
 */
static void output_unlock(void)
{
	if(ulog.output_lock_enabled == UL_FALSE)
	{
		return;
	}
	/* if the scheduler is started and in thread context */
	/* get_nest() == 0 表明当前在线程中,没在中断服务函数中,在中断服务函数中不能使用互斥量
	 * thread_self() != NULL 表明当前调度器已经启动
	 */
	if(platform_interrupt_get_nest() == 0 && platform_thread_self() != UL_NULL)
	{
		/* 解锁 */
		platform_Mutex_Give(&ulog.output_locker);
	}
	else
	{
#ifdef ULOG_USING_ISR_LOG
		platform_interrupt_enable(ulog.output_locker_isr_lvl);/*使能中断*/
#endif
	}
	
}

/* output_lock
 * 输出上锁
 */
static void output_lock(void)
{
	/* 是否使能锁 */
	if(ulog.output_lock_enabled == UL_FALSE)
	{
		return;
	}

	/* if the scheduler is started and in thread context */
	/* get_nest() == 0 表明当前在线程中,没在中断服务函数中,在中断服务函数中不能使用互斥量
	 * thread_self() != NULL 表明当前调度器已经启动
	 */
	if(platform_interrupt_get_nest() == 0 && platform_thread_self() != UL_NULL)
	{
		/* 上锁 */
		platform_Mutex_Take(&ulog.output_locker,platform_MAX_DELAY);
	}
	else
	{
#ifdef ULOG_USING_ISR_LOG
		ulog.output_locker_isr_lvl = platform_interrupt_disable();
#endif
	}
}


/* ulog_output_lock_enabled
 * 是否使能互斥锁
 * enable:RT_TURE:使能,RT_FALSE:不使能
 */
void ulog_output_lock_enabled(ul_bool_t enabled)
{
    ulog.output_lock_enabled = enabled;
}

/* get_log_buf
 * 返回log缓冲区
 * 在中断服务函数中,返回log_buf_isr,or return log_buf_th
 */
static char *get_log_buf(void)
{
	if(platform_interrupt_get_nest() == 0)/*每在中断服务函数中*/
	{
		return ulog.log_buf_th;
	}
	else/*在中断服务函数中*/
	{
#ifdef ULOG_USING_ISR_LOG
		return ulog.log_buf_isr;
#else
		ul_kprintf("Error: Current mode not supported run in ISR. Please enable ULOG_USING_ISR_LOG.\n");
		return UL_NULL;
#endif
	}
}

/* ulog_head_formater
 * 格式化输出:添加头 "\033[35m[time] A/"
 */
ul_size_t ulog_head_formater(char *log_buf, ul_uint32_t level, const char *tag)
{
	/* the caller has locker, so it can use static variable for reduce stack usage */
	static ul_size_t log_len;

	UL_ASSERT(log_buf);
	UL_ASSERT(level <= LOG_LVL_DBG);
	UL_ASSERT(tag);

#ifdef ULOG_USING_COLOR	/*使用颜色*/
	/* add CSI start sign and color info */
	if(color_output_info[level])
	{
		log_len += ulog_strcpy(log_len, log_buf + log_len, CSI_START);
		log_len += ulog_strcpy(log_len, log_buf + log_len, color_output_info[level]);
	}
#endif	/*ULOG_USING_COLOR*/

	log_buf[log_len] = '\0';

#ifdef ULOG_OUTPUT_TIME
	/* add time info */
{
#ifdef ULOG_TIME_USING_TIMESTAMP
	static struct timeval now;
	static struct tm *tm, tm_tmp;
	static ul_bool_t check_usec_support = UL_FALSE, usec_is_support = UL_FALSE;
	time_t t = (time_t)0;

	if(gettimeofday(&now,UL_NULL) >= 0)
	{
		t = now.tv_sec;
	}
	tm = localtime_r(&t, &tm_tmp);
	/* show the time format MM-DD HH:MM:SS */
	ul_snprintf(log_buf + log_len, ULOG_LINE_BUF_SIZE - log_len, "%02d-%02d %02d:%02d:%02d", tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	/* check the microseconds support when kernel is startup */	
	if(t > 0 && !check_usec_support && platform_thread_self() != UL_NULL)
	{
		long old_usec = now.tv_usec;
		/* delay some time for wait microseconds changed */
		platform_thread_mdelay(10);
		gettimeofday(&now, UL_NULL);
		check_usec_support = UL_TRUE;
		/* the microseconds is not equal between two gettimeofday calls */
		if(now.tv_usec != old_usec)
			usec_is_support = UL_TRUE;
	}
	if(usec_is_support)
	{
		/*show the millisecond */
		log_len += ul_strlen(log_buf + log_len);
		ul_snprintf(log_buf + log_len, ULOG_LINE_BUF_SIZE - log_len, ".%03d", now.tvusec / 1000);
	}
#else	/*添加时间信息*/
	static ul_size_t tick_len = 0;
	log_buf[log_len] = '[';
	tick_len = ulog_ultoa(log_buf + log_len + 1,platform_getTickCount());
	log_buf[log_len + 1 + tick_len] = ']';
	log_buf[log_len + 1 + tick_len + 1] = '\0';
#endif /*ULOG_TIME_USING_TIMESTAMP*/
	log_len += ul_strlen(log_buf + log_len);
}
#endif /*ULOG_OUTPUT_TIME*/


#ifdef ULOG_OUTPUT_LEVEL
#ifdef ULOG_OUTPUT_TIME
	log_len += ulog_strcpy(log_len, log_buf + log_len, " ");
#endif
	/* add level info */
	log_len += ulog_strcpy(log_len, log_buf + log_len, level_output_info[level]);
#endif /*ULOG_OUTPUT_LEVEL*/
	
}






/* ulog_formater
 * 格式化输出字符到缓冲区
 * log_buf:		字符串缓冲区
 * level:		输出等级
 * tag:			标签
 * newline:		是否使用换行
 * format：		格式
 * args:		可变形参列表
 */
ul_size_t ulog_formater(char *log_buf, ul_uint32_t level, const char *tag, ul_bool_t newline, const char *format, va_list args)
{
	/* the caller has locker, so it can use static variable for reduce stack usage
     */
	static ul_size_t log_len;
	static int fmt_result;

	UL_ASSERT(log_buf);
	UL_ASSERT(format);

	/* log head
	 */
	log_len = ulog_head_formater(log_buf, level, tag);

	fmt_result = ul_vsnprintf(log_buf + log_len, ULOG_LINE_BUF_SIZE - log_len, format, args);

	if((log_len + fmt_result <= ULOG_LINE_BUF_SIZE) && (fmt_result > -1))
	{
		log_len += fmt_result;
	}
	else
	{
		/* using max length
	 	 */
	 	log_len = ULOG_LINE_BUF_SIZE;
	}
	/* log tail
	 */
	return ulog_tail_formater(log_buf, log_len, newline, level);
}




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
	UL_ASSERT(level <= LOG_LVL_DBG);/*判断输出等级level是否满足要求*/
#else
	UL_ASSERT(LOG_PRI(level) <= LOG_DEBUG);
#endif

	if(!ulog.init_ok)/*ulog 为初始化*/
	{
		return;
	}

#ifdef ULOG_USING_FILTER
#ifndef ULOG_USING_SYSLOG	/*不使用syslog*/
	/* level越大说明越不重要,
	 * 如果输出等级大于过滤器或者tag的等级则不输出
     */
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
	/*在tag字符串中,没找到ulog.filter.tag*/
	else if(!ul_strstr(tag, ulog.filter.tag))
	{
		return;
	}
#endif	/*ULOG_USING_FILTER*/

	/* 获取ulog的log缓冲区
  	 */
	log_buf = get_log_buf();

	/* lock output
	 */
	output_lock();

	/* If there is a recursion, we use a simple way */
	if((ulog_voutput_recursion == UL_TRUE) && hex_buf == UL_NULL)
	{
		ul_kprintf(format, args);
		if(newline == UL_TRUE)
		{
			ul_kprintf(ULOG_NEWLINE_SIGN);/*输出换行*/
		}
		output_unlock();
		return;
	}

	ulog_voutput_recursion = UL_TRUE;

	/* hex_buf == NULL 正常输出
	 * hex_buf != NULL 表示要输出hex数据
	 */
	if(hex_buf == UL_NULL)
	{
#ifndef ULOG_USING_SYSLOG
		log_len = ulog_formater(log_buf, level, tag, newline, format, args);
#else	
		extern ul_size_t syslog_formater(char *log_buf, ul_uint8_t level, const char *tag, ul_bool_t newline, const char *format, va_list args);
		log_len = syslog_formater(log_buf, level, tag, newline, format, args);
#endif	/*ULOG_USING_SYSLOG*/
	}
	else
	{
		/* hex mode */
		log_len = ulog_hex_formater(log_buf, tag, hex_buf, hex_size, hex_width, hex_addr);
	}
	
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






/* ulog_tag_lvl_filter_get
 * 找到tag标签的输出等级level
 * tag：		标签
 * return：	该标签的输出等级
 */
ul_uint32_t ulog_tag_lvl_filter_get(const char *tag)
{
	ul_slist_t *node;
	ulog_tag_lvl_filter_t tag_lvl = UL_NULL;
	ul_uint32_t level = LOG_FILTER_LVL_ALL;

	if(!ulog.init_ok)/*未初始化*/
	{
		return level;
	}

	/* lock output
	 */
	output_lock();
	/* 遍历ulog的tag_lvl_filter链表,
	 * 找到tag的level等级
	 */
	for(node = ul_slist_first(ulog_tag_lvl_list_get()); node; node = ul_slist_next(node))
	{
		tag_lvl = ul_slist_entry(node, struct ulog_tag_lvl_filter, list);
		if(!ul_strncmp(tag_lvl->tag, tag, ULOG_FILTER_TAG_MAX_LEN))/*表明找到tag的过滤器链表节点*/
		{
			level = tag_lvl->level;/*返回输出level*/
			break;
		}
	}

	/* unlock output
	 */
	output_unlock();
	return level;
}

/* ulog_tag_lvl_list_get
 * 获取ulogtag标签链表的根节点
 * return 返回ulog过滤器的根节点
 */
ul_slist_t *ulog_tag_lvl_list_get(void)
{
	return &ulog.filter.tag_lvl_list;
}





