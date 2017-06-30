/*
*********************************************************************************************************
*
*	模块名称 : 主程序模块。
*	文件名称 : main.c
*	版    本 : V1.0
*	说    明 : 本实验主要学习FreeRTOS的移植
*              实验目的：
*                1. 学习FreeRTOS的任务栈溢出检测方法一（模拟栈溢出）。
*                2. FreeRTOS的任务栈溢出检测方法一说明：
*                   a. FreeRTOSConfig.h文件中配置宏定义：
*                      #define  configCHECK_FOR_STACK_OVERFLOW   1
*                   b. 在任务切换时检测任务栈指针是否过界了，如果过界了，在任务切换的时候会触发栈溢出钩子函数。
*                      void vApplicationStackOverflowHook( TaskHandle_t xTask,
*                                                          signed char *pcTaskName );
*                      用户可以在钩子函数里面做一些处理。本实验是在钩子函数中打印出现栈溢出的任务。
*                   c. 这种方法不能保证所有的栈溢出都能检测到。比如任务在执行的过程中发送过栈溢出。任务切换前
*                      栈指针又恢复到了正常水平，这种情况在任务切换的时候是检测不到的。又比如任务栈溢出后，把
*                      这部分栈区的数据修改了，这部分栈区的数据不重要或者暂时没有用到还好，如果是重要数据被修
*                      改将直接导致系统进入硬件异常。这种情况下，栈溢出检测功能也是检测不到的。
*                   d. 本实验就是简单的在任务vTaskUserIF中申请过大的栈空间，模拟出一种栈溢出的情况，溢出后触
*                      发钩子函数，因为我们将溢出部分的数据修改了，进而造成进入硬件异常。
*              实验内容：
*                1. 按下按键K1可以通过串口打印任务执行情况（波特率115200，数据位8，奇偶校验位无，停止位1）
*                   =================================================
*                   任务名      任务状态 优先级   剩余栈 任务序号
*                   vTaskUserIF     R       1       318     1
*                	IDLE            R       0       118     5
*                	vTaskLED        B       2       490     2
*                	vTaskMsgPro     B       3       490     3
*               	vTaskStart      B       4       490     4
*
*                	任务名       运行计数         使用率
*                	vTaskUserIF     467             <1%
*                	IDLE            126495          99%
*                	vTaskMsgPro     1               <1%
*                	vTaskStart      639             <1%
*                	vTaskLED        0               <1%
*                  串口软件建议使用SecureCRT（V4光盘里面有此软件）查看打印信息。
*                  各个任务实现的功能如下：
*                   vTaskTaskUserIF 任务: 接口消息处理	
*                   vTaskLED        任务: LED闪烁
*                   vTaskMsgPro     任务: 消息处理，这里是用作LED闪烁
*                   vTaskStart      任务: 启动任务，也就是最高优先级任务，这里实现按键扫描
*                2. 任务运行状态的定义如下，跟上面串口打印字母B, R, D, S对应：
*                    #define tskBLOCKED_CHAR		( 'B' )  阻塞
*                    #define tskREADY_CHAR		    ( 'R' )  就绪
*                    #define tskDELETED_CHAR		( 'D' )  删除
*                    #define tskSUSPENDED_CHAR	    ( 'S' )  挂起
*                3. K2按键按下，模拟栈溢出。
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
*	函 数 名: StackOverflowTest
*	功能说明: 任务栈溢出测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void StackOverflowTest(void)
{
	int16_t i;
	uint8_t buf[2048];
	
	(void)buf; /* 防止警告 */
	
	/*
	  1. 为了能够模拟任务栈溢出，并触发任务栈溢出函数，这里强烈建议使用数组的时候逆着赋值。
	     因为对于M3和M4内核的MCU，堆栈生长方向是向下生长的满栈。即高地址是buf[2047], 低地址
	     是buf[0]。如果任务栈溢出了，也是从高地址buf[2047]到buf[0]的某个地址开始溢出。
	        因此，如果用户直接修改的是buf[0]开始的数据且这些溢出部分的数据比较重要，会直接导致
	     进入到硬件异常。
	  2. 栈溢出检测是在任务切换的时候执行的，我们这里加个延迟函数，防止修改了重要的数据导致直接
	     进入硬件异常。
	  3. 任务vTaskTaskUserIF的栈空间大小是2048字节，在此任务的入口已经申请了栈空间大小
		 ------uint8_t ucKeyCode;
	     ------uint8_t pcWriteBuffer[500];
	     这里再申请如下这么大的栈空间
	     -------int16_t i;
		 -------uint8_t buf[2048];
	     必定溢出。
	*/
		//for(i = 0; i <= 2017; i++)
	for(i = 2047; i >= 0; i--)
	{
		printf("|%d",i);
		buf[i] = 0x55;
	
		vTaskDelay(1);
	}
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
					
				case KEY_DOWN_K2:			 
					printf("K2键按下，模拟任务栈溢出检测\r\n");
					//StackOverflowTest();
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
	static uint16_t time=0;
    while(1)
    {
			printf("时间：%d S\r\n",time++);
		bsp_LedToggle(2);
        vTaskDelay(1000);
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
        vTaskDelay(500);
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
		/* 
	  开始执行启动任务主函数前使能独立看门狗。
	  设置LSI是128分频，下面函数参数范围0-0xFFF，分别代表最小值3.2ms和最大值13107.2ms
	  下面设置的是10s，如果10s内没有喂狗，系统复位。
	*/
	bsp_InitIwdg(0x35);
	
	/* 打印系统开机状态，方便查看系统是否复位 */
	printf("=====================================================\r\n");
	printf("=系统开机执行\r\n");
	printf("=====================================================\r\n");
	
    while(1)
    {
			IWDG_Feed();
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
*	函 数 名: vApplicationStackOverflowHook
*	功能说明: 栈溢出的钩子函数
*	形    参: xTask        任务句柄
*             pcTaskName   任务名
*	返 回 值: 无
*********************************************************************************************************
*/
void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
	printf("任务：%s 发现栈溢出\r\n", pcTaskName);
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
