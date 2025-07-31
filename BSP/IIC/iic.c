/*!
    \file       iic.c
    \brief      Software I2C driver with multi-device support
    \version    1.0
    \date       2025-07-24
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./IIC/iic.h"
#include "./DELAY/delay.h"
#include <stdbool.h>

/* I2C device instance 1 configuration */
iicDeviceStruct iicDevice_1={
    RCU_GPIOD,      /* SCL clock source */
    GPIOD,          /* SCL port */
    GPIO_PIN_12,    /* SCL pin */
    RCU_GPIOD,      /* SDA clock source */
    GPIOD,          /* SDA port */
    GPIO_PIN_13,    /* SDA pin */
    false,          /* initialization state */
};

/* I2C device instance 2 configuration */
iicDeviceStruct iicDevice_2={
    RCU_GPIOE,      /* SCL clock source */
    GPIOE,          /* SCL port */
    GPIO_PIN_2,     /* SCL pin */
    RCU_GPIOE,      /* SDA clock source */
    GPIOE,          /* SDA port */
    GPIO_PIN_3,     /* SDA pin */
    false,          /* initialization state */
};

/*!
    \brief      set SCL line state
    \param[in]  x: line state (0 or 1)
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     none
*/
static void IIC_SCL(uint8_t x, iicDeviceStruct *iicDev)
{
    x ? gpio_bit_write(iicDev->gpio_periph_scl, iicDev->pin_scl, SET) : \
      gpio_bit_write(iicDev->gpio_periph_scl, iicDev->pin_scl, RESET);
}

/*!
    \brief      set SDA line state
    \param[in]  x: line state (0 or 1)
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     none
*/
static void IIC_SDA(uint8_t x, iicDeviceStruct *iicDev)
{
    x ? gpio_bit_write(iicDev->gpio_periph_sda, iicDev->pin_sda, SET) : \
      gpio_bit_write(iicDev->gpio_periph_sda, iicDev->pin_sda, RESET);
}

/*!
    \brief      read SDA line state
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     SDA line state (0 or 1)
*/
static uint8_t IIC_READ_SDA(iicDeviceStruct *iicDev)
{
    return gpio_input_bit_get(iicDev->gpio_periph_sda, iicDev->pin_sda);
}

/*!
    \brief      initialize I2C device GPIO configuration
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     none
*/
void iic_init(iicDeviceStruct *iicDev)
{
    /* enable GPIO clock for SCL and SDA pins */
    rcu_periph_clock_enable(iicDev->periph_scl);
    rcu_periph_clock_enable(iicDev->periph_sda);

    /* initialize SCL pin */
    gpio_mode_set(iicDev->gpio_periph_scl,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,iicDev->pin_scl);
    gpio_output_options_set(iicDev->gpio_periph_scl, GPIO_OTYPE_OD, GPIO_OSPEED_12MHZ, iicDev->pin_scl);/* SCL pin: output, open-drain, 12MHz */

    /* initialize SDA pin */
    gpio_mode_set(iicDev->gpio_periph_sda,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,iicDev->pin_sda);
    gpio_output_options_set(iicDev->gpio_periph_sda, GPIO_OTYPE_OD, GPIO_OSPEED_12MHZ, iicDev->pin_sda);/* SDA pin: output, open-drain, 12MHz */

    iic_stop(iicDev);            /* send stop signal to release I2C bus */
    
    iicDev->iic_state = true;    /* mark I2C device as initialized */
}

/*!
    \brief      I2C timing delay function to control communication speed
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     none
*/
static void iic_delay(iicDeviceStruct *iicDev)
{
    if(iicDev == &iicDevice_1)
    {
        delay_us(1);    /* 1us delay, communication speed ~500KHz */
    }
    else if(iicDev == &iicDevice_2)
    {
        delay_us(1);    /* 1us delay, communication speed ~500KHz */
    }
    else
    {
        delay_us(1);    /* 1us delay, communication speed ~500KHz */
    }
}

/*!
    \brief      generate I2C start signal
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     none
*/
void iic_start(iicDeviceStruct *iicDev)
{
    IIC_SDA(1, iicDev);
    IIC_SCL(1, iicDev);
    iic_delay(iicDev);
    IIC_SDA(0, iicDev);     /* START signal: when SCL is high, SDA transitions from high to low */
    iic_delay(iicDev);
    IIC_SCL(0, iicDev);     /* clamp I2C bus, prepare to send or receive data */
    iic_delay(iicDev);
}

/*!
    \brief      generate I2C stop signal
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     none
*/
void iic_stop(iicDeviceStruct *iicDev)
{
    IIC_SDA(0, iicDev);     /* STOP signal: when SCL is high, SDA transitions from low to high */
    iic_delay(iicDev);
    IIC_SCL(1, iicDev);
    iic_delay(iicDev);
    IIC_SDA(1, iicDev);     /* release I2C bus control signal */
    iic_delay(iicDev);
}

/*!
    \brief      wait for ACK response signal
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     0: ACK received successfully
                1: ACK receiving failed
*/
uint8_t iic_wait_ack(iicDeviceStruct *iicDev)
{
    uint8_t waittime = 0;
    uint8_t rack = 0;

    IIC_SDA(1, iicDev);     /* release SDA line (let external device control SDA line) */
    iic_delay(iicDev);
    IIC_SCL(1, iicDev);     /* SCL=1, clock high allows external device to send ACK */
    iic_delay(iicDev);

    while(IIC_READ_SDA(iicDev))    /* wait for ACK */
    {
        waittime++;

        if(waittime > 250)
        {
            iic_stop(iicDev);
            rack = 1;
            break;
        }
    }

    IIC_SCL(0, iicDev);     /* SCL=0, end ACK checking */
    iic_delay(iicDev);
    return rack;
}

/*!
    \brief      send ACK response signal
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     none
*/
void iic_ack(iicDeviceStruct *iicDev)
{
    IIC_SDA(0, iicDev);     /* SCL 0 -> 1 when SDA = 0, indicates ACK */
    iic_delay(iicDev);
    IIC_SCL(1, iicDev);     /* generate one clock pulse */
    iic_delay(iicDev);
    IIC_SCL(0, iicDev);
    iic_delay(iicDev);
    IIC_SDA(1, iicDev);     /* release SDA line */
    iic_delay(iicDev);
}

/*!
    \brief      send NACK response signal
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     none
*/
void iic_nack(iicDeviceStruct *iicDev)
{
    IIC_SDA(1, iicDev);     /* SCL 0 -> 1 when SDA = 1, indicates NACK */
    iic_delay(iicDev);
    IIC_SCL(1, iicDev);     /* generate one clock pulse */
    iic_delay(iicDev);
    IIC_SCL(0, iicDev);
    iic_delay(iicDev);
}

/*!
    \brief      send one byte via I2C
    \param[in]  data: data to be transmitted
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     none
*/
void iic_send_byte(uint8_t data, iicDeviceStruct *iicDev)
{
    uint8_t t;
    
    for (t = 0; t < 8; t++)
    {
        IIC_SDA((data & 0x80) >> 7, iicDev);    /* send MSB first */
        iic_delay(iicDev);
        IIC_SCL(1, iicDev);
        iic_delay(iicDev);
        IIC_SCL(0, iicDev);
        data <<= 1;             /* shift left 1 bit for next transmission */
    }
    IIC_SDA(1, iicDev);         /* after transmission, release SDA line */
}

/*!
    \brief      receive one byte via I2C
    \param[in]  ack: ack=1 sends ACK; ack=0 sends NACK
    \param[in]  iicDev: pointer to I2C device structure
    \param[out] none
    \retval     received data byte
*/
uint8_t iic_read_byte(uint8_t ack, iicDeviceStruct *iicDev)
{
    uint8_t i, receive = 0;

    for (i = 0; i < 8; i++ )    /* receive 1 byte of data */
    {
        receive <<= 1;  /* shift left for high bits, received data MSB should be shifted */
        IIC_SCL(1, iicDev);
        iic_delay(iicDev);

        if (IIC_READ_SDA(iicDev))
        {
            receive++;
        }
        
        IIC_SCL(0, iicDev);
        iic_delay(iicDev);
    }

    if (!ack)
    {
        iic_nack(iicDev);     /* send NACK */
    }
    else
    {
        iic_ack(iicDev);      /* send ACK */
    }

    return receive;
}
