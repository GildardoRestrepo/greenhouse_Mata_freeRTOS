/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_types.h"      /* app_event_t: Item Size de qEvents */
#include "task_logger.h"    /* log_msg_t: Item Size de qLog */
#include "task_actuators.h" /* act_fatal_stack_overflow */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for tActuators */
osThreadId_t tActuatorsHandle;
const osThreadAttr_t tActuators_attributes = {
  .name = "tActuators",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for tControl */
osThreadId_t tControlHandle;
const osThreadAttr_t tControl_attributes = {
  .name = "tControl",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for tSensors */
osThreadId_t tSensorsHandle;
const osThreadAttr_t tSensors_attributes = {
  .name = "tSensors",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for tDisplay */
osThreadId_t tDisplayHandle;
const osThreadAttr_t tDisplay_attributes = {
  .name = "tDisplay",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for tLogger */
osThreadId_t tLoggerHandle;
const osThreadAttr_t tLogger_attributes = {
  .name = "tLogger",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for qEvents */
osMessageQueueId_t qEventsHandle;
const osMessageQueueAttr_t qEvents_attributes = {
  .name = "qEvents"
};
/* Definitions for qLog */
osMessageQueueId_t qLogHandle;
const osMessageQueueAttr_t qLog_attributes = {
  .name = "qLog"
};
/* Definitions for mtxI2C */
osMutexId_t mtxI2CHandle;
const osMutexAttr_t mtxI2C_attributes = {
  .name = "mtxI2C"
};
/* Definitions for mtxData */
osMutexId_t mtxDataHandle;
const osMutexAttr_t mtxData_attributes = {
  .name = "mtxData"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartActuatorsTask(void *argument);
void StartControlTask(void *argument);
void StartSensorsTask(void *argument);
void StartDisplayTask(void *argument);
void StartLoggerTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
   (void)xTask;
   (void)pcTaskName;
   act_fatal_stack_overflow();     /* bomba y buzzer apagados, LED rojo fijo; no retorna */
}
/* USER CODE END 4 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of mtxI2C */
  mtxI2CHandle = osMutexNew(&mtxI2C_attributes);

  /* creation of mtxData */
  mtxDataHandle = osMutexNew(&mtxData_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of qEvents */
  qEventsHandle = osMessageQueueNew (8, sizeof(app_event_t), &qEvents_attributes);

  /* creation of qLog */
  qLogHandle = osMessageQueueNew (16, sizeof(log_msg_t), &qLog_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of tActuators */
  tActuatorsHandle = osThreadNew(StartActuatorsTask, NULL, &tActuators_attributes);

  /* creation of tControl */
  tControlHandle = osThreadNew(StartControlTask, NULL, &tControl_attributes);

  /* creation of tSensors */
  tSensorsHandle = osThreadNew(StartSensorsTask, NULL, &tSensors_attributes);

  /* creation of tDisplay */
  tDisplayHandle = osThreadNew(StartDisplayTask, NULL, &tDisplay_attributes);

  /* creation of tLogger */
  tLoggerHandle = osThreadNew(StartLoggerTask, NULL, &tLogger_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartActuatorsTask */
/**
  * @brief  Function implementing the tActuators thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartActuatorsTask */
__weak void StartActuatorsTask(void *argument)
{
  /* USER CODE BEGIN StartActuatorsTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartActuatorsTask */
}

/* USER CODE BEGIN Header_StartControlTask */
/**
* @brief Function implementing the tControl thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartControlTask */
__weak void StartControlTask(void *argument)
{
  /* USER CODE BEGIN StartControlTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartControlTask */
}

/* USER CODE BEGIN Header_StartSensorsTask */
/**
* @brief Function implementing the tSensors thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSensorsTask */
__weak void StartSensorsTask(void *argument)
{
  /* USER CODE BEGIN StartSensorsTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartSensorsTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief Function implementing the tDisplay thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDisplayTask */
__weak void StartDisplayTask(void *argument)
{
  /* USER CODE BEGIN StartDisplayTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDisplayTask */
}

/* USER CODE BEGIN Header_StartLoggerTask */
/**
* @brief Function implementing the tLogger thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLoggerTask */
__weak void StartLoggerTask(void *argument)
{
  /* USER CODE BEGIN StartLoggerTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartLoggerTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

