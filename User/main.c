/*
*********************************************************************************************************
*
*	模块名称 : 主程序模块。
*	文件名称 : main.c
*	版    本 : V1.0
*	说    明 : 本实验主要学习FreeRTOS的移植
*              实验目的：
*                 1. 学习FreeRTOS的移植。
*              实验内容：
*                 1. 创建了如下四个任务：
*                    vTaskTaskUserIF 任务: 接口消息处理，这里用作LED闪烁	
*                    vTaskLED        任务: LED闪烁
*                    vTaskMsgPro     任务: 信息处理，这里是用作LED闪烁
*                    vTaskStart      任务: 启动任务，也就是最高优先级任务，这里用作LED闪烁
*             1. K2长按下，挂起vTaskLED；
*							2. K3键按下，启动单次定时器中断，50ms后在定时器中断将任务vTaskLED恢复。
*              注意事项：
*              注意事项：
*                 1. 本实验推荐使用串口软件SecureCRT，要不串口打印效果不整齐。此软件在
*                    V4开发板光盘里面有。
*                 2. 务必将编辑器的缩进参数和TAB设置为4来阅读本文件，要不代码显示不整齐。
*
*	修改记录 :
*		版本号    日期         作者            说明
*       V1.0   2017          Eric    1. ST固件库到V3.6.1版本
*                                        2. BSP驱动包V1.2
*                                        3. FreeRTOS版本V8.2.3
*
*	Copyright (C), 2016-2020,BayNexus 
*
*********************************************************************************************************
*/
#include "includes.h"



/*
**********************************************************************************************************
											函数声明
**********************************************************************************************************
*/
static void vTaskTaskUserIF(void *pvParameters);
static void vTaskLED(void *pvParameters);
static void vTaskMsgPro(void *pvParameters);
static void vTaskStart(void *pvParameters);
static void AppTaskCreate (void);
static void TIM_CallBack1(void);
/*
**********************************************************************************************************
											变量声明
**********************************************************************************************************
*/
static TaskHandle_t xHandleTaskUserIF = NULL;
static TaskHandle_t xHandleTaskLED = NULL;
static TaskHandle_t xHandleTaskMsgPro = NULL;
static TaskHandle_t xHandleTaskStart = NULL;

/*
*********************************************************************************************************
*	函 数 名: main
*	功能说明: 标准c程序入口。
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
int main(void)
{
	/* 
	  在启动调度前，为了防止初始化STM32外设时有中断服务程序执行，这里禁止全局中断(除了NMI和HardFault)。
	  这样做的好处是：
	  1. 防止执行的中断服务程序中有FreeRTOS的API函数。
	  2. 保证系统正常启动，不受别的中断影响。
	  3. 关于是否关闭全局中断，大家根据自己的实际情况设置即可。
	  在移植文件port.c中的函数prvStartFirstTask中会重新开启全局中断。通过指令cpsie i开启，__set_PRIMASK(1)
	  和cpsie i是等效的。
     */
	__set_PRIMASK(1);  
	
	/* 硬件初始化 */
	bsp_Init(); 
	
	/* 1. 初始化一个定时器中断，精度高于滴答定时器中断，这样才可以获得准确的系统信息 仅供调试目的，实际项
		  目中不要使用，因为这个功能比较影响系统实时性。
	   2. 为了正确获取FreeRTOS的调试信息，可以考虑将上面的关闭中断指令__set_PRIMASK(1); 注释掉。 
	*/
	vSetupSysInfoTest();
	/* 创建任务 */
	AppTaskCreate();
	
    /* 启动调度，开始执行任务 */
    vTaskStartScheduler();

	/* 
	  如果系统正常启动是不会运行到这里的，运行到这里极有可能是用于定时器任务或者空闲任务的
	  heap空间不足造成创建失败，此要加大FreeRTOSConfig.h文件中定义的heap大小：
	  #define configTOTAL_HEAP_SIZE	      ( ( size_t ) ( 17 * 1024 ) )
	*/
	while(1);
}

/*
*********************************************************************************************************
*	函 数 名: vTaskTaskUserIF
*	功能说明: 接口消息处理，这里用作LED闪烁	
*	形    参: pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
*   优 先 级: 1  (数值越小优先级越低，这个跟uCOS相反)
*********************************************************************************************************
*/
static void vTaskTaskUserIF(void *pvParameters)
{
	uint8_t ucKeyCode;
	uint8_t pcWriteBuffer[500];
	
    while(1)
    {
			ucKeyCode = bsp_GetKey();
		
			if (ucKeyCode != KEY_NONE)
			{
				switch (ucKeyCode)
				{
			 /* K1键按下 打印任务执行情况 */
			 case KEY_DOWN_K1:			 
				 printf("=================================================\r\n");
				 printf("任务名      任务状态 优先级   剩余栈 任务序号\r\n");
				 vTaskList((char *)&pcWriteBuffer);
				 printf("%s\r\n", pcWriteBuffer);
				
				 printf("\r\n任务名       运行计数         使用率\r\n");
				 vTaskGetRunTimeStats((char *)&pcWriteBuffer);
				 printf("%s\r\n", pcWriteBuffer);
			   break;
			 
#if 0 /*增加删除任务*/		 
			 /* K2键按下 删除任务vTaskLED */
				case KEY_DOWN_K2:			 
					printf("K2键按下，删除任务vTaskLED\r\n");
					if(xHandleTaskLED != NULL)
					{
						vTaskDelete(xHandleTaskLED);
						xHandleTaskLED = NULL;
					}
					break;
					
				/* K3键按下 重新创建任务vTaskLED */
				case KEY_DOWN_K3:	
					printf("K3键按下，重新创建任务vTaskLED\r\n");
					if(xHandleTaskLED == NULL)
					{
						xTaskCreate(    vTaskLED,            /* 任务函数  */
										"vTaskLED",          /* 任务名    */
										512,                 /* stack大小，单位word，也就是4字节 */
										NULL,                /* 任务参数  */
										2,                   /* 任务优先级*/
										&xHandleTaskLED );   /* 任务句柄  */
					}
					break;
#endif /*增加删除任务*/	
					
				/* K2键长按下，挂起任务vTaskLED */
				case KEY_LONG_K2:
					printf("K2键长按下，挂起任务vTaskLED\r\n");
					vTaskSuspend(xHandleTaskLED);
					break;
				
				/* K3键长按下，恢复任务vTaskLED */
				case KEY_DOWN_K3:
				  printf("K3键按下，启动单次定时器中断，50ms后在定时器中断将任务vTaskLED恢复\r\n");
					bsp_StartHardTimer(1 ,50000, (void *)TIM_CallBack1);
					break;
				/* 其他的键值不处理 */
				default:                     
					break;
			}
			bsp_LedToggle(1);
		}
		
		vTaskDelay(20);
		
	}
}

/*
*********************************************************************************************************
*	函 数 名: vTaskLED
*	功能说明: LED闪烁	
*	形    参: pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
*   优 先 级: 2  
*********************************************************************************************************
*/
static void vTaskLED(void *pvParameters)
{
    while(1)
    {
		bsp_LedToggle(2);
        vTaskDelay(200);
    }
}

/*
*********************************************************************************************************
*	函 数 名: vTaskMsgPro
*	功能说明: 信息处理，这里是用作LED闪烁	
*	形    参: pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
*   优 先 级: 3  
*********************************************************************************************************
*/
static void vTaskMsgPro(void *pvParameters)
{
    while(1)
    {
		bsp_LedToggle(3);
        vTaskDelay(300);
    }
}

/*
*********************************************************************************************************
*	函 数 名: vTaskStart
*	功能说明: 启动任务，也就是最高优先级任务，这里用作LED闪烁
*	形    参: pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
*   优 先 级: 4  
*********************************************************************************************************
*/
static void vTaskStart(void *pvParameters)
{
    while(1)
    {
		/* 按键扫描 */
		bsp_KeyScan();
        vTaskDelay(10);
    }
}
/*
*********************************************************************************************************
*	函 数 名: TIM_CallBack1
*	功能说明: 定时器中断的回调函数，此函数被bsp_StartHardTimer所调用。		  			  
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void TIM_CallBack1(void)
{
	BaseType_t xYieldRequired;

     /* 恢复挂起任务 */
     xYieldRequired = xTaskResumeFromISR(xHandleTaskLED);

	 /* 退出中断后是否需要执行任务切换 */
     if( xYieldRequired == pdTRUE )
     {
         portYIELD_FROM_ISR(xYieldRequired);
     }
}

/*
*********************************************************************************************************
*	函 数 名: AppTaskCreate
*	功能说明: 创建应用任务
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
static void AppTaskCreate (void)
{
    xTaskCreate( vTaskTaskUserIF,   	/* 任务函数  */
                 "vTaskUserIF",     	/* 任务名    */
                 512,               	/* 任务栈大小，单位word，也就是4字节 */
                 NULL,              	/* 任务参数  */
                 1,                 	/* 任务优先级*/
                 &xHandleTaskUserIF );  /* 任务句柄  */
	
	
	xTaskCreate( vTaskLED,    		/* 任务函数  */
                 "vTaskLED",  		/* 任务名    */
                 512,         		/* 任务栈大小，单位word，也就是4字节 */
                 NULL,        		/* 任务参数  */
                 2,           		/* 任务优先级*/
                 &xHandleTaskLED ); /* 任务句柄  */
	
	xTaskCreate( vTaskMsgPro,     		/* 任务函数  */
                 "vTaskMsgPro",   		/* 任务名    */
                 512,             		/* 任务栈大小，单位word，也就是4字节 */
                 NULL,           		/* 任务参数  */
                 3,               		/* 任务优先级*/
                 &xHandleTaskMsgPro );  /* 任务句柄  */
	
	
	xTaskCreate( vTaskStart,     		/* 任务函数  */
                 "vTaskStart",   		/* 任务名    */
                 512,            		/* 任务栈大小，单位word，也就是4字节 */
                 NULL,           		/* 任务参数  */
                 4,              		/* 任务优先级*/
                 &xHandleTaskStart );   /* 任务句柄  */
}

/***************************** BayNexus (END OF FILE) *********************************/
