/*
 * File      : usart.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 * 2010-03-29     Bernard      remove interrupt Tx and DMA Rx mode
 * 2012-02-08     aozima       update for F4.
 */

#include "stm32f4xx_conf.h"
#include "usart.h"
#include "board.h"
#include <serial.h>

/*
 * Use UART1 as console output and finsh input
 * interrupt Rx and poll Tx (stream mode)
 *
 * Use UART2 with interrupt Rx and poll Tx
 * Use UART3 with DMA Tx and interrupt Rx -- DMA channel 2
 *
 * USART DMA setting on STM32
 * USART1 Tx --> DMA Channel 4
 * USART1 Rx --> DMA Channel 5
 * USART2 Tx --> DMA Channel 7
 * USART2 Rx --> DMA Channel 6
 * USART3 Tx --> DMA Channel 2
 * USART3 Rx --> DMA Channel 3
 */

#ifdef RT_USING_UART1
struct stm32_serial_int_rx uart1_int_rx;
struct stm32_serial_device uart1 =
{
USART1, &uart1_int_rx,
RT_NULL };
struct rt_device uart1_device;
#endif

#ifdef RT_USING_UART2
struct stm32_serial_int_rx uart2_int_rx;
struct stm32_serial_device uart2 =
{
	USART2,
	&uart2_int_rx,
	RT_NULL
};
struct rt_device uart2_device;
#endif

#ifdef RT_USING_UART3
struct stm32_serial_int_rx uart3_int_rx;
struct stm32_serial_dma_tx uart3_dma_tx;
struct stm32_serial_device uart3 =
{
USART3, &uart3_int_rx, &uart3_dma_tx };
struct rt_device uart3_device;
#endif

#ifdef RT_USING_UART4
struct stm32_serial_int_rx uart4_int_rx;
struct stm32_serial_device uart4 =
{
UART4, &uart4_int_rx,
RT_NULL };
struct rt_device uart4_device;
#endif

#ifdef RT_USING_UART6
struct stm32_serial_int_rx uart6_int_rx;
struct stm32_serial_dma_tx uart6_dma_tx;
struct stm32_serial_device uart6 =
{
USART6, &uart6_int_rx, RT_NULL
};
struct rt_device uart6_device;
#endif

//#define USART1_DR_Base  0x40013804
//#define USART2_DR_Base  0x40004404
//#define USART3_DR_Base  0x40004804
//#define USART6_DR_Base  0x40011404

/* USART1_REMAP = 0 */
#define UART1_GPIO_TX		GPIO_Pin_9
#define UART1_TX_PIN_SOURCE GPIO_PinSource9
#define UART1_GPIO_RX		GPIO_Pin_10
#define UART1_RX_PIN_SOURCE GPIO_PinSource10
#define UART1_GPIO			GPIOA
#define UART1_GPIO_RCC      RCC_AHB1Periph_GPIOA
#define RCC_APBPeriph_UART1	RCC_APB2Periph_USART1
#define UART1_TX_DMA		DMA1_Channel4
#define UART1_RX_DMA		DMA1_Channel5

#define UART2_GPIO_TX	    GPIO_Pin_2
#define UART2_TX_PIN_SOURCE GPIO_PinSource2
#define UART2_GPIO_RX	    GPIO_Pin_3
#define UART2_RX_PIN_SOURCE GPIO_PinSource3
#define UART2_GPIO	    	GPIOA
#define UART2_GPIO_RCC   	RCC_AHB1Periph_GPIOA
#define RCC_APBPeriph_UART2	RCC_APB1Periph_USART2

/* USART3_REMAP[1:0] = 00 */
#define UART3_GPIO_TX		GPIO_Pin_10
#define UART3_TX_PIN_SOURCE GPIO_PinSource10
#define UART3_GPIO_RX		GPIO_Pin_11
#define UART3_RX_PIN_SOURCE GPIO_PinSource11
#define UART3_GPIO			GPIOB
#define UART3_GPIO_RCC   	RCC_AHB1Periph_GPIOB
#define RCC_APBPeriph_UART3	RCC_APB1Periph_USART3
#define UART3_TX_DMA		DMA1_Stream3
#define UART3_RX_DMA		DMA1_Stream1

#define UART4_GPIO_TX		GPIO_Pin_10
#define UART4_TX_PIN_SOURCE GPIO_PinSource10
#define UART4_GPIO_RX		GPIO_Pin_11
#define UART4_RX_PIN_SOURCE GPIO_PinSource11
#define UART4_GPIO			GPIOC
#define UART4_GPIO_RCC   	RCC_AHB1Periph_GPIOC
#define RCC_APBPeriph_UART4	RCC_APB1Periph_UART4
#define UART4_TX_DMA		DMA1_Stream3
#define UART4_RX_DMA		DMA1_Stream1

#define UART6_GPIO_TX	    GPIO_Pin_6
#define UART6_TX_PIN_SOURCE GPIO_PinSource6
#define UART6_GPIO_RX	    GPIO_Pin_7
#define UART6_RX_PIN_SOURCE GPIO_PinSource7
#define UART6_GPIO	    	GPIOC
#define UART6_GPIO_RCC   	RCC_AHB1Periph_GPIOC
#define RCC_APBPeriph_UART6	RCC_APB2Periph_USART6
#define UART6_RX_DMA		DMA2_Stream1
#define UART6_TX_DMA		DMA2_Stream6

void RS485_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	//TODO 485的方向控制引脚没有初始化
	//ARM1_RS485   EN
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	//RS485Trans   EN
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
//	ARM1_485_1_EN(0); TODO 请使能485方向控制
//	ARM1_485_2_EN(0);
}

static void RCC_Configuration(void)
{
#ifdef RT_USING_UART1
	/* Enable USART2 GPIO clocks */
	RCC_AHB1PeriphClockCmd(UART1_GPIO_RCC, ENABLE);
	/* Enable USART2 clock */
	RCC_APB2PeriphClockCmd(RCC_APBPeriph_UART1, ENABLE);
#endif

#ifdef RT_USING_UART2
	/* Enable USART2 GPIO clocks */
	RCC_AHB1PeriphClockCmd(UART2_GPIO_RCC, ENABLE);
	/* Enable USART2 clock */
	RCC_APB1PeriphClockCmd(RCC_APBPeriph_UART2, ENABLE);
#endif

#ifdef RT_USING_UART3
	/* Enable USART3 GPIO clocks */
	RCC_AHB1PeriphClockCmd(UART3_GPIO_RCC, ENABLE);
	/* Enable USART3 clock */
	RCC_APB1PeriphClockCmd(RCC_APBPeriph_UART3, ENABLE);

	/* DMA clock enable */
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
#endif

#ifdef RT_USING_UART4
	/* Enable USART3 GPIO clocks */
	RCC_AHB1PeriphClockCmd(UART4_GPIO_RCC, ENABLE);
	/* Enable USART3 clock */
	RCC_APB1PeriphClockCmd(RCC_APBPeriph_UART4, ENABLE);
#endif

#ifdef RT_USING_UART6
	/* Enable USART6 GPIO clocks */
	RCC_AHB1PeriphClockCmd(UART6_GPIO_RCC, ENABLE);
	/* Enable USART6 clock */
	RCC_APB2PeriphClockCmd(RCC_APBPeriph_UART6, ENABLE);
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

#endif
}

static void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

#ifdef RT_USING_UART1
	/* Configure USART1 Rx/tx PIN */
	GPIO_InitStructure.GPIO_Pin = UART1_GPIO_RX | UART1_GPIO_TX;
	GPIO_Init(UART1_GPIO, &GPIO_InitStructure);

	/* Connect alternate function */
	GPIO_PinAFConfig(UART1_GPIO, UART1_TX_PIN_SOURCE, GPIO_AF_USART1);
	GPIO_PinAFConfig(UART1_GPIO, UART1_RX_PIN_SOURCE, GPIO_AF_USART1);
#endif

#ifdef RT_USING_UART2
	/* Configure USART2 Rx/tx PIN */
	GPIO_InitStructure.GPIO_Pin = UART2_GPIO_TX | UART2_GPIO_RX;
	GPIO_Init(UART2_GPIO, &GPIO_InitStructure);

	/* Connect alternate function */
	GPIO_PinAFConfig(UART2_GPIO, UART2_TX_PIN_SOURCE, GPIO_AF_USART2);
	GPIO_PinAFConfig(UART2_GPIO, UART2_RX_PIN_SOURCE, GPIO_AF_USART2);
#endif

#ifdef RT_USING_UART3
	/* Configure USART3 Rx/tx PIN */
	GPIO_InitStructure.GPIO_Pin = UART3_GPIO_RX | UART3_GPIO_TX;
	GPIO_Init(UART3_GPIO, &GPIO_InitStructure);

	/* Connect alternate function */
	GPIO_PinAFConfig(UART3_GPIO, UART3_TX_PIN_SOURCE, GPIO_AF_USART3);
	GPIO_PinAFConfig(UART3_GPIO, UART3_RX_PIN_SOURCE, GPIO_AF_USART3);
#endif

#ifdef RT_USING_UART4
	/* Configure USART3 Rx/tx PIN */
	GPIO_InitStructure.GPIO_Pin = UART4_GPIO_RX | UART4_GPIO_TX;
	GPIO_Init(UART4_GPIO, &GPIO_InitStructure);

	/* Connect alternate function */
	GPIO_PinAFConfig(UART4_GPIO, UART4_TX_PIN_SOURCE, GPIO_AF_UART4);
	GPIO_PinAFConfig(UART4_GPIO, UART4_RX_PIN_SOURCE, GPIO_AF_UART4);
#endif

#ifdef RT_USING_UART6
	/* Configure USART6 Rx/tx PIN */
	GPIO_InitStructure.GPIO_Pin = UART6_GPIO_TX | UART6_GPIO_RX;
	GPIO_Init(UART6_GPIO, &GPIO_InitStructure);

	/* Connect alternate function */
	GPIO_PinAFConfig(UART6_GPIO, UART6_TX_PIN_SOURCE, GPIO_AF_USART6);
	GPIO_PinAFConfig(UART6_GPIO, UART6_RX_PIN_SOURCE, GPIO_AF_USART6);
#endif
}

static void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

#ifdef RT_USING_UART1
	/* Enable the USART1 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif

#ifdef RT_USING_UART2
	/* Enable the USART2 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif

#ifdef RT_USING_UART3
	/* Enable the USART3 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

//	/* Enable the DMA1 Channel2 Interrupt */
//	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream3_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
#endif

#ifdef RT_USING_UART4
	/* Enable the USART2 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif

#ifdef RT_USING_UART6
	/* Enable the USART6 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

//	/* Enable the DMA1 Channel2 Interrupt */
//	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream6_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
#endif
}

static void DMA_Configuration(void)
{
	DMA_InitTypeDef DMA_InitStructure;
#if defined (RT_USING_UART3)
	/* Configure DMA Stream */
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) (&USART3->DR);
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) 0;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_BufferSize = (uint32_t) 1;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

	DMA_DeInit(UART3_TX_DMA);
	DMA_Init(UART3_TX_DMA, &DMA_InitStructure);

	DMA_ITConfig(UART3_TX_DMA, DMA_IT_TC | DMA_IT_TE, ENABLE);
#endif
#if defined (RT_USING_UART6)
	/* Configure DMA Stream */
	DMA_InitStructure.DMA_Channel = DMA_Channel_5;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) (&USART6->DR);
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) 0;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_BufferSize = (uint32_t) 1;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

	DMA_DeInit(UART6_TX_DMA);
	DMA_Init(UART6_TX_DMA, &DMA_InitStructure);

	DMA_ITConfig(UART6_TX_DMA, DMA_IT_TC | DMA_IT_TE, ENABLE);
#endif
}

/*
 * Init all related hardware in here
 * rt_hw_serial_init() will register all supported USART device
 */
void rt_hw_usart_init()
{
	USART_InitTypeDef USART_InitStructure;

//	RS485_Init();
	RCC_Configuration();
	GPIO_Configuration();
	NVIC_Configuration();
//	DMA_Configuration();

	/* uart init */
#ifdef RT_USING_UART1
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	/* register uart1 */
	rt_hw_serial_register(&uart1_device, "uart1",
	RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
			&uart1);

	/* enable interrupt */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
#endif

#ifdef RT_USING_UART2
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	/* register uart2 */
	rt_hw_serial_register(&uart2_device, "uart2",
			RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
			&uart2);

	/* Enable USART2 DMA Rx request */
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
#endif

#ifdef RT_USING_UART3
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);

//	uart3_dma_tx.dma_channel = UART3_TX_DMA;
//
//	/* register uart3 */
//	rt_hw_serial_register(&uart3_device, "uart3",
//	RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_TX,
//			&uart3);
		rt_hw_serial_register(&uart3_device, "uart3",
		RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
				&uart3);

	/* Enable USART3 DMA Tx request */
//	USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);

	/* enable interrupt */
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
#endif

#ifdef RT_USING_UART4
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(UART4, &USART_InitStructure);

	/* register uart2 */
	rt_hw_serial_register(&uart4_device, "uart4",
	RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX, &uart4);

	/* Enable USART2 DMA Rx request */
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
#endif

#ifdef RT_USING_UART6
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART6, &USART_InitStructure);

	rt_hw_serial_register(&uart6_device, "uart6",
	RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
			&uart6);
//	uart6.dma_tx->dma_channel = UART6_TX_DMA;
//	/* register uart6 */
//	rt_hw_serial_register(&uart6_device, "uart6",
//	RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_TX,
//			&uart6);
//
//	/* Enable USART6 DMA Tx request */
//	USART_DMACmd(USART6, USART_DMAReq_Tx, ENABLE);

	/* Enable USART6 DMA Rx request */
	USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);
#endif
}

