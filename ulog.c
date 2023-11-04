/*
 * ulog.c
 *
 *  Created on: Sep 19, 2023
 *      Author: hxd
 */
#include "H01_Device.h"

#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"
#include "semphr.h"
#include "ulogdef.h"
#include "ringblk_buf.h"
#include "ringbuffer.h"
#include "dprintf.h"

#include "ulog.h"
#include "ulservice.h"
#define ULOG_USING_COLOR		/*ʹ����ɫ*/
#define ULOG_OUTPUT_TIME		/*ʹ��TIME*/
#define ULOG_OUTPUT_LEVEL		/*ʹ�õȼ�*/
#define ULOG_OUTPUT_TAG			/*ʹ�ñ�ǩ*/
#define ULOG_USING_FILTER		/*ʹ�ù�����*/
#define ULOG_USING_ASYNC_OUTPUT	/*ʹ���첽���*/

#define ULOG_ASYNC_OUTPUT_BUF_SIZE 	2048	/*���ο��С*/
#define ULOG_ASYNC_OUTPUT_SIZE		2048	/*���λ�������С*/


/* the number which is max stored line logs */
#ifndef ULOG_ASYNC_OUTPUT_STORE_LINES
#define ULOG_ASYNC_OUTPUT_STORE_LINES  (ULOG_ASYNC_OUTPUT_BUF_SIZE * 3 / 2 / 80)
#endif

#ifdef ULOG_USING_COLOR
/**
 * CSI(Control Sequence Introducer/Initiator) sign
 * more information on https://en.wikipedia.org/wiki/ANSI_escape_code
 */
#define CSI_START                      "\033["
#define CSI_END                        "\033[0m"
/* output log front color */
#define F_BLACK                        "30m"
#define F_RED                          "31m"
#define F_GREEN                        "32m"
#define F_YELLOW                       "33m"
#define F_BLUE                         "34m"
#define F_MAGENTA                      "35m"
#define F_CYAN                         "36m"
#define F_WHITE                        "37m"
/* output log default color definition */
#ifndef ULOG_COLOR_DEBUG
#define ULOG_COLOR_DEBUG               NULL
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

struct ulog_t
{
	bool	init_ok;
	bool	output_lock_enabled;
	SemaphoreHandle_t	output_locker;
	/*all backends*/
	slist_t backend_list;
	char	log_buf_th[ULOG_LINE_BUF_SIZE + 1];

#ifdef ULOG_USING_ASYNC_OUTPUT	/*�첽���*/
	bool	async_enabled;
	rbb_t	async_rbb;
	/*ringbuffer for log_raw function only*/
	struct ringbuffer *async_rb;
	TaskHandle_t async_th;
	SemaphoreHandle_t async_notice;
#endif/*ULOG_USING_ASYNC_OUTPUT*/

#ifdef ULOG_USING_FILTER
	struct
	{
		/*all tag's level filter*/
		slist_t tag_lvl_list;
		/*global filter level,tag and keyword*/
		uint32_t level;
		char tag[ULOG_FILTER_TAG_MAX_LEN + 1];
		char keyword[ULOG_FILTER_KW_MAX_LEN + 1];
	}filter;
#endif/*ULOG_USING_FILTER*/
};

/*level output info*/
static const char *const level_output_info[] =
{
	"A/",
	NULL,
	NULL,
	"E/",
	"W/",
	NULL,
	"I/",
	"D/",
};




#ifdef ULOG_USING_COLOR
static const char *const color_output_info[] =
{
		ULOG_COLOR_ASSERT,
		NULL,
		NULL,
		ULOG_COLOR_ERROR,
		ULOG_COLOR_WARN,
		NULL,
		ULOG_COLOR_INFO,
		ULOG_COLOR_DEBUG,
};
#endif/*ULOG_USING_COLOR*/


static struct ulog_t ulog = {0};


/* ulog_strcpy
 * ��cur_len��ʼ����src�����ݵ�dst������
 * cur_len:��ǰulog�����������е����ݳ���
 * dst:Ŀ�껺������ַ
 * src:Դ���ݵ�ַ
 * */
size_t ulog_strcpy(size_t cur_len,char *dst,const char *src)
{
	const char *src_old = src;
	UL_ASSERT(dst);
	UL_ASSERT(src);

	while(*src != 0)
	{
		if(cur_len++ < ULOG_LINE_BUF_SIZE)
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
 * ��1��int���͵�����ת��Ϊ�ַ���
 * s:�ַ���������
 * n:Ҫת����int����
 * return�ַ�����
 * */
size_t ulog_ultoa(char *s,unsigned long int n)
{
	size_t i = 0,j = 0, len = 0;
	char swap;
	do{
		s[len++] = n % 10 + '0';
	}while(n /= 10);
	s[len] = '\0';
	for(i = 0, j = len - 1; i < j; ++i, --j)
	{
		swap = s[i];
		s[i] = s[j];
		s[j] = swap;
	}
	return len;
}

/* output_unlock
 * ����
 * */
static void output_unlock(void)
{
	if(ulog.output_lock_enabled == FALSE)
	{
		return;
	}
	/*��Ҫ�жϵ�ǰ�Ƿ����ж���*/
	if(xTaskGetCurrentTaskHandle()!= NULL)/*��ǰ��������ִ��*/
	{
		xSemaphoreGive(ulog.output_locker);
	}

}

/* output_lock
 * ����
 * */
static void output_lock(void)
{
	if(ulog.output_lock_enabled == FALSE)
	{
		return;
	}
	if(xTaskGetCurrentTaskHandle()!= NULL)/*��ǰ��������ִ��*/
	{
		xSemaphoreTake(ulog.output_locker,portMAX_DELAY);
	}

}


/* ulog_output_lock_enable
 * ����log�������
 * */
void ulog_output_lock_enable(bool enabled)
{
	ulog.output_lock_enabled = enabled;
}

/* get_log_buf
 * ����ulog��buf������
 * */
static char *get_log_buf(void)
{
	return ulog.log_buf_th;
}

/* ulog_head_format
 * ��װlog��ͷ
 * log_buf:��ʽ���ĺ��������Ļ�����
 * level:����ȼ�
 * tag:��ǩ
 * */
size_t ulog_head_formater(char *log_buf, uint32_t level, const char *tag)
{
	static size_t log_len;
#ifdef ULOG_OUTPUT_TIME
	static size_t tick_len = 0;
#endif/*ULOG_OUTPUT_TIME*/

#if ULOG_OUTPUT_THREAD_NAME
	rt_size_t name_len = 0;
	const char *thread_name = "N/A";
#endif/*ULOG_OUTPUT_THREAD_NAME*/

	UL_ASSERT(log_buf);
	UL_ASSERT(level <= LOG_LVL_DBG);
	UL_ASSERT(tag);

	log_len = 0;
#ifdef ULOG_USING_COLOR
	if(color_output_info[level])
	{
		log_len += ulog_strcpy(log_len, log_buf + log_len, CSI_START);
		log_len += ulog_strcpy(log_len, log_buf + log_len, color_output_info[level]);
	}
#endif/*ULOG_USING_COLOR*/
	log_buf[log_len] = '\0';

	/*add timestamp */
#ifdef ULOG_OUTPUT_TIME
	log_buf[log_len] = '[';
	tick_len = ulog_ultoa(log_buf + log_len + 1,xTaskGetTickCount());
	log_buf[log_len + 1 + tick_len] = ']';
	log_buf[log_len + 1 + tick_len + 1] = '\0';
	log_len += strlen(log_buf + log_len);
#endif/*ULOG_OUTPUT_TIME*/

#ifdef ULOG_OUTPUT_LEVEL
#ifdef ULOG_OUTPUT_TIME
	log_len += ulog_strcpy(log_len, log_buf + log_len, " ");
#endif/*ULOG_OUTPUT_TIME*/

	/*add level info*/
	log_len += ulog_strcpy(log_len, log_buf + log_len, level_output_info[level]);
#endif/*ULOG_OUTPUT_LEVEL*/

#ifdef ULOG_OUTPUT_TAG
#if !defined(ULOG_OUTPUT_LEVEL) && defined(ULOG_OUTPUT_TIME)
	log_len += ulog_strcpy(log_len, log_buf + log_len, " ");
#endif /*!defined(ULOG_OUTPUT_LEVEL) && defined(ULOG_OUTPUT_TIME)*/
	/*add tag info*/
	log_len += ulog_strcpy(log_len, log_buf + log_len, tag);
#endif/*ULOG_OUTPUT_TAG*/

#ifdef ULOG_OUTPUT_THREAD_NAME
#if defined(ULOG_OUTPUT_TIME) || defined(ULOG_OUTPUT_LEVEL) || defined(ULOG_OUTPUT_TAG)
	log_len += ulog_strcpy(log_len, log_buf + log_len," ");
#endif
	/*�жϵ�ǰ����������Ƶ��Ƿ�Ϊ��*/
	if(xTaskGetCurrentTaskHandle())/*��ǰ���������������ƿ�,��ҪFreeRTOSʵ��*/
	{
		thread_name = pcTaskGetName(xTaskGetCurrentTaskHandle());
	}
	name_len = strnlen(thread_name,UL_NAME_MAX);
	strncpy(log_buf + log_len, thread_name, name_len);
	log_len += name_len;

#endif/*ULOG_OUTUT_THREAD_NAME*/

	log_len += ulog_strcpy(log_len, log_buf + log_len, ": ");
	return log_len;
}

/* ulog_tail_formater
 * ��װlog��β
 * log_buf:��ʽ���ĺ��������Ļ�����
 * log_len:��ǰlogbuf�ж�������
 * newline:�Ƿ�ʹ�û��з�
 * level:����ȼ�
 * */
size_t ulog_tail_formater(char *log_buf, size_t log_len, bool newline, uint32_t level)
{
	static size_t newline_len;
	UL_ASSERT(log_buf);
	newline_len = strlen(ULOG_NEWLINE_SIGN);
#ifdef ULOG_USING_COLOR
	if(log_len + (sizeof(CSI_END) - 1) + newline_len + sizeof((char)'\0') > ULOG_LINE_BUF_SIZE)
	{
		log_len = ULOG_LINE_BUF_SIZE;
		log_len -= (sizeof(CSI_END) - 1);

#else
	if(log_len + newline_len + sizeof((char)'\0') > ULOG_LINE_BUF_SIZE)
	{
		log_len = ULOG_LINE_BUF_SIZE;
#endif/*ULOG_USING_COLOR*/
		log_len -= newline_len;
		log_len -= sizeof((char)'\0');
	}

	if(newline)
	{
		log_len += ulog_strcpy(log_len, log_buf + log_len, ULOG_NEWLINE_SIGN);
	}

#ifdef ULOG_USING_COLOR
	if(color_output_info[level])
	{
		log_len += ulog_strcpy(log_len, log_buf + log_len, CSI_END);
	}
#endif/*ULOG_USING_COLOR*/

	log_buf[log_len] = '\0';
	return log_len;
}

/* ulog_formater
 * ��ʽ�������������
 * log_buf:��ʽ��������ݴ洢������
 * level:����ȼ�
 * tag:��ǩ
 * newline:�Ƿ�������з�
 * format:�����ʽ
 * args:�ɱ��β��б�
 * */
size_t ulog_formater(char *log_buf, uint32_t level, const char *tag, bool newline, const char *format, va_list args)
{
	/*the caller has locker,so it can use static variable for reduce stack usage*/
	static size_t log_len;
	static int fmt_result;

	UL_ASSERT(log_buf);
	UL_ASSERT(format);

	/*make log head*/
	log_len = ulog_head_formater(log_buf, level, tag);

	fmt_result = vsnprintf(log_buf + log_len, ULOG_LINE_BUF_SIZE - log_len, format, args);


	if((log_len + fmt_result <= ULOG_LINE_BUF_SIZE) && (fmt_result > -1))
	{
		log_len += fmt_result;
	}
	else
	{
		log_len = ULOG_LINE_BUF_SIZE;
	}
	return ulog_tail_formater(log_buf, log_len, newline, level);
}

/* ulog_hex_format
 * ���hex
 * log_buf:log������
 * tag:��ǩ
 * buf:hex���ݻ�����
 * size:hex���ݴ�С
 * width:1�е�������
 * addr:���ݵ�ַ
 * */
size_t ulog_hex_format(char *log_buf, const char *tag, const uint8_t *buf, size_t size, size_t width, uint32_t addr)
{
/*�ж��ַ��Ƿ������ʾ*/
#define __is_print(ch)		((unsigned int)((ch) - ' ') < 127u - ' ')
	static size_t log_len,j;
	static int fmt_result;
	char dump_string[8];

	UL_ASSERT(log_buf);
	UL_ASSERT(buf);
	/*log head*/
	log_len = ulog_head_formater(log_buf, LOG_LVL_DBG, tag);
	/*log content*/
	fmt_result = snprintf(log_buf + log_len, ULOG_LINE_BUF_SIZE, "%04x-%04x: ", (unsigned int)addr, (unsigned int)(addr + size));
	/*calculate log length*/
	if((fmt_result > -1) && (fmt_result <= ULOG_LINE_BUF_SIZE))
	{
		log_len += fmt_result;
	}
	else
	{
		log_len = ULOG_LINE_BUF_SIZE;
	}
	/*dump hex*/
	for(j = 0; j < width; j++)
	{
		if(j < size)
		{
			snprintf(dump_string, sizeof(dump_string), "%02X ", buf[j]);
		}
		else
		{
			strncpy(dump_string, "   ", sizeof(dump_string));
		}
		log_len += ulog_strcpy(log_len, log_buf + log_len, dump_string);
		if((j + 1) % 8 == 0)
		{
			log_len += ulog_strcpy(log_len, log_buf + log_len, " ");
		}
	}
	log_len += ulog_strcpy(log_len, log_buf + log_len, " ");
	/*dump char for hex*/
	for(j = 0; j < size; j++)
	{
		snprintf(dump_string, sizeof(dump_string), "%c", __is_print(buf[j]) ? buf[j] : '.');
		log_len += ulog_strcpy(log_len, log_buf + log_len, dump_string);
	}
	/*log tail*/
	return ulog_tail_formater(log_buf, log_len, TRUE, LOG_LVL_DBG);
}
extern void ukuptstr(const char *str,int len);

/* ulog_output_to_all_backend
 * ���к�����
 * level:����ȼ�
 * tag:��ǩ
 * is_raw:�Ƿ���ԭʼ����
 * log:���ݻ�����
 * len:���ݻ������е����ݳ���
 * */
static void ulog_output_to_all_backend(uint32_t level, const char *tag, bool is_raw, const char *log, size_t len)
{
	slist_t *node;
	ulog_backend_t backend;
#if !defined(ULOG_USING_COLOR) || defined(ULOG_USING_SYSLOG)
#else
	size_t color_info_len = 0,output_len = len;
	const char *output_log = log;
	size_t color_hdr_len;
#endif

	if(!ulog.init_ok)
		return;

	/*if there is no backend*/
	if(!slist_first(&ulog.backend_list))/*û�к��,printf���*/
	{
		uprintf("%s",log);

		//ukupts(log);
		//ukuptstr(log,len);
		//dprintf("%s",log);
		return;
	}

	/*output for all backends*/
	for(node = slist_first(&ulog.backend_list); node; node = slist_next(node))
	{
		backend = slist_entry(node, struct ulog_backend, list);
		if(backend->out_level < level)
		{
			continue;
		}
#if !defined(ULOG_USING_COLOR) || defined(ULOG_USING_SYSLOG)
		backend->output(backend, level, tag, is_raw, log, len);
#else
		if(backend->filter && backend->filter(backend, level, tag, is_raw, log, len) == FALSE)
		{
			continue;
		}
		if(backend->support_color || is_raw)
		{
			backend->output(backend, level, tag, is_raw, log, len);
		}
		else
		{
			if(color_output_info[level] != NULL)
			{
				color_info_len = strlen(color_output_info[level]);
			}
			if(color_info_len)
			{
				color_hdr_len = strlen(CSI_START) + color_info_len;
				output_log += color_hdr_len;
				output_len -= (color_hdr_len + (sizeof(CSI_END) - 1));
			}
			backend->output(backend, level, tag, is_raw, output_log, output_len);
		}
#endif/*!defined(ULOG_USING_COLOR) || defined(ULOG_USING_SYSLOG)*/
	}

}

/* do_output
 * ����������ط�??
 * level:����ȼ�
 * tag:��ǩ
 * is_raw:���ԭʼ����
 * log_buf:log������
 * log_len:log���������ݳ���
 * */
static void do_output(uint32_t level, const char *tag, bool is_raw, const char *log_buf, size_t log_len)
{
#ifdef ULOG_USING_ASYNC_OUTPUT
	rbb_blk_t log_blk;
	ulog_frame_t log_frame;
	static bool already_output = FALSE;
	size_t log_buf_size = log_len + sizeof((char)'\0');
	if(ulog.async_enabled)
	{
		if(is_raw == FALSE)
		{
			/* allocate log frame
			 * ����һ����
			 * */
			log_blk = rbb_blk_alloc(ulog.async_rbb, RT_ALIGN(sizeof(struct ulog_frame) + log_buf_size, RT_ALIGN_SIZE));
			if(log_blk)
			{
				/*package the log frame*/
				log_frame = (ulog_frame_t)log_blk->buf;
				log_frame->magic = ULOG_FRAME_MAGIC;
				log_frame->is_raw = is_raw;
				log_frame->level = level;
				log_frame->log_len = log_len;
				log_frame->tag = tag;
				log_frame->log = (const char *)log_blk->buf + sizeof(struct ulog_frame);
				/* copy log data
				 * ����log_buf�е�log���ݵ�������
				 * */
				strncpy((char*)(log_blk->buf + sizeof(struct ulog_frame)), log_buf, log_buf_size);
				/* put the block
				 * ���¸ÿ��״̬,�Ѿ�����������
				 * */
				rbb_blk_put(log_blk);
				/*send a notice*/
				xSemaphoreGive(ulog.async_notice);
			}
			else
			{
				if(already_output == FALSE)
				{
					uprintf("Warning: There is no enough buffer for saving async log, please increase the ULOG_ASYNC_OUTPUT_SIZE option.\n");
					already_output = TRUE;
				}
			}
		}
		else if(ulog.async_rb)/*ԭʼ���,ʹ��ringbuffer*/
		{
			/*дlogbuf�����λ�����*/
			ringbuffer_put(ulog.async_rb, (const uint8_t *)log_buf, (uint16_t)log_len);
			/*send a notice*/
			xSemaphoreGive(ulog.async_notice);/*�ͷ��ź���,��֪�첽�������*/
		}
		return;
	}
#endif/*ULOG_USING_ASYNC_OUTPUT*/
	ulog_output_to_all_backend(level, tag, is_raw, log_buf, log_len);
}


/* ulog_voutput
 * ��ʽ�����
 * level:����ȼ�
 * tag:��ǩ
 * newline:�Ƿ��������
 * hex_buf:hex��ʽ������ݵ�ַ
 * hex_size:hex���ݳ���
 * hex_width:hex���ÿ�г���
 * hex_addr:hex���ݵ�ַ
 * format:�����ʽ
 * args:�ɱ��β��б�
 * */
void ulog_voutput(uint32_t level, const char *tag, bool newline, const uint8_t *hex_buf, size_t hex_size, size_t hex_width, uint32_t hex_addr, const char *format,va_list args)
{

	static bool ulog_voutput_recursion = FALSE;
	char *log_buf = NULL;
	static size_t log_len = 0;
	UL_ASSERT(tag);
	UL_ASSERT((format && !hex_buf) || (!format && hex_buf));
	UL_ASSERT(level <= LOG_LVL_DBG);

	if(!ulog.init_ok)
	{
		return;
	}

#ifdef ULOG_USING_FILTER
	if(level > ulog.filter.level || level > ulog_tag_lvl_filter_get(tag))
	{
		return;
	}
	else if(!strstr(tag, ulog.filter.tag))
	{

		/*tag filter*/
		return;
	}
#endif/*ULOG_USING_FILTER*/

	/*get log buffer*/
	log_buf = get_log_buf();

	/*lock output*/
	output_lock();
	/*if there is a recursion,we use a simple way*/
	if((ulog_voutput_recursion == TRUE) && (hex_buf == NULL))
	{
		uprintf(format, args);
		if(newline == TRUE)
		{
			uprintf(ULOG_NEWLINE_SIGN);
		}
		output_unlock();
		return;
	}

	ulog_voutput_recursion = TRUE;

	if(hex_buf == NULL)
	{

		log_len = ulog_formater(log_buf, level, tag, newline, format, args);
	}
	else
	{
		log_len = ulog_hex_format(log_buf, tag, hex_buf, hex_size, hex_width, hex_addr);

	}

#ifdef ULOG_USING_FILTER
	/*keyword filter*/
	if(ulog.filter.keyword[0] != '\0')
	{
		/*add string end sign*/
		log_buf[log_len] = '\0';
		/*find the keyword*/
		if(!strstr(log_buf, ulog.filter.keyword))
		{
			ulog_voutput_recursion = FALSE;
			output_unlock();
			return;
		}
	}

#endif/*ULOG_USING_FILTER*/
	do_output(level, tag, FALSE, log_buf, log_len);

	ulog_voutput_recursion = FALSE;

	output_unlock();
}



/* ulog_output
 * ��ʽ�����
 * level:����ȼ�
 * tag:��ǩ
 * newline:�Ƿ��������
 * format:�����ʽ
 * */
void ulog_output(uint32_t level, const char *tag, bool newline, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	ulog_voutput(level, tag, newline, NULL, 0, 0, 0, format, args);

	va_end(args);
}

/* ulog_raw
 * ���ԭʼ����
 * format:��ʽ��
 * ...:�ɱ��β�
 * */
void ulog_raw(const char *format, ...)
{
	int i;

	size_t log_len = 0;
	char *log_buf = NULL;
	va_list args;
	int fmt_result;

	if(!ulog.init_ok)
	{
		return;
	}

#ifdef ULOG_USING_ASYNC_OUTPUT
	if(ulog.async_rb == NULL)/*�첽���,�������λ���������*/
	{
		ulog.async_rb = ringbuffer_create(ULOG_ASYNC_OUTPUT_SIZE);
	}
#endif

	/*get log buf*/
	log_buf = get_log_buf();

	/*lock output*/
	output_lock();

	/*args point to the first variable parameter*/
	va_start(args, format);
	fmt_result = vsnprintf(log_buf, ULOG_LINE_BUF_SIZE, format, args);
	va_end(args);

	/*calculate log length*/
	if((fmt_result > -1) && (fmt_result <= ULOG_LINE_BUF_SIZE))
	{
		log_len = fmt_result;
	}
	else
	{
		log_len = ULOG_LINE_BUF_SIZE;
	}

	if(log_buf[log_len] == '\0')
	{
		dprintf("ulog_raw : log line has 0\r\n");
		for(i=0;i<=log_len;i++)
		{
			dprintf("log_buf[%d] = %c,%d\r\n",i,log_buf[i],log_buf[i]);
		}
	}

	/*do log output*/
	do_output(LOG_LVL_DBG, "", TRUE, log_buf, log_len);

	/*unlock output*/
	output_unlock();
}

/* ulog_hexdump
 * ���hex����
 * tag:��ǩ
 * width:���һ�еĿ��
 * buf:hex���ݻ�������ַ
 * size:hex���ݳ���
 * */
void ulog_hexdump(const char *tag, size_t width, const uint8_t *buf, size_t size, ...)
{
	size_t i, len;
	va_list args;
	va_start(args, size);
	for(i = 0; i < size; i += width, buf += width)
	{
		if(i + width > size)
		{
			len = size - i;
		}
		else
		{
			len = width;
		}
		ulog_voutput(LOG_LVL_DBG, tag, TRUE, buf, len, width, i, NULL, args);
	}
	va_end(args);
}

#ifdef ULOG_USING_FILTER

/* ulog_be_lvl_filter_set
 * ����ָ�����ֺ�˵�����ȼ�,
 * be_name:��������
 * level:����������ȼ�
 * LOG_FILTER_LVL_SILENT:log�����,LOG_FILTER_LVL_ALL,
 * */
int ulog_be_lvl_filter_set(const char *be_name, uint32_t level)
{
	slist_t *node = NULL;
	ulog_backend_t backend;
	int result = RT_OK;

	if(level > LOG_FILTER_LVL_ALL)
	{
		return RT_FAIL;
	}

	if(!ulog.init_ok)
	{
		return result;
	}

	for(node = slist_first(&ulog.backend_list); node; node = slist_next(node))
	{
		backend = slist_entry(node, struct ulog_backend, list);
		if(strncmp(backend->name, be_name, UL_NAME_MAX) == 0)
		{
			backend->out_level = level;
		}
	}
	return result;
}


/* ulog_tag_lvl_filter_set
 * ���ñ�ǩ������ȼ�
 * tag:ָ���ı�ǩ
 * level:����ȼ�
 * */
int ulog_tag_lvl_filter_set(const char *tag, uint32_t level)
{
	slist_t *node;
	ulog_tag_lvl_filter_t tag_lvl = NULL;
	int result = RT_OK;

	if(level > LOG_FILTER_LVL_ALL)
	{
		return RT_FAIL;
	}
	if(!ulog.init_ok)
	{
		return result;
	}

	/*lock output*/
	output_lock();
	/*find the tag in list*/
	for(node = slist_first(ulog_tag_lvl_list_get()); node; node = slist_next(node))
	{
		tag_lvl = slist_entry(node, struct ulog_tag_lvl_filter, list);
		if(!strncmp(tag_lvl->tag, tag, ULOG_FILTER_TAG_MAX_LEN))
		{
			break;
		}
		else
		{
			tag_lvl = NULL;
		}
	}

	/*find ok*/
	if(tag_lvl)
	{
		if(level == LOG_FILTER_LVL_ALL)/*LOG_FILTER_LVL_ALL,ֱ�����������,�Ѹ�tag�ӹ����������Ƴ�*/
		{
			slist_remove(ulog_tag_lvl_list_get(), &tag_lvl->list);
			vPortFree(tag_lvl);
		}
		else
		{
			tag_lvl->level = level;
		}
	}
	else/*û�ҵ�,��ӵ�����������*/
	{
		/*only add the new tag's level filer when level is not LOG_FILTER_LVL_ALL*/
		if(level != LOG_FILTER_LVL_ALL)
		{
			/*new a tag's level filter*/
			tag_lvl = (ulog_tag_lvl_filter_t)pvPortMalloc(sizeof(struct ulog_tag_lvl_filter));
			if(tag_lvl)
			{
				memset(tag_lvl->tag, 0, sizeof(tag_lvl->tag));
				strncpy(tag_lvl->tag, tag, ULOG_FILTER_TAG_MAX_LEN);
				tag_lvl->level = level;
				slist_append(ulog_tag_lvl_list_get(), &tag_lvl->list);
			}
			else
			{
				result = RT_FAIL;
			}
		}
	}
	output_unlock();
	return result;
}

/* ulog_tag_lvl_filter_get
 * ��ȡָ����ǩ�Ĺ���������ȼ�
 * tag:��ǩ
 * return:����ȼ�
 * */
uint32_t ulog_tag_lvl_filter_get(const char *tag)
{
	slist_t *node;
	ulog_tag_lvl_filter_t tag_lvl = NULL;
	uint32_t level = LOG_FILTER_LVL_ALL;

	if(!ulog.init_ok)
	{
		return level;
	}

	/*lock output*/
	output_lock();
	/*find the tag in list*/
	for(node = slist_first(ulog_tag_lvl_list_get()); node; node = slist_next(node))
	{
		tag_lvl = slist_entry(node, struct ulog_tag_lvl_filter, list);
		if(!strncmp(tag_lvl->tag, tag, ULOG_FILTER_TAG_MAX_LEN))
		{
			level = tag_lvl->level;
			break;
		}
	}

	/*unlock output*/
	output_unlock();

	return level;
}

/* ulog_tag_lvl_list_get
 * ���ر�ǩ������������
 * */
slist_t *ulog_tag_lvl_list_get(void)
{
	return &ulog.filter.tag_lvl_list;
}

/* ulog_global_filter_lvl_set
 * ����ȫ�ֵĹ������ȼ�
 * level:�ȼ�
 * */
void ulog_global_filter_lvl_set(uint32_t level)
{
	UL_ASSERT(level <= LOG_FILTER_LVL_ALL);
	ulog.filter.level = level;
}

/* ulog_global_filter_lvl_get
 * ����ȫ�ֵĹ������ȼ�
 * */
uint32_t ulog_global_filter_lvl_get(void)
{
	return ulog.filter.level;
}

/* ulog_global_filter_tag_set
 * ����ȫ�ֹ�������ǩ
 * tag:��ǩ
 * */
void ulog_global_filter_tag_set(const char *tag)
{
	UL_ASSERT(tag);
	strncpy(ulog.filter.tag, tag, ULOG_FILTER_TAG_MAX_LEN);
}

/* ulog_global_filetr_tag_get
 * ����ȫ�ֹ�������ǩ
 * */
const char *ulog_global_filetr_tag_get(void)
{
	return ulog.filter.tag;
}

/* ulog_global_filter_kw_set
 * ����ȫ�ֹ��˹ؼ���
 * keyword:�ؼ���
 * */
void ulog_global_filter_kw_set(const char *keyword)
{
	UL_ASSERT(keyword);
	strncpy(ulog.filter.keyword, keyword, ULOG_FILTER_KW_MAX_LEN);
}

/* ulog_global_filter_kw_get
 * ��ȡȫ�ֹ��˹ؼ���
 * */
const char *ulog_global_filter_kw_get(void)
{
	return ulog.filter.keyword;
}

#endif/*ULOG_USING_FILTER*/


/* ulog_backend_register
 * ע��һ��������
 * backend:��˽ṹ��
 * name:�������
 * support_color:�Ƿ�֧����ɫ���
 * */
uint8_t ulog_backend_register(ulog_backend_t backend, const char *name, bool support_color)
{
	UL_ASSERT(backend);
	UL_ASSERT(name);
	UL_ASSERT(ulog.init_ok);
	UL_ASSERT(backend->output);

	if(backend->init)
	{
		backend->init(backend);
	}

	backend->support_color = support_color;
	backend->out_level = LOG_FILTER_LVL_ALL;
	strncpy(backend->name, name, UL_NAME_MAX);

	taskENTER_CRITICAL();
	slist_append(&ulog.backend_list, &backend->list);
	taskEXIT_CRITICAL();

	return RT_OK;

}

/* ulog_backend_unregister
 * ע��һ��������
 * backend:��˾��
 * */
uint8_t ulog_backend_unregister(ulog_backend_t backend)
{
	UL_ASSERT(backend);
	UL_ASSERT(ulog.init_ok);

	if(backend->deinit)
	{
		backend->deinit(backend);
	}
	taskENTER_CRITICAL();
	slist_remove(&ulog.backend_list, &backend->list);
	taskEXIT_CRITICAL();

	return RT_OK;
}

/* ulog_backend_set_filter
 * ���ú������Ĺ�����
 * backend:��˾��
 * filter:������
 * */
uint8_t ulog_backend_set_filter(ulog_backend_t backend, ulog_backend_filter_t filter)
{
	UL_ASSERT(backend);

	taskENTER_CRITICAL();
	backend->filter = filter;
	taskEXIT_CRITICAL();
	return RT_OK;
}

/* ulog_backend_find
 * ���������ҵ���˾��
 * name:�������
 * */
ulog_backend_t ulog_backend_find(const char *name)
{
	slist_t *node;
	ulog_backend_t backend;
	UL_ASSERT(ulog.init_ok);

	taskENTER_CRITICAL();
	for(node = slist_first(&ulog.backend_list); node; node = slist_next(node))
	{
		backend = slist_entry(node, struct ulog_backend, list);
		if(strncmp(backend->name, name, UL_NAME_MAX) == 0)
		{
			taskEXIT_CRITICAL();
			return backend;
		}
	}


	taskEXIT_CRITICAL();
	return NULL;
}

#ifdef ULOG_USING_ASYNC_OUTPUT

/* ulog_async_output
 * �첽���
 * */
void ulog_async_output(void)
{
	rbb_blk_t log_blk;
	ulog_frame_t log_frame;
	size_t log_len;
	char *log;
	size_t len;
	if(!ulog.async_enabled)
	{
		return;
	}
	/* ��rbb->blk_list�����Ѿ�PUT�Ŀ�
	 * */
	while((log_blk = rbb_blk_get(ulog.async_rbb)) != NULL)
	{
		log_frame = (ulog_frame_t)log_blk->buf;
		if(log_frame->magic == ULOG_FRAME_MAGIC)/*�ж�ͷ*/
		{
			/*output to all backends*/
			ulog_output_to_all_backend(log_frame->level, log_frame->tag, log_frame->is_raw, log_frame->log, log_frame->log_len);
		}
		/*�ͷſ�*/
		rbb_blk_free(ulog.async_rbb, log_blk);
	}
	/* ԭʼ�������
	 * ringbuffer
	 * */
	if(ulog.async_rb)
	{
		/*��ȡringbuffer��Ч���ݸ���*/
		log_len = ringbuffer_data_len(ulog.async_rb);
		log = pvPortMalloc(log_len + 1);/*����ռ�*/
		if(log)
		{
			/*��ringbuffer�ж����ݵ�log������*/
			len = ringbuffer_get(ulog.async_rb, (uint8_t*)log, (uint16_t)log_len);
			log[log_len] = '\0';
			ulog_output_to_all_backend(LOG_LVL_DBG, "", TRUE, log, len);/*���*/
			vPortFree(log);
		}
	}
}

/* ulog_async_output_enabled
 * �첽���ʹ��
 * enabled:�Ƿ�ʹ���첽���
 * */
void ulog_async_output_enabled(bool enabled)
{
	ulog.async_enabled = enabled;
}

/* ulog_async_waiting_log
 * �ȴ���log��Ҫ���
 * */
uint8_t ulog_async_waiting_log(uint32_t time)
{
	BaseType_t nRet;

	//rt_sem_control(&ulog.async_notice, RT_IPC_CMD_RESET, RT_NULL);
	nRet = xSemaphoreTake(ulog.async_notice,time);
	if(pdPASS != nRet)/*errQUEUE_EMPTY*/
	{
		return RT_FAIL;
	}
	return RT_OK;
}

static void async_output_thread_entry(void *param)
{
	ulog_async_output();
	for(;;)
	{
		ulog_async_waiting_log(portMAX_DELAY);
		for(;;)
		{
			ulog_async_output();
			/* if there is no log output for a certain period of time,
			 * refresh the log buffer
			 * */
			if(ulog_async_waiting_log(pdMS_TO_TICKS(2000)) == RT_OK)
			{
				continue;
			}
			else
			{
				ulog_flush();
				break;
			}
		}
	}
}

#endif/*ULOG_USING_ASYNC_OUTPUT*/

/* ulog_flush
 * flush���к�˵�log
 * */
void ulog_flush(void)
{
	slist_t *node;
	ulog_backend_t backend;
	if(!ulog.init_ok)
	{
		return;
	}
#ifdef ULOG_USING_ASYNC_OUTPUT
	ulog_async_output();
#endif/*ULOG_USING_ASYNC_OUTPUT*/

	/*flush all backends*/
	for(node = slist_first(&ulog.backend_list); node; node = slist_next(node))
	{
		backend = slist_entry(node, struct ulog_backend, list);
		if(backend->flush)
		{
			backend->flush(backend);
		}
	}

}


/* ulog_init
 * ulog ��ʼ��
 * */
int ulog_init(void)
{
	if(ulog.init_ok)
		return RT_OK;
	/*��ʼ��������*/
	ulog.output_locker = xSemaphoreCreateMutex();
	if(NULL == ulog.output_locker)
	{
		return RT_FAIL;/*����������ʧ��*/
	}
	ulog.output_lock_enabled = TRUE;
	slist_init(&ulog.backend_list);

#ifdef ULOG_USING_FILTER
	slist_init(ulog_tag_lvl_list_get());
#endif/*ULOG_USING_FILTER*/

#ifdef ULOG_USING_ASYNC_OUTPUT
	UL_ASSERT(ULOG_ASYNC_OUTPUT_STORE_LINES>=2);
	ulog.async_enabled = TRUE;
	/*async output ring block buffer*/
	ulog.async_rbb = rbb_create(RT_ALIGN(ULOG_ASYNC_OUTPUT_BUF_SIZE, RT_ALIGN_SIZE), ULOG_ASYNC_OUTPUT_STORE_LINES);
	if(ulog.async_rbb == NULL)
	{
		uprintf("Error:ulog init failed! no memory for async rbb.\n");
		vSemaphoreDelete(ulog.output_locker);
		return RT_FAIL;
	}
	ulog.async_notice = xSemaphoreCreateBinary();
	if(ulog.async_notice == NULL)
	{
		vSemaphoreDelete(ulog.output_locker);
		return RT_FAIL;
	}
	ulog_async_init();
#endif/*ULOG_USING_ASYNC_OUTPUT*/

#ifdef ULOG_USING_FILTER
	ulog_global_filter_lvl_set(LOG_FILTER_LVL_ALL);
#endif/*ULOG_USING_FILTER*/
	ulog.init_ok = TRUE;
	return RT_OK;
}


#ifdef ULOG_USING_ASYNC_OUTPUT
/* ulog_async_init
 * ulog�첽�����ʼ��,�����첽�������
 * */
int ulog_async_init(void)
{
	if(ulog.async_th == NULL)
	{
		/*async output thread*/
		if(pdPASS != xTaskCreate(async_output_thread_entry, "ulog_async", 512, NULL, 1, &ulog.async_th))
		{
			uprintf("Error: ulog init failed! No memory for asycn output thread.\n");
			return RT_FAIL;
		}
	}
	return RT_OK;
}

#endif/*ULOG_USING_ASYNC_OUTPUT*/


/* ulog_deinit
 * ulogʧ��
 * */
void ulog_deinit(void)
{
	slist_t *node;
	ulog_backend_t backend;
#ifdef ULOG_USING_FILTER
	ulog_tag_lvl_filter_t tag_lvl;
#endif/*ULOG_USING_FILTER*/

	if(!ulog.init_ok)
	{
		return;
	}
	/*deinit all backends*/
	for(node = slist_first(&ulog.backend_list); node; node = slist_next(node))
	{
		backend = slist_entry(node, struct ulog_backend, list);
		if(backend->deinit)
		{
			backend->deinit(backend);
		}
	}

#ifdef ULOG_USING_FILTER
	/*deinit tag's level filter*/
	for(node = slist_first(ulog_tag_lvl_list_get()); node; node = slist_next(node))
	{
		tag_lvl = slist_entry(node, struct ulog_tag_lvl_filter, list);
		vPortFree(tag_lvl);
	}

#endif/*ULOG_USING_FILTER*/


	/*���ٻ�����*/
	vSemaphoreDelete(ulog.output_locker);

#ifdef ULOG_USING_ASYNC_OUTPUT
	rbb_destroy(ulog.async_rbb);
	if(ulog.async_th != NULL)/*��ֹɾ����ǰ����*/
	{
		vTaskDelete(ulog.async_th);
	}
	if(ulog.async_rb)
	{
		ringbuffer_destroy(ulog.async_rb);
	}

#endif

	ulog.init_ok = FALSE;

}





