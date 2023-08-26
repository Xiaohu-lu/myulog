#ifndef __PLATFORM_H
#define __PLATFORM_H

/* return code */
#define PLAT_RT_OK					0
#define PLAT_RT_ERROR				1

/* delay max */
#define platform_MAX_DELAY			0xFFFFFFFF

/* task name max len */
#define paltform_NAME_MAX			configMAX_TASK_NAME_LEN

typedef struct platform_mutex
{
	SemaphoreHandle_t	mutex;
}platform_mutex_t;

typedef struct platform_thread
{
	TaskHandle_t		thread;
}platform_thread_t;

unsigned char platform_interrupt_get_nest(void);
platform_thread_t platform_thread_self(void);
int platform_Mutex_Take(platform_mutex_t mutex, unsigned int time);
int platform_Mutex_Give(platform_mutex_t mutex, unsigned int time);
int platform_interrupt_disable(void);
void platform_interrupt_enable(int level);






#endif



