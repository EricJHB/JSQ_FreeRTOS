/*
*********************************************************************************************************
*
*	ģ������ : ������ģ�顣
*	�ļ����� : main.c
*	��    �� : V1.0
*	˵    �� : ��ʵ����ҪѧϰFreeRTOS����ֲ
*              ʵ��Ŀ�ģ�
*                1. ѧϰFreeRTOS������ջ�����ⷽ��һ��ģ��ջ�������
*                2. FreeRTOS������ջ�����ⷽ��һ˵����
*                   a. FreeRTOSConfig.h�ļ������ú궨�壺
*                      #define  configCHECK_FOR_STACK_OVERFLOW   1
*                   b. �������л�ʱ�������ջָ���Ƿ�����ˣ���������ˣ��������л���ʱ��ᴥ��ջ������Ӻ�����
*                      void vApplicationStackOverflowHook( TaskHandle_t xTask,
*                                                          signed char *pcTaskName );
*                      �û������ڹ��Ӻ���������һЩ������ʵ�����ڹ��Ӻ����д�ӡ����ջ���������
*                   c. ���ַ������ܱ�֤���е�ջ������ܼ�⵽������������ִ�еĹ����з��͹�ջ����������л�ǰ
*                      ջָ���ָֻ���������ˮƽ����������������л���ʱ���Ǽ�ⲻ���ġ��ֱ�������ջ����󣬰�
*                      �ⲿ��ջ���������޸��ˣ��ⲿ��ջ�������ݲ���Ҫ������ʱû���õ����ã��������Ҫ���ݱ���
*                      �Ľ�ֱ�ӵ���ϵͳ����Ӳ���쳣����������£�ջ�����⹦��Ҳ�Ǽ�ⲻ���ġ�
*                   d. ��ʵ����Ǽ򵥵�������vTaskUserIF����������ջ�ռ䣬ģ���һ��ջ���������������
*                      �����Ӻ�������Ϊ���ǽ�������ֵ������޸��ˣ�������ɽ���Ӳ���쳣��
*              ʵ�����ݣ�
*                1. ���°���K1����ͨ�����ڴ�ӡ����ִ�������������115200������λ8����żУ��λ�ޣ�ֹͣλ1��
*                   =================================================
*                   ������      ����״̬ ���ȼ�   ʣ��ջ �������
*                   vTaskUserIF     R       1       318     1
*                	IDLE            R       0       118     5
*                	vTaskLED        B       2       490     2
*                	vTaskMsgPro     B       3       490     3
*               	vTaskStart      B       4       490     4
*
*                	������       ���м���         ʹ����
*                	vTaskUserIF     467             <1%
*                	IDLE            126495          99%
*                	vTaskMsgPro     1               <1%
*                	vTaskStart      639             <1%
*                	vTaskLED        0               <1%
*                  �����������ʹ��SecureCRT��V4���������д�������鿴��ӡ��Ϣ��
*                  ��������ʵ�ֵĹ������£�
*                   vTaskTaskUserIF ����: �ӿ���Ϣ����	
*                   vTaskLED        ����: LED��˸
*                   vTaskMsgPro     ����: ��Ϣ��������������LED��˸
*                   vTaskStart      ����: ��������Ҳ����������ȼ���������ʵ�ְ���ɨ��
*                2. ��������״̬�Ķ������£������洮�ڴ�ӡ��ĸB, R, D, S��Ӧ��
*                    #define tskBLOCKED_CHAR		( 'B' )  ����
*                    #define tskREADY_CHAR		    ( 'R' )  ����
*                    #define tskDELETED_CHAR		( 'D' )  ɾ��
*                    #define tskSUSPENDED_CHAR	    ( 'S' )  ����
*                3. K2�������£�ģ��ջ�����
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
static void AppObjCreate (void);
/*
**********************************************************************************************************
											��������
**********************************************************************************************************
*/
static TaskHandle_t xHandleTaskUserIF = NULL;
static TaskHandle_t xHandleTaskLED = NULL;
static TaskHandle_t xHandleTaskMsgPro = NULL;
static TaskHandle_t xHandleTaskStart = NULL;
/* ��Ϣ���� */
static QueueHandle_t xQueue1 = NULL;
static QueueHandle_t xQueue2 = NULL;
static uint32_t msg_ucCount = 0;

typedef struct Msg
{
	uint8_t  ucMessageID;
	uint16_t usData[2];
	uint32_t ulData[2];
}MSG_T;

MSG_T   g_tMsg; /* ����һ���ṹ��������Ϣ���� */
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
	/* ��������ͨ�Ż��� */
	AppObjCreate();
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
*	�� �� ��: StackOverflowTest
*	����˵��: ����ջ�������
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void StackOverflowTest(void)
{
	int16_t i;
	uint8_t buf[2048];
	
	(void)buf; /* ��ֹ���� */
	
	/*
	  1. Ϊ���ܹ�ģ������ջ���������������ջ�������������ǿ�ҽ���ʹ�������ʱ�����Ÿ�ֵ��
	     ��Ϊ����M3��M4�ں˵�MCU����ջ����������������������ջ�����ߵ�ַ��buf[2047], �͵�ַ
	     ��buf[0]���������ջ����ˣ�Ҳ�ǴӸߵ�ַbuf[2047]��buf[0]��ĳ����ַ��ʼ�����
	        ��ˣ�����û�ֱ���޸ĵ���buf[0]��ʼ����������Щ������ֵ����ݱȽ���Ҫ����ֱ�ӵ���
	     ���뵽Ӳ���쳣��
	  2. ջ���������������л���ʱ��ִ�еģ���������Ӹ��ӳٺ�������ֹ�޸�����Ҫ�����ݵ���ֱ��
	     ����Ӳ���쳣��
	  3. ����vTaskTaskUserIF��ջ�ռ��С��2048�ֽڣ��ڴ����������Ѿ�������ջ�ռ��С
		 ------uint8_t ucKeyCode;
	     ------uint8_t pcWriteBuffer[500];
	     ����������������ô���ջ�ռ�
	     -------int16_t i;
		 -------uint8_t buf[2048];
	     �ض������
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
*	�� �� ��: vTaskTaskUserIF
*	����˵��: �ӿ���Ϣ������������LED��˸	
*	��    ��: pvParameters ���ڴ���������ʱ���ݵ��β�
*	�� �� ֵ: ��
*   �� �� ��: 1  (��ֵԽС���ȼ�Խ�ͣ������uCOS�෴)
*********************************************************************************************************
*/
static void vTaskTaskUserIF(void *pvParameters)
{
	MSG_T   *ptMsg;
	uint8_t ucCount = 0;
	uint8_t ucKeyCode;
	uint8_t pcWriteBuffer[500];
	
	/* ��ʼ���ṹ��ָ�� */
	ptMsg = &g_tMsg;
	
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
					
	/* K2�����£���xQueue1�������� */
				case KEY_DOWN_K2:
					ucCount++;
				
					/* ����Ϣ���з����ݣ������Ϣ�������ˣ��ȴ�10��ʱ�ӽ��� */
					if( xQueueSend(xQueue2,
								   (void *) &ucCount,
								   (TickType_t)10) != pdPASS )
					{
						/* ����ʧ�ܣ���ʹ�ȴ���10��ʱ�ӽ��� */
						printf("K2�����£���xQueue1��������ʧ�ܣ���ʹ�ȴ���10��ʱ�ӽ���\r\n");
					}
					else
					{
						/* ���ͳɹ� */
						printf("K2�����£���xQueue1�������ݳɹ�\r\n");						
					}
					break;
				
				/* K3�����£���xQueue2�������� */
				case KEY_DOWN_K3:
					ptMsg->ucMessageID++;
					ptMsg->ulData[0]++;;
					ptMsg->usData[0]++;
					
					/* ʹ����Ϣ����ʵ��ָ������Ĵ��� */
					if(xQueueSend(xQueue2,                  /* ��Ϣ���о�� */
								 (void *) &ptMsg,           /* ���ͽṹ��ָ�����ptMsg�ĵ�ַ */
								 (TickType_t)10) != pdPASS )
					{
						/* ����ʧ�ܣ���ʹ�ȴ���10��ʱ�ӽ��� */
						printf("K3�����£���xQueue2��������ʧ�ܣ���ʹ�ȴ���10��ʱ�ӽ���\r\n");
					}
					else
					{
						/* ���ͳɹ� */
						printf("K3�����£���xQueue2�������ݳɹ�\r\n");						
					}
				/* �����ļ�ֵ������ */
				default:                     
					break;
			}
			//bsp_LedToggle(1);
		}
			bsp_LedToggle(1);
		IWDG_Feed();
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
	MSG_T *ptMsg;
	BaseType_t xResult;
	const TickType_t xMaxBlockTime = pdMS_TO_TICKS(200); /* �������ȴ�ʱ��Ϊ200ms */
	
    while(1)
    {
		xResult = xQueueReceive(xQueue2,                   /* ��Ϣ���о�� */
		                        (void *)&ptMsg,  		   /* �����ȡ���ǽṹ��ĵ�ַ */
		                        (TickType_t)xMaxBlockTime);/* ��������ʱ�� */
		
		
		if(xResult == pdPASS)
		{
			/* �ɹ����գ���ͨ�����ڽ����ݴ�ӡ���� */
			printf("���յ���Ϣ��������ptMsg->ucMessageID = %d\r\n", ptMsg->ucMessageID);
			printf("���յ���Ϣ��������ptMsg->ulData[0] = %d\r\n", ptMsg->ulData[0]);
			printf("���յ���Ϣ��������ptMsg->usData[0] = %d\r\n", ptMsg->usData[0]);
		}
		else
		{
			/* ��ʱ */
			//printf("���յ���Ϣ�������ݳ�ʱ\r\n");
			bsp_LedToggle(2);
			//bsp_LedToggle(3);
		}
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
	uint32_t led_on_time;
	uint32_t type = 0;
	uint32_t count_old;
	uint32_t led_on_time_old;
	BaseType_t xResult;
	const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100); /* �������ȴ�ʱ��Ϊ100ms */
    while(1)
    {
			led_on_time = msg_ucCount%200;//����10ms ���ǲ�

			if(ulHighFrequencyTimerTicks != count_old)//Ƶ��50usһ��
			{
				if(type == 0)
				{
					if(led_on_time == 0 && led_on_time!=led_on_time_old)type = 1; 
					if(ulHighFrequencyTimerTicks%200 >= led_on_time)//����5ms
					{
						bsp_LedOn(3);
						bsp_LedOn(4);
					}
					else 
					{
						bsp_LedOff(3);
						bsp_LedOff(4);
					}
				}
				else
				{
					if(led_on_time == 0 && led_on_time!=led_on_time_old)type = 0; 
					if(ulHighFrequencyTimerTicks%200 >= led_on_time)//����5ms
					{
						bsp_LedOff(3);
						bsp_LedOff(4);
					}
					else 
					{
						bsp_LedOn(3);
						bsp_LedOn(4);
					}
				}
				count_old = ulHighFrequencyTimerTicks;
				led_on_time_old = led_on_time;
			}
			/*
			bsp_LedOn(3);
			vTaskDelay(led_on_time);
			bsp_LedOff(3);
			vTaskDelay(5-led_on_time);
			*/
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
		/* 
	  ��ʼִ����������������ǰʹ�ܶ������Ź���
	  ����LSI��128��Ƶ�����溯��������Χ0-0xFFF���ֱ������Сֵ3.2ms�����ֵ13107.2ms
	  �������õ���10s�����10s��û��ι����ϵͳ��λ��
	*/
	//bsp_InitIwdg(0x35);
	
	/* ��ӡϵͳ����״̬������鿴ϵͳ�Ƿ�λ */
	printf("=====================================================\r\n");
	printf("=ϵͳ����ִ��\r\n");
	printf("=====================================================\r\n");
	
    while(1)
    {
			msg_ucCount++;
			#if 0
								/* ����Ϣ���з����ݣ������Ϣ�������ˣ��ȴ�10��ʱ�ӽ��� */
					if( xQueueSend(xQueue1,
								   (void *) &ucCount,
								   (TickType_t)10) != pdPASS )
					{
						/* ����ʧ�ܣ���ʹ�ȴ���10��ʱ�ӽ��� */
						printf("��xQueue1��������ʧ�ܣ���ʹ�ȴ���10��ʱ�ӽ���\r\n");
					}
					else
					{
						/* ���ͳɹ� */
						printf("��xQueue1�������ݳɹ�\r\n");						
					}
					#endif
			IWDG_Feed();
		/* ����ɨ�� */
		  bsp_KeyScan();
      vTaskDelay(20);
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
		printf("============TIM_CallBack1=========================\r\n");
	 /* �˳��жϺ��Ƿ���Ҫִ�������л� */
     if( xYieldRequired == pdTRUE )
     {
         portYIELD_FROM_ISR(xYieldRequired);
     }
}
/*
*********************************************************************************************************
*	�� �� ��: vApplicationStackOverflowHook
*	����˵��: ջ����Ĺ��Ӻ���
*	��    ��: xTask        ������
*             pcTaskName   ������
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
	printf("����%s ����ջ���\r\n", pcTaskName);
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
/*
*********************************************************************************************************
*	�� �� ��: AppObjCreate
*	����˵��: ��������ͨ�Ż���
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void AppObjCreate (void)
{
	/* ����10��uint8_t����Ϣ���� */
	xQueue1 = xQueueCreate(10, sizeof(uint8_t));
    if( xQueue1 == 0 )
    {
        /* û�д����ɹ����û�������������봴��ʧ�ܵĴ������ */
    }
	
	/* ����10���洢ָ���������Ϣ���У�����CM3/CM4�ں���32λ����һ��ָ�����ռ��4���ֽ� */
	xQueue2 = xQueueCreate(10, sizeof(struct Msg *));
    if( xQueue2 == 0 )
    {
        /* û�д����ɹ����û�������������봴��ʧ�ܵĴ������ */
    }
}
/***************************** BayNexus (END OF FILE) *********************************/
