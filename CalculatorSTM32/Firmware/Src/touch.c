#include "touch.h"
#include "board_config.h"
#include "stmpe811.h"
#include <string.h>

static I2C_HandleTypeDef bus;
static uint8_t initialized,available,bus_error,held,candidate,have_sample;
static uint8_t driver_initialized;
static uint16_t last_x,last_y;
static uint32_t changed_at;
#define TOUCH_ADDRESS 0x82u /* ST HAL uses the shifted 7-bit address. */

void IOE_Init(void) {
    GPIO_InitTypeDef p={0};
    if(initialized)return;
    __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOC_CLK_ENABLE();__HAL_RCC_I2C3_CLK_ENABLE();
    p.Mode=GPIO_MODE_AF_OD;p.Pull=GPIO_PULLUP;p.Speed=GPIO_SPEED_FREQ_VERY_HIGH;p.Alternate=GPIO_AF4_I2C3;
    p.Pin=GPIO_PIN_8;HAL_GPIO_Init(GPIOA,&p);p.Pin=GPIO_PIN_9;HAL_GPIO_Init(GPIOC,&p);
    bus.Instance=I2C3;bus.Init.ClockSpeed=100000;bus.Init.DutyCycle=I2C_DUTYCYCLE_2;
    bus.Init.OwnAddress1=0;bus.Init.AddressingMode=I2C_ADDRESSINGMODE_7BIT;
    bus.Init.DualAddressMode=I2C_DUALADDRESS_DISABLE;bus.Init.OwnAddress2=0;
    bus.Init.GeneralCallMode=I2C_GENERALCALL_DISABLE;bus.Init.NoStretchMode=I2C_NOSTRETCH_DISABLE;
    if(HAL_I2C_Init(&bus)!=HAL_OK){bus_error=1;return;}
    initialized=1;
}
void IOE_ITConfig(void) { /* Polling TSC_CTRL avoids sharing an EXTI input. */ }
void IOE_Delay(uint32_t delay){HAL_Delay(delay);}
void IOE_Write(uint8_t addr,uint8_t reg,uint8_t value) {
    if(!initialized || HAL_I2C_Mem_Write(&bus,addr,reg,I2C_MEMADD_SIZE_8BIT,&value,1,3)!=HAL_OK)bus_error=1;
}
uint8_t IOE_Read(uint8_t addr,uint8_t reg) {
    uint8_t value=0;
    if(!initialized || HAL_I2C_Mem_Read(&bus,addr,reg,I2C_MEMADD_SIZE_8BIT,&value,1,3)!=HAL_OK)bus_error=1;
    return value;
}
uint16_t IOE_ReadMultiple(uint8_t addr,uint8_t reg,uint8_t *data,uint16_t length) {
    if(!initialized || HAL_I2C_Mem_Read(&bus,addr,reg,I2C_MEMADD_SIZE_8BIT,data,length,3)!=HAL_OK){
        memset(data,0,length);bus_error=1;return 0;
    }
    return length;
}
int mc_touch_init(void) {
    if(initialized){HAL_I2C_DeInit(&bus);initialized=0;}
    bus_error=0;IOE_Init();
    available=0;
    if(bus_error || stmpe811_ReadID(TOUCH_ADDRESS)!=STMPE811_ID)return 0;
    if(driver_initialized)stmpe811_Reset(TOUCH_ADDRESS);
    else {stmpe811_Init(TOUCH_ADDRESS);driver_initialized=1;}
    stmpe811_TS_Start(TOUCH_ADDRESS);
    if(!bus_error)available=1;
    held=candidate=have_sample=0;
    return available;
}
int mc_touch_available(void){return available;}
int mc_touch_poll(uint32_t now,McTouch *out) {
    uint8_t down;
    out->down=0;out->x=last_x;out->y=last_y;
    if(!available)return 0;
    bus_error=0;
    down=(IOE_Read(TOUCH_ADDRESS,STMPE811_REG_TSC_CTRL)&STMPE811_TS_CTRL_STATUS)!=0;
    if(down && IOE_Read(TOUCH_ADDRESS,STMPE811_REG_FIFO_SIZE)>0){
        uint16_t x=0,y=0;
        stmpe811_TS_GetXY(TOUCH_ADDRESS,&x,&y);
        if(!bus_error){last_x=x;last_y=y;have_sample=1;}
    }
    if(bus_error){available=0;held=0;return 0;}
    /* Physical contact status is distinct from FIFO-empty: no repeated keys while held. */
    if(down!=candidate){candidate=down;changed_at=now;}
    if(now-changed_at>=(down?20u:35u))held=(uint8_t)(down && have_sample);
    if(!down && !held)have_sample=0;
    out->down=held;out->x=last_x;out->y=last_y;return 1;
}
