/*!
    \file       delay.c
    \brief      system delay driver based on SysTick timer
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./SYSTEM/system.h"
#include "./DELAY/delay.h"
#include <stdint.h>
 
/* include the following header files if using OS */
#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"					/* FreeRTOS support */		  
#include "task.h" 
#endif

static uint16_t  fac_us=0;							/* microsecond delay multiplier */	   
static uint16_t  fac_ms=0;							/* millisecond delay multiplier, represents ms per tick in RTOS */

/*!
    \brief      SysTick timer interrupt service routine
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SysTick_Handler(void)
{
	#if SYSTEM_SUPPORT_OS
		if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)    /* check if scheduler is running */
		{
			extern void xPortSysTickHandler(void);
			xPortSysTickHandler();	                               /* call FreeRTOS tick handler */
		}
	#endif
}

/*!
    \brief      initialize delay function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void delay_init()
{
	uint32_t reload;
	
	/* set SysTick clock source to system clock for FreeRTOS compatibility */
	systick_clksource_set(SYSTICK_CLKSOURCE_CKSYS);
	
	/* calculate microsecond delay multiplier, needed regardless of OS usage */
	fac_us = SystemCoreClock / 1000000;				        
	
	/* calculate counts per microsecond */
	reload = SystemCoreClock / 1000000;				        

	#if SYSTEM_SUPPORT_OS
		/* set overflow time according to configTICK_RATE_HZ */
		reload *= 1000000 / configTICK_RATE_HZ;		    	    
		/* note: reload is 24-bit register, maximum value is 16777216 */
		
		/* calculate minimum delay unit for OS (milliseconds) */
		fac_ms = 1000 / configTICK_RATE_HZ;				        

		/* enable SysTick interrupt for FreeRTOS task scheduling */
		SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;   	        
		
		/* set reload value for interrupt every 1/configTICK_RATE_HZ seconds */
		SysTick->LOAD = reload - 1; 	
	#else
		/* bare-metal environment: use reload for 1ms delay */
		reload *= 1000;
		SysTick->LOAD = reload - 1;
	#endif


	/* enable SysTick timer */
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;   	        
}								    


/*!
    \brief      microsecond precision delay function
    \param[in]  nus: delay time in microseconds (0~7158278, max value is 2^32/fac_us@fac_us=600)
    \param[out] none
    \retval     none
*/
void delay_us(uint32_t nus)
{		
	uint32_t ticks;                                     /* target tick count */
	uint32_t told, tnow, tcnt = 0;                       	/* previous, current, accumulated tick count */
	uint32_t reload = SysTick->LOAD + 1;			    /* reload value (actual LOAD+1) */
	
	ticks = nus * fac_us; 						        /* calculate required tick count */
	told = SysTick->VAL;        				        /* record initial counter value */
	
	while(1)
	{
		tnow = SysTick->VAL;	
        (tnow < told) ? (tcnt += (told-tnow)) : (tcnt += (reload-tnow+told));
        told = tnow;
        if(tcnt >= ticks) break; 
	}										    
}  

/*!
    \brief      intelligent millisecond delay function
    \param[in]  nms: delay time in milliseconds (0~65535)
    \param[out] none
    \retval     none
*/
void delay_ms(uint16_t nms)
{
	#if SYSTEM_SUPPORT_OS
		/* check if FreeRTOS scheduler is running */
		if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
		{		
			/* use system delay if delay time >= OS minimum time period */
			if(nms >= fac_ms)						                            
			{ 
				vTaskDelay(nms / fac_ms);	 		                        /* use FreeRTOS delay, release CPU */
			}
			
			/* calculate remaining time less than OS tick, use busy-wait */
			nms %= fac_ms;						                            
		}
		
		/* use normal delay for remaining time or when scheduler not started */
		delay_us((uint32_t)(nms * 1000));				                        
	#else
		/* bare-metal environment: use microsecond delay loop */
		for(uint16_t i = 0; i < nms; i++) 
		{
			delay_us(1000);			                                    /* delay 1000us (1ms) each time */
		}
	#endif
}

/*!
    \brief      force busy-wait millisecond delay function
    \param[in]  nms: delay time in milliseconds (0~65535)
    \param[out] none
    \retval     none
*/
void delay_xms(uint16_t nms)
{
	/* loop nms times, delay 1ms each time */
	for(uint16_t i = 0; i < nms; i++) 
	{
		delay_us(1000);                                /* precise delay 1000 microseconds */
	}
}
