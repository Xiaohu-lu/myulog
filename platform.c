#include "platform.h"

/* platform_interrupt_get_nest
 * 获取当前中断嵌套层数
 * return：	中断嵌套层数
 */
unsigned char platform_interrupt_get_nest(void)
{
	return 0;
}

/* platform_thread_self
 * 返回当前运行线程的句柄
 * return：	当前运行线程的句柄
 */
platform_thread_t platform_thread_self(void)
{
	return prvGetTCBFromHandle(NULL);
}

/* platform_Mutex_Take
 * 获取互斥量
 * mutex：	互斥量句柄
 * time：	等待时间
 * return：	成功返回0,失败返回1
 */
int platform_Mutex_Take(platform_mutex_t mutex, unsigned int time)
{
	if(xSemaphoreTake(mutex->mutex,time) == 1)
	{
		return PLAT_RT_OK;
	}
	return PLAT_RT_ERROR;
	//return xSemaphoreTake(mutex->mutex,time);
}

/* platform_Mutex_Take
 * 获取互斥量
 * mutex：	互斥量句柄
 * time：	等待时间
 * return：	成功返回0,失败返回1
 */
int platform_Mutex_Give(platform_mutex_t mutex, unsigned int time)
{
	if(xSemaphoreGive(mutex->mutex,time) == 1)
	{
		return PLAT_RT_OK;
	}
	return PLAT_RT_ERROR;
	//return xSemaphoreTake(mutex->mutex,time);
}


/* platform_interrupt_disable
 * 关闭中断函数,RTThread可以在中断里面使用,FreeRTOS使用不能在中断使用的关中断函数
 */
int platform_interrupt_disable(void)
{
#if 0
	portDISABLE_INTERRUPTS();
	return 0;
#else
	return portSET_INTERRUPT_MASK_FROM_ISR();
#endif
}

/* platform_interrupt_enable
 * 开中断函数,RTThread可以在中断里面使用,FreeRTOS使用不能在中断使用的开中断函数
 */
void platform_interrupt_enable(int level)
{
#if 0
	portENABLE_INTERRUPTS();
#else
	portCLEAR_INTERRUPT_MASK_FROM_ISR(level);
#endif
}


