#include "audio.h"
#include "board_config.h"
#include "board.h"

#if MC_AUDIO_ENABLED
static DAC_HandleTypeDef dac;
static DMA_HandleTypeDef dma;
static TIM_HandleTypeDef timer;
static uint16_t buffer[256]; /* Internal SRAM, accessible to DMA1. */
static volatile uint32_t increment, amplitude;
static uint32_t phase, began;
static uint8_t effect;
static const int16_t sine[32] = {0,195,383,556,707,831,924,981,1000,981,924,831,707,556,383,195,
                               0,-195,-383,-556,-707,-831,-924,-981,-1000,-981,-924,-831,-707,-556,-383,-195};
static void fill(unsigned offset) {
    unsigned i;
    uint32_t step = increment, peak = amplitude;
    for (i=offset;i<offset+128;++i) {
        phase += step;
        buffer[i] = (uint16_t)(2048 + (sine[(phase >> 16) & 31u] * (int32_t)peak) / 1000);
    }
}
void HAL_DACEx_ConvHalfCpltCallbackCh2(DAC_HandleTypeDef *h) { (void)h; fill(0); }
void HAL_DACEx_ConvCpltCallbackCh2(DAC_HandleTypeDef *h) { (void)h; fill(128); }
void HAL_DACEx_ErrorCallbackCh2(DAC_HandleTypeDef *h) { (void)h; amplitude = increment = 0; }
void HAL_DACEx_DMAUnderrunCallbackCh2(DAC_HandleTypeDef *h) { (void)h; amplitude = increment = 0; }
void mc_audio_irq(void) { HAL_DMA_IRQHandler(&dma); }
void DMA1_Stream6_IRQHandler(void) { mc_audio_irq(); }
void TIM6_DAC_IRQHandler(void) { HAL_DAC_IRQHandler(&dac); }
void mc_audio_init(void) {
    GPIO_InitTypeDef p = {0};
    DAC_ChannelConfTypeDef ch = {0};
    TIM_MasterConfigTypeDef master = {0};
    unsigned i;
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_DAC_CLK_ENABLE(); __HAL_RCC_DMA1_CLK_ENABLE(); __HAL_RCC_TIM6_CLK_ENABLE();
    p.Pin=GPIO_PIN_5; p.Mode=GPIO_MODE_ANALOG; p.Pull=GPIO_NOPULL; HAL_GPIO_Init(GPIOA,&p);
    dac.Instance = DAC;
    if (HAL_DAC_Init(&dac) != HAL_OK) mc_board_fail(5);
    ch.DAC_Trigger = DAC_TRIGGER_T6_TRGO; ch.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    if (HAL_DAC_ConfigChannel(&dac,&ch,DAC_CHANNEL_2) != HAL_OK) mc_board_fail(5);
    dma.Instance=DMA1_Stream6; dma.Init.Channel=DMA_CHANNEL_7;
    dma.Init.Direction=DMA_MEMORY_TO_PERIPH; dma.Init.PeriphInc=DMA_PINC_DISABLE;
    dma.Init.MemInc=DMA_MINC_ENABLE; dma.Init.PeriphDataAlignment=DMA_PDATAALIGN_HALFWORD;
    dma.Init.MemDataAlignment=DMA_MDATAALIGN_HALFWORD; dma.Init.Mode=DMA_CIRCULAR;
    dma.Init.Priority=DMA_PRIORITY_HIGH; dma.Init.FIFOMode=DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&dma) != HAL_OK) mc_board_fail(5);
    __HAL_LINKDMA(&dac,DMA_Handle2,dma);
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn,2,0); HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn,2,0); HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    timer.Instance=TIM6; timer.Init.Prescaler=0;
    timer.Init.CounterMode=TIM_COUNTERMODE_UP; timer.Init.Period=90000000u/MC_AUDIO_RATE-1u;
    timer.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&timer) != HAL_OK) mc_board_fail(5);
    master.MasterOutputTrigger=TIM_TRGO_UPDATE; master.MasterSlaveMode=TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&timer,&master) != HAL_OK) mc_board_fail(5);
    for(i=0;i<256;++i) buffer[i]=2048;
    if (HAL_DAC_Start_DMA(&dac,DAC_CHANNEL_2,(uint32_t *)buffer,256,DAC_ALIGN_12B_R) != HAL_OK) mc_board_fail(5);
    if (HAL_TIM_Base_Start(&timer) != HAL_OK) mc_board_fail(5);
}
void mc_audio_update(uint32_t now,uint8_t click,uint8_t error,uint8_t volume) {
    uint32_t duration;
    if(click){effect=error?2:1;began=now;}
    duration=effect==2?90u:25u;
    if(now-began>=duration)effect=0;
    increment=effect?(effect==2?330u:1100u)*(32u*65536u)/MC_AUDIO_RATE:0;
    amplitude=effect?((uint32_t)volume*MC_AUDIO_PEAK/100u):0;
}

#else
void mc_audio_init(void) { }
void mc_audio_update(uint32_t now,uint8_t click,uint8_t error,uint8_t volume) { (void)now;(void)click;(void)error;(void)volume; }
void mc_audio_irq(void) { }
#endif
