/*
*********************************************************************************************************
*
*	ģ������ : ������ģ�顣
*	�ļ����� : main.c
*	��    �� : V1.0
*	˵    �� : ��ʵ����ҪѧϰFreeRTOS����ֲ
*              ʵ��Ŀ�ģ�
*                 1. ѧϰFreeRTOS����ֲ��
*              ʵ�����ݣ�
*                 1. �����������ĸ�����
*                    vTaskTaskUserIF ����: �ӿ���Ϣ������������LED��˸	
*                    vTaskLED        ����: LED��˸
*                    vTaskMsgPro     ����: ��Ϣ��������������LED��˸
*                    vTaskStart      ����: ��������Ҳ����������ȼ�������������LED��˸
*             1. K2�����£�����vTaskLED��
*							2. K3�����£��������ζ�ʱ���жϣ�50ms���ڶ�ʱ���жϽ�����vTaskLED�ָ���
*              ע�����
*              ע�����
*                 1. ��ʵ���Ƽ�ʹ�ô������SecureCRT��Ҫ�����ڴ�ӡЧ�������롣�������
*                    V4��������������С�
*                 2. ��ؽ��༭��������������TAB����Ϊ4���Ķ����ļ���Ҫ��������ʾ�����롣
*
*	�޸ļ�¼ :
*		�汾��    ����         ����            ˵��
*       V1.0   2017          Eric    1. ST�̼��⵽V3.6.1�汾
*                                        2. BSP������V1.2
*                                        3. FreeRTOS�汾V8.2.3
*
*	Copyright (C), 2016-2020,BayNexus 
*
*********************************************************************************************************
*/
#include "includes.h"



/*
**********************************************************************************************************
											��������
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
											��������
**********************************************************************************************************
*/
static TaskHandle_t xHandleTaskUserIF = NULL;
static TaskHandle_t xHandleTaskLED = NULL;
static TaskHandle_t xHandleTaskMsgPro = NULL;
static TaskHandle_t xHandleTaskStart = NULL;

/*
*********************************************************************************************************
*	�� �� ��: main
*	����˵��: ��׼c������ڡ�
*	��    �Σ���
*	�� �� ֵ: ��
*********************************************************************************************************
*/
int main(void)
{
	/* 
	  ����������ǰ��Ϊ�˷�ֹ��ʼ��STM32����ʱ���жϷ������ִ�У������ֹȫ���ж�(����NMI��HardFault)��
	  �������ĺô��ǣ�
	  1. ��ִֹ�е��жϷ����������FreeRTOS��API������
	  2. ��֤ϵͳ�������������ܱ���ж�Ӱ�졣
	  3. �����Ƿ�ر�ȫ���жϣ���Ҹ����Լ���ʵ��������ü��ɡ�
	  ����ֲ�ļ�port.c�еĺ���prvStartFirstTask�л����¿���ȫ���жϡ�ͨ��ָ��cpsie i������__set_PRIMASK(1)
	  ��cpsie i�ǵ�Ч�ġ�
     */
	__set_PRIMASK(1);  
	
	/* Ӳ����ʼ�� */
	bsp_Init(); 
	
	/* 1. ��ʼ��һ����ʱ���жϣ����ȸ��ڵδ�ʱ���жϣ������ſ��Ի��׼ȷ��ϵͳ��Ϣ ��������Ŀ�ģ�ʵ����
		  Ŀ�в�Ҫʹ�ã���Ϊ������ܱȽ�Ӱ��ϵͳʵʱ�ԡ�
	   2. Ϊ����ȷ��ȡFreeRTOS�ĵ�����Ϣ�����Կ��ǽ�����Ĺر��ж�ָ��__set_PRIMASK(1); ע�͵��� 
	*/
	vSetupSysInfoTest();
	/* �������� */
	AppTaskCreate();
	
    /* �������ȣ���ʼִ������ */
    vTaskStartScheduler();

	/* 
	  ���ϵͳ���������ǲ������е�����ģ����е����Ｋ�п��������ڶ�ʱ��������߿��������
	  heap�ռ䲻����ɴ���ʧ�ܣ���Ҫ�Ӵ�FreeRTOSConfig.h�ļ��ж����heap��С��
	  #define configTOTAL_HEAP_SIZE	      ( ( size_t ) ( 17 * 1024 ) )
	*/
	while(1);
}

/*
*********************************************************************************************************
*	�� �� ��: vTaskTaskUserIF
*	����˵��: �ӿ���Ϣ������������LED��˸	
*	��    ��: pvParameters ���ڴ���������ʱ���ݵ��β�
*	�� �� ֵ: ��
*   �� �� ��: 1  (��ֵԽС���ȼ�Խ�ͣ������uCOS�෴)
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
			 /* K1������ ��ӡ����ִ����� */
			 case KEY_DOWN_K1:			 
				 printf("=================================================\r\n");
				 printf("������      ����״̬ ���ȼ�   ʣ��ջ �������\r\n");
				 vTaskList((char *)&pcWriteBuffer);
				 printf("%s\r\n", pcWriteBuffer);
				
				 printf("\r\n������       ���м���         ʹ����\r\n");
				 vTaskGetRunTimeStats((char *)&pcWriteBuffer);
				 printf("%s\r\n", pcWriteBuffer);
			   break;
			 
#if 0 /*����ɾ������*/		 
			 /* K2������ ɾ������vTaskLED */
				case KEY_DOWN_K2:			 
					printf("K2�����£�ɾ������vTaskLED\r\n");
					if(xHandleTaskLED != NULL)
					{
						vTaskDelete(xHandleTaskLED);
						xHandleTaskLED = NULL;
					}
					break;
					
				/* K3������ ���´�������vTaskLED */
				case KEY_DOWN_K3:	
					printf("K3�����£����´�������vTaskLED\r\n");
					if(xHandleTaskLED == NULL)
					{
						xTaskCreate(    vTaskLED,            /* ������  */
										"vTaskLED",          /* ������    */
										512,                 /* stack��С����λword��Ҳ����4�ֽ� */
										NULL,                /* �������  */
										2,                   /* �������ȼ�*/
										&xHandleTaskLED );   /* ������  */
					}
					break;
#endif /*����ɾ������*/	
					
				/* K2�������£���������vTaskLED */
				case KEY_LONG_K2:
					printf("K2�������£���������vTaskLED\r\n");
					vTaskSuspend(xHandleTaskLED);
					break;
				
				/* K3�������£��ָ�����vTaskLED */
				case KEY_DOWN_K3:
				  printf("K3�����£��������ζ�ʱ���жϣ�50ms���ڶ�ʱ���жϽ�����vTaskLED�ָ�\r\n");
					bsp_StartHardTimer(1 ,50000, (void *)TIM_CallBack1);
					break;
				/* �����ļ�ֵ������ */
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
*	�� �� ��: vTaskLED
*	����˵��: LED��˸	
*	��    ��: pvParameters ���ڴ���������ʱ���ݵ��β�
*	�� �� ֵ: ��
*   �� �� ��: 2  
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
*	�� �� ��: vTaskMsgPro
*	����˵��: ��Ϣ��������������LED��˸	
*	��    ��: pvParameters ���ڴ���������ʱ���ݵ��β�
*	�� �� ֵ: ��
*   �� �� ��: 3  
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
*	�� �� ��: vTaskStart
*	����˵��: ��������Ҳ����������ȼ�������������LED��˸
*	��    ��: pvParameters ���ڴ���������ʱ���ݵ��β�
*	�� �� ֵ: ��
*   �� �� ��: 4  
*********************************************************************************************************
*/
static void vTaskStart(void *pvParameters)
{
    while(1)
    {
		/* ����ɨ�� */
		bsp_KeyScan();
        vTaskDelay(10);
    }
}
/*
*********************************************************************************************************
*	�� �� ��: TIM_CallBack1
*	����˵��: ��ʱ���жϵĻص��������˺�����bsp_StartHardTimer�����á�		  			  
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void TIM_CallBack1(void)
{
	BaseType_t xYieldRequired;

     /* �ָ��������� */
     xYieldRequired = xTaskResumeFromISR(xHandleTaskLED);

	 /* �˳��жϺ��Ƿ���Ҫִ�������л� */
     if( xYieldRequired == pdTRUE )
     {
         portYIELD_FROM_ISR(xYieldRequired);
     }
}

/*
*********************************************************************************************************
*	�� �� ��: AppTaskCreate
*	����˵��: ����Ӧ������
*	��    �Σ���
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void AppTaskCreate (void)
{
    xTaskCreate( vTaskTaskUserIF,   	/* ������  */
                 "vTaskUserIF",     	/* ������    */
                 512,               	/* ����ջ��С����λword��Ҳ����4�ֽ� */
                 NULL,              	/* �������  */
                 1,                 	/* �������ȼ�*/
                 &xHandleTaskUserIF );  /* ������  */
	
	
	xTaskCreate( vTaskLED,    		/* ������  */
                 "vTaskLED",  		/* ������    */
                 512,         		/* ����ջ��С����λword��Ҳ����4�ֽ� */
                 NULL,        		/* �������  */
                 2,           		/* �������ȼ�*/
                 &xHandleTaskLED ); /* ������  */
	
	xTaskCreate( vTaskMsgPro,     		/* ������  */
                 "vTaskMsgPro",   		/* ������    */
                 512,             		/* ����ջ��С����λword��Ҳ����4�ֽ� */
                 NULL,           		/* �������  */
                 3,               		/* �������ȼ�*/
                 &xHandleTaskMsgPro );  /* ������  */
	
	
	xTaskCreate( vTaskStart,     		/* ������  */
                 "vTaskStart",   		/* ������    */
                 512,            		/* ����ջ��С����λword��Ҳ����4�ֽ� */
                 NULL,           		/* �������  */
                 4,              		/* �������ȼ�*/
                 &xHandleTaskStart );   /* ������  */
}

/***************************** BayNexus (END OF FILE) *********************************/
