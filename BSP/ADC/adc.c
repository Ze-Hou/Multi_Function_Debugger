/*!
    \file       adc.c
    \brief      ADC2 internal channel driver with DMA support
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./ADC/adc.h"
#include "./DELAY/delay.h"
#include "./TIMER/timer.h"
#include "./USART/usart.h"

/* ADC2 internal channel data buffer */
static uint16_t adc2_internal_channel_data[ADC2_INTERNAL_CHANNEL_DATA_LENGTH][4];
volatile static uint8_t adc2_dam_flag = 0;                 /*!< DMA transfer completion flag: 0=no transfer, 1=half transfer, 2=full transfer */
#define ADC_DMA_HALF_TRANSFER_FLAG      1
#define ADC_DMA_FULL_TRANSFER_FLAG      2

static float cpu_temperature_slope = 0.0;                   /*!< CPU temperature slope for calibration */
#define HIGH_PRECISION_CPU_TEMPERATURE_SLOPE = 3.3;         /*!< High precision temperature sensor slope for calibration, 3.3mV/℃ */


/* static function declarations */
static void adc2_internal_channel_dma_config(void);        /*!< configure DMA for ADC2 internal channel data transfer */

/*!
    \brief      configure ADC2 for internal channel sampling
    \param[in]  none
    \param[out] none
    \retval     none
    \details    Configures ADC2 to sample internal channels including VBAT, temperature sensor,
                internal reference voltage, and high precision temperature sensor. Sets up
                oversampling, DMA transfer, and TIMER40 for periodic conversion triggering.
                Sampling frequency is 100Hz with 10ms intervals.
*/
void adc2_internal_channel_config(void)
{
    /* configure ADC clock source */
    rcu_adc_clock_config(IDX_ADC2, RCU_ADCSRC_PLL1P);
    
    /* enable ADC2 peripheral clock */
    rcu_periph_clock_enable(RCU_ADC2);
    adc_clock_config(ADC2, ADC_CLK_ASYNC_DIV2);             /* CK_ADC = PLLIP / 2 */
    
    /* configure ADC2 special functions */
    adc_special_function_config(ADC2, ADC_CONTINUOUS_MODE, DISABLE);
    adc_special_function_config(ADC2, ADC_SCAN_MODE, ENABLE);
    adc_resolution_config(ADC2, ADC_RESOLUTION_12B);
    adc_data_alignment_config(ADC2, ADC_DATAALIGN_RIGHT);
    
    /* configure regular channel sequence length */
    adc_channel_length_config(ADC2, ADC_REGULAR_CHANNEL, 4);
    
    /* configure regular channel sequence with sampling time */
    /* t(adc) = 1000000 / 65MHz * (12.5 + 640.5) = 10.046us, t(total) = 10.046us * 4 = 40.184us */
    adc_regular_channel_config(ADC2, 0, ADC_CHANNEL_17, 638);   /* ADC 1/4 voltage of external battery, 640.5 cycles */
    adc_regular_channel_config(ADC2, 1, ADC_CHANNEL_18, 638);   /* ADC temperature sensor channel, 640.5 cycles */
    adc_regular_channel_config(ADC2, 2, ADC_CHANNEL_19, 638);   /* ADC internal reference voltage channel, 640.5 cycles */
    adc_regular_channel_config(ADC2, 3, ADC_CHANNEL_20, 638);   /* ADC high precision temperature sensor channel, 640.5 cycles */
    
    /* enable internal channels */
    adc_internal_channel_config(ADC_CHANNEL_INTERNAL_VBAT, ENABLE);
    adc_internal_channel_config(ADC_CHANNEL_INTERNAL_TEMPSENSOR, ENABLE);
    adc_internal_channel_config(ADC_CHANNEL_INTERNAL_VREFINT, ENABLE);
    adc_internal_channel_config(ADC_CHANNEL_INTERNAL_HP_TEMPSENSOR, ENABLE);
    
    /* configure oversampling mode */
    /* t(oversample total) = 40.184us * 128 = 10287.104us */
    adc_oversample_mode_config(ADC2, ADC_OVERSAMPLING_ALL_CONVERT, ADC_OVERSAMPLING_SHIFT_8B, 255);
    adc_oversample_mode_enable(ADC2);
    
    /* configure external trigger */
    adc_external_trigger_config(ADC2, ADC_REGULAR_CHANNEL, EXTERNAL_TRIGGER_RISING);
    
    /* configure DMA mode */
    adc_dma_request_after_last_enable(ADC2);
    adc_dma_mode_enable(ADC2);
    
    /* enable ADC2 */
    adc_enable(ADC2);
    delay_xms(10);
    
    /* calibrate ADC2 */
    adc_calibration_mode_config(ADC2, ADC_CALIBRATION_OFFSET);
    adc_calibration_number(ADC2, ADC_CALIBRATION_NUM16);
    adc_calibration_enable(ADC2);
    
    /* configure DMA and timer for periodic conversion */
    adc2_internal_channel_dma_config();
    timer_general40_config(300, 10000);                     /* trigger conversion every 10ms, sampling rate 100Hz */

    cpu_temperature_slope = (float)(ADC_TEMP_CALIBRATION_VALUE_MINUS40 - ADC_TEMP_CALIBRATION_VALUE_25) *3.3 / 4095 /(25 + 40);
}

/*!
    \brief      get ADC2 internal channel data from DMA buffer
    \param[in]  adc2_data: pointer to ADC2 data structure
    \param[out] adc2_data: ADC2 internal channel measurement results
    \retval     0: no new data available
    \retval     1: half transfer data available
    \retval     2: full transfer data available
*/
uint8_t adc2_get_internal_channel_data(adc2_data_struct *adc2_data)
{
    uint8_t ret = 0;
    uint8_t i = 0, adc_count = 0;
    uint32_t adc_value[4];

    adc_count = ADC2_INTERNAL_CHANNEL_DATA_LENGTH / 2;      /* half transfer count */
    /* initialize accumulation variables */
    adc_value[0] = 0;
    adc_value[1] = 0;
    adc_value[2] = 0;
    adc_value[3] = 0;

    switch(adc2_dam_flag)
    {
        case 1: /* half transfer complete */
            adc2_dam_flag = 0;
            /* accumulate first half of buffer data */
            for(i = 0; i < adc_count; i++)
            {
                adc_value[0] += adc2_internal_channel_data[i][0];   /* VBAT channel */
                adc_value[1] += adc2_internal_channel_data[i][1];   /* temperature sensor */
                adc_value[2] += adc2_internal_channel_data[i][2];   /* internal Vref */
                adc_value[3] += adc2_internal_channel_data[i][3];   /* high precision temperature */
            }
            /* convert to physical units */
            adc2_data->vbat = (float)adc_value[0] / adc_count* 3.3 / 4095 * 4;  /* VBAT = ADC * 3.3V / 4095 * 4 (1/4 divider) */
            adc2_data->cpu_temperature = (float)(ADC_TEMP_CALIBRATION_VALUE_25 - (float)adc_value[1] / adc_count) * 3.3 / 4095 \
                                         / cpu_temperature_slope + 25;          /* temperature calculation with slope correction */
            adc2_data->internal_vref = (float)((float)adc_value[2] / adc_count) * 3.3 / 4095;   /* internal reference voltage */
            adc2_data->high_precision_cpu_temperature = (float)((float)adc_value[3] / adc_count - ADC_HIGH_TEMP_CALIBRATION_VALUE_25) \
                                                        *3.3 /4095 / (3.3 / 1000) + 25;         /* high precision temperature */
            ret = ADC_DMA_HALF_TRANSFER_FLAG;
            break;
        case 2: /* full transfer complete */
            adc2_dam_flag = 0;
            /* accumulate second half of buffer data */
            for(i = adc_count; i < ADC2_INTERNAL_CHANNEL_DATA_LENGTH; i++)
            {
                adc_value[0] += adc2_internal_channel_data[i][0];   /* VBAT channel */
                adc_value[1] += adc2_internal_channel_data[i][1];   /* temperature sensor */
                adc_value[2] += adc2_internal_channel_data[i][2];   /* internal Vref */
                adc_value[3] += adc2_internal_channel_data[i][3];   /* high precision temperature */
            }
            /* convert to physical units */
            adc2_data->vbat = (float)adc_value[0] / adc_count* 3.3 / 4095 * 4;  /* VBAT = ADC * 3.3V / 4095 * 4 (1/4 divider) */
            adc2_data->cpu_temperature = (float)(ADC_TEMP_CALIBRATION_VALUE_25 - (float)adc_value[1] / adc_count) * 3.3 / 4095 \
                                         / cpu_temperature_slope + 25;          /* temperature calculation with slope correction */
            adc2_data->internal_vref = (float)((float)adc_value[2] / adc_count) * 3.3 / 4095;   /* internal reference voltage */
            adc2_data->high_precision_cpu_temperature = (float)((float)adc_value[3] / adc_count - ADC_HIGH_TEMP_CALIBRATION_VALUE_25) \
                                                        *3.3 /4095 / (3.3 / 1000) + 25;         /* high precision temperature */
            ret = ADC_DMA_FULL_TRANSFER_FLAG;
            break;
        default: 
            /* no new data available */
            break;
    }

    return ret;
}

/*!
    \brief      print ADC2 internal channel data to USART
    \param[in]  none
    \param[out] none
    \retval     none
*/
void adc2_print_internal_channel_data(void)
{
    uint8_t res = 0;
    adc2_data_struct adc2_data;

    /* get latest ADC data from DMA buffer */
    res = adc2_get_internal_channel_data(&adc2_data);
    
    /* print data only when new measurements are available */
    if(res == ADC_DMA_HALF_TRANSFER_FLAG || res == ADC_DMA_FULL_TRANSFER_FLAG)
    {
        PRINT_INFO("print adc2 internal channel data information>>\r\n");
        PRINT_INFO("VBAT: %.3f V\r\n", adc2_data.vbat);                                    /* battery voltage with 3 decimal places */
        PRINT_INFO("CPU Temperature: %.2f °C\r\n", adc2_data.cpu_temperature);             /* CPU temperature with 2 decimal places */
        PRINT_INFO("Internal Vref: %.3f V\r\n", adc2_data.internal_vref);                  /* internal reference voltage */
        PRINT_INFO("High Precision CPU Temperature: %.2f °C\r\n", adc2_data.high_precision_cpu_temperature);  /* high precision temperature */
    }
}

/*!
    \brief      configure DMA for ADC2 internal channel data transfer
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void adc2_internal_channel_dma_config(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    /* enable DMA0 and DMAMUX clock */
    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMAMUX);
    
    /* reset DMA0 channel 7 */
    dma_deinit(DMA0, DMA_CH7);
    
    /* configure DMA0 channel 7 parameters */
	dma_init_struct.request 			= DMA_REQUEST_ADC2;                                 /* DMA request source */
    dma_init_struct.periph_addr 		= ADC2_RDATA;                                       /* peripheral data address: ADC2 data register */  
    dma_init_struct.memory0_addr 		= (uint32_t)adc2_internal_channel_data;             /* memory address: data buffer */  
    dma_init_struct.number 				= ADC2_INTERNAL_CHANNEL_DATA_LENGTH * 4;            /* transfer data size */   		    
    dma_init_struct.periph_inc 			= DMA_PERIPH_INCREASE_DISABLE;                      /* peripheral address increment */   
    dma_init_struct.memory_inc 			= DMA_MEMORY_INCREASE_ENABLE;                       /* memory address increment */   
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;                           /* data width: 16-bit */   
    dma_init_struct.direction 			= DMA_PERIPH_TO_MEMORY;                             /* transfer direction: peripheral to memory */    	
    dma_init_struct.priority 			= DMA_PRIORITY_HIGH;                                /* DMA priority: high */  
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_ENABLE;                         /* DMA mode: circular */
    dma_single_data_mode_init(DMA0, DMA_CH7, &dma_init_struct);
    
    /* enable DMA interrupts */
    dma_interrupt_enable(DMA0, DMA_CH7, DMA_INT_HTF);       /* half transfer interrupt */
    dma_interrupt_enable(DMA0, DMA_CH7, DMA_INT_FTF);       /* full transfer interrupt */
    
    /* configure NVIC for DMA interrupt */
    nvic_irq_enable(DMA0_Channel7_IRQn, 4, 0);
    
    /* enable DMA channel */
    dma_channel_enable(DMA0, DMA_CH7);
}

/*!
    \brief      DMA0 Channel 7 interrupt handler for ADC2 data transfer
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DMA0_Channel7_IRQHandler(void)
{
    /* check half transfer complete interrupt */
    if(dma_interrupt_flag_get(DMA0, DMA_CH7, DMA_INT_FLAG_HTF))
    {
        dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_HTF);
        adc2_dam_flag = ADC_DMA_HALF_TRANSFER_FLAG;       /* half transfer complete */
    }
    /* check full transfer complete interrupt */
    else if(dma_interrupt_flag_get(DMA0, DMA_CH7, DMA_INT_FLAG_FTF))
    {
        dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_FTF);
        adc2_dam_flag = ADC_DMA_FULL_TRANSFER_FLAG;       /* full transfer complete */
    }
}
