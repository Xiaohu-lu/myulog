/*
 * ulog.h
 *
 *  Created on: Sep 19, 2023
 *      Author: hxd
 */

#ifndef ULOG_H_
#define ULOG_H_
#include "ulog_def.h"
#include <stdarg.h>




size_t ulog_strcpy(size_t cur_len,char *dst,const char *src);
size_t ulog_ultoa(char *s,unsigned long int n);
void ulog_output_lock_enable(bool enabled);
size_t ulog_head_formater(char *log_buf, uint32_t level, const char *tag);
size_t ulog_tail_formater(char *log_buf, size_t log_len, bool newline, uint32_t level);
size_t ulog_formater(char *log_buf, uint32_t level, const char *tag, bool newline, const char *format, va_list args);
size_t ulog_hex_format(char *log_buf, const char *tag, const uint8_t *buf, size_t size, size_t width, uint32_t addr);
void ulog_voutput(uint32_t level, const char *tag, bool newline, const uint8_t *hex_buf, size_t hex_size, size_t hex_width, uint32_t hex_addr, const char *format,va_list args);
void ulog_output(uint32_t level, const char *tag, bool newline, const char *format, ...);
void ulog_raw(const char *format, ...);
void ulog_hexdump(const char *tag, size_t width, const uint8_t *buf, size_t size, ...);


int ulog_be_lvl_filter_set(const char *be_name, uint32_t level);
int ulog_tag_lvl_filter_set(const char *tag, uint32_t level);
uint32_t ulog_tag_lvl_filter_get(const char *tag);
slist_t *ulog_tag_lvl_list_get(void);
void ulog_global_filter_lvl_set(uint32_t level);
uint32_t ulog_global_filter_lvl_get(void);
void ulog_global_filter_tag_set(const char *tag);
const char *ulog_global_filetr_tag_get(void);
void ulog_global_filter_kw_set(const char *keyword);
const char *ulog_global_filter_kw_get(void);
uint8_t ulog_backend_register(ulog_backend_t backend, const char *name, bool support_color);
uint8_t ulog_backend_unregister(ulog_backend_t backend);
uint8_t ulog_backend_set_filter(ulog_backend_t backend, ulog_backend_filter_t filter);
ulog_backend_t ulog_backend_find(const char *name);

void ulog_async_output(void);
void ulog_async_output_enabled(bool enabled);
uint8_t ulog_async_waiting_log(uint32_t time);

void ulog_flush(void);
int ulog_init(void);
int ulog_async_init(void);
void ulog_deinit(void);



#define LOG_E(...)							ulog_e(LOG_TAG,__VA_ARGS__)
#define LOG_W(...)							ulog_w(LOG_TAG,__VA_ARGS__)
#define LOG_I(...)							ulog_i(LOG_TAG,__VA_ARGS__)
#define LOG_D(...)							ulog_d(LOG_TAG,__VA_ARGS__)
#define LOG_RAW(...)						ulog_raw(__VA_ARGS__)
#define LOG_HEX(name, width, buf, size)		ulog_hex(name, width, buf, size)


#endif /* ULOG_H_ */
