#include "cpu_utils.h"
#include "FreeRTOS.h"
#include "task.h"

/********************** NOTES **********************************************
To use this module, the following steps should be followed :

1- in the _OS_Config.h file (ex. FreeRTOSConfig.h) enable the following macros :
      - #define configUSE_IDLE_HOOK        1
      - #define configUSE_TICK_HOOK        1

2- in the _OS_Config.h define the following macros :
      - #define traceTASK_SWITCHED_IN()  extern void StartIdleMonitor(void); \
                                         StartIdleMonitor()
      - #define traceTASK_SWITCHED_OUT() extern void EndIdleMonitor(void); \
                                         EndIdleMonitor()
*******************************************************************************/

xTaskHandle xIdleHandle = NULL;
static uint8_t osCpuUtilization = 0;
static TickType_t osCpuIdleStartTime = 0;
static TickType_t osCpuIdleSpentTime = 0;
static TickType_t osCpuTotalIdleTime = 0;

volatile static uint16_t tick_count = 0;

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Application Idle Hook
  * @param  None
  * @retval None
  */
void vApplicationIdleHook(void)
{
    if(xIdleHandle == NULL)
    {
        /* Store the handle to the idle task. */
        xIdleHandle = xTaskGetCurrentTaskHandle();
    }
}

/**
  * @brief  Application Idle Hook
  * @param  None
  * @retval None
  */
void vApplicationTickHook(void)
{
    if(tick_count++ >= CALCULATION_PERIOD)
    {
        tick_count = 0;

        if(osCpuTotalIdleTime > CALCULATION_PERIOD)
        {
            osCpuTotalIdleTime = CALCULATION_PERIOD;
        }
        
        osCpuUtilization = (100 - (osCpuTotalIdleTime * 100) / CALCULATION_PERIOD);
        osCpuTotalIdleTime = 0;
    }
}

/**
  * @brief  Start Idle monitor
  * @param  None
  * @retval None
  */
void StartIdleMonitor(void)
{
    if(xTaskGetCurrentTaskHandle() == xIdleHandle)
    {
        osCpuIdleStartTime = xTaskGetTickCountFromISR();
    }
}

/**
  * @brief  Stop Idle monitor
  * @param  None
  * @retval None
  */
void EndIdleMonitor(void)
{
    if(xTaskGetCurrentTaskHandle() == xIdleHandle)
    {
        /* Store the handle to the idle task. */
        osCpuIdleSpentTime = xTaskGetTickCountFromISR() - osCpuIdleStartTime;
        osCpuTotalIdleTime += osCpuIdleSpentTime;
    }
}

/**
  * @brief  Stop Idle monitor
  * @param  None
  * @retval None
  */
uint8_t osGetCpuUUsage(void)
{
    return (uint8_t)osCpuUtilization;
}
