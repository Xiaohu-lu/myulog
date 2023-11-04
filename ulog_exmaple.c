/*
 * ulog_exmaple.c
 *
 *  Created on: Sep 20, 2023
 *      Author: hxd
 */
#include "Drivers.h"
#define LOG_TAG		"example"
#define LOG_LVL		LOG_LVL_DBG
#include "ulog.h"

uint8_t gBuf[128];
int gcount = 0;
void Ulog_Example(void *paramter)
{
	vTaskDelay(2000);
	uint32_t count = 0;
	int i;
	for(i=0;i<sizeof(gBuf);i++)
	{
		gBuf[i] = i;
	}
	ulog_init();
	//ulog_async_init();
	for(;;)
	{
		//uprintf("count = %d\n", count);
		LOG_D("LOG_D(%d): RT-Thread is an open source ", count);
		//ulog_output(LOG_LVL, LOG_TAG, TRUE, "LOG_D(%d): RT-Thread is an open source %d", gcount,count);
		count++;
		gcount++;
		if(gcount == 5)
		{
			ulog_tag_lvl_filter_set("example", LOG_LVL_ERROR);
			//ulog_global_filter_kw_set("example");
		}
		if(gcount == 10)
		{
			ulog_tag_lvl_filter_set("posturetask", LOG_LVL_ERROR);
		}
		LOG_D("LOG_DD(%d): RT-Thread is an open source IoT operating system from China.", count);
		vTaskDelay(2000);
		LOG_HEX("gBuf",16,gBuf,sizeof(gBuf));
		vTaskDelay(2000);
	}
}

void ulog_test(void *param)
{
	int count = 0;
	for(;;)
	{
		count++;
		LOG_RAW(" raw test\r\n");
		//vTaskDelay(100);
		LOG_RAW("LOG_D(%d) ulog_test count\r\n",count);
		//vTaskDelay(100);
		LOG_RAW("A B C D E F G \r\n");
		//vTaskDelay(100);
		LOG_RAW("H I G K L M N O\r\n");
		//vTaskDelay(100);
		LOG_RAW("P Q R S T U V W X\r\n");
		//vTaskDelay(100);
		LOG_RAW("Y Z a b c d e f g h\r\n");
		vTaskDelay(100);
	}
}




void Ulog_Test(void)
{
	taskENTER_CRITICAL();
	if(pdPASS == xTaskCreate(Ulog_Example, "ulog", 512, NULL, 1, NULL))
	{
		dprintf("ULOG example Task Creat SUCCESS!\r\n");
	}
	if(pdPASS == xTaskCreate(ulog_test, "ulogtest", 512, NULL, 1, NULL))
	{
		dprintf("ULOG example Task Creat SUCCESS!\r\n");
	}
	taskEXIT_CRITICAL();
}
