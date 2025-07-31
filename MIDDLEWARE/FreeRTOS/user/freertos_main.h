/*!
    \file       freertos_main.h
    \brief      FreeRTOS main header file for task and queue declarations
    \version    1.0
    \date       2025-07-26
    \author     Ze-Hou
*/

#ifndef __FREERTOS_MAIN_H
#define __FREERTOS_MAIN_H
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/******************************************************************************************************/
/* Task handles */
extern TaskHandle_t DAP_LINKTask_Handler;           /*!< DAP link task handle */
extern TaskHandle_t LVGLTask_Handler;               /*!< LVGL task handle */
extern TaskHandle_t OTHERTask_Handler;              /*!< Other tasks handle */
extern TaskHandle_t DBUGGER_DOWNLOADTask_Handler;   /*!< Debugger download task handle */
extern TaskHandle_t WIRELESSTask_Handler;           /*!< Wireless communication task handle */

/* Queue handles */
extern QueueHandle_t xQueueIna226Data;              /*!< INA226 data queue handle */
extern QueueHandle_t xQueueCpuInfo;                 /*!< CPU information queue handle */
extern QueueHandle_t xQueueLvglMainState;           /*!< LVGL main state queue handle */
extern QueueHandle_t xQueueRtcData;                 /*!< RTC data queue handle */
extern QueueHandle_t xQueueMEMPerused;              /*!< Memory usage percentage queue handle */
extern QueueHandle_t xQueueDebuggerDownload;        /*!< Debugger download queue handle */
extern QueueHandle_t xQueueWirelseeState;           /*!< Wireless state queue handle */

/* Semaphore handles */

/******************************************************************************************************/

/*!
    \brief      FreeRTOS main initialization function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void freertos_main(void);
#endif /* __FREERTOS_MAIN_H */
