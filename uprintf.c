/*
 * uprintf.c
 *
 *  Created on: Sep 19, 2023
 *      Author: hxd
 */
#include "Drivers.h"
#include "Uart.h"
#include <stdarg.h>
#define BUFFERSIZE	128
char uprintbuff[BUFFERSIZE];
uint8_t RxArray[10];
uint8_t uprint_init = 0;
void uprintf_init(void)
{
    STRUCT_UART_FORMAT sFrame;
    STRUCT_DATA_BUF RxBuf;
    STRUCT_UART_PIN UART_Port;


    sFrame.BaudRate = 115200;        // baud rate
    sFrame.CharBit = CHAR_8BIT;     // character bit
    sFrame.StopBit = ONE_STOP;      // stop bit
    sFrame.Parity = NONE;           // parity

    RxBuf.pBuf = RxArray;
    RxBuf.Size = sizeof(RxArray);

    UART_Port.TxdPin = UART0_TXD_GPA2,
    UART_Port.RxdPin = UART0_RXD_GPA3;
    UART_Port.RtsPin = UART_NO_PIN;
    UART_Port.CtsPin = UART_NO_PIN;

    UART_Init(UART0, UART_CLK_SRC_APBCLK, UART_Port, sFrame, &RxBuf, NULL);
}

void uprintf(const char* fmt, ...)
{
    va_list args;
    uint32_t len;
    //debug_print(fmt);
    if (0 == uprint_init)
    {
        uprintf_init();
    }

    va_start(args, fmt);
    len = vsnprintf(uprintbuff, sizeof(uprintbuff) - 1, fmt, args);
    va_end(args);
    if(len > BUFFERSIZE - 1)
    {
    	len = BUFFERSIZE - 1;
    }
    UART_SendBuf(UART0, uprintbuff, len);
}

void ukupts(const char *str)
{
	if (0 == uprint_init)
	{
		uprintf_init();
	}
	if(!str)
		return;
	//dprintf("%s",str);
	GpioRegs.GPAUSE0.bit.GPA2 = MUX_UART;/*modify 230809,接收到UART命令按键不能使用了*/
	UART_SendBuf(UART0,str,strlen(str));
}

void ukuptstr(const char *str,int len)
{
	if (0 == uprint_init)
	{
		uprintf_init();
	}
	if(!str)
		return;
	//dprintf("%s",str);
	GpioRegs.GPAUSE0.bit.GPA2 = MUX_UART;/*modify 230809,接收到UART命令按键不能使用了*/
	UART_SendBuf(UART0,str,len);
}
