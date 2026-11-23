#include "audio.h"
#include "board_config.h"
#include "board.h"
#include "game.h"

#if SI_AUDIO_ENABLED
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
void si_audio_irq(void) { HAL_DMA_IRQHandler(&dma); }
void DMA1_Stream6_IRQHandler(void) { si_audio_irq(); }
void TIM6_DAC_IRQHandler(void) { HAL_DAC_IRQHandler(&dac); }
void si_audio_init(void) {
    GPIO_InitTypeDef p = {0};
    DAC_ChannelConfTypeDef ch = {0};
    TIM_MasterConfigTypeDef master = {0};
    unsigned i;
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_DAC_CLK_ENABLE(); __HAL_RCC_DMA1_CLK_ENABLE(); __HAL_RCC_TIM6_CLK_ENABLE();
    p.Pin=GPIO_PIN_5; p.Mode=GPIO_MODE_ANALOG; p.Pull=GPIO_NOPULL; HAL_GPIO_Init(GPIOA,&p);
    dac.Instance = DAC;
    if (HAL_DAC_Init(&dac) != HAL_OK) si_board_fail(5);
    ch.DAC_Trigger = DAC_TRIGGER_T6_TRGO; ch.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    if (HAL_DAC_ConfigChannel(&dac,&ch,DAC_CHANNEL_2) != HAL_OK) si_board_fail(5);
    dma.Instance=DMA1_Stream6; dma.Init.Channel=DMA_CHANNEL_7;
    dma.Init.Direction=DMA_MEMORY_TO_PERIPH; dma.Init.PeriphInc=DMA_PINC_DISABLE;
    dma.Init.MemInc=DMA_MINC_ENABLE; dma.Init.PeriphDataAlignment=DMA_PDATAALIGN_HALFWORD;
    dma.Init.MemDataAlignment=DMA_MDATAALIGN_HALFWORD; dma.Init.Mode=DMA_CIRCULAR;
    dma.Init.Priority=DMA_PRIORITY_HIGH; dma.Init.FIFOMode=DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&dma) != HAL_OK) si_board_fail(5);
    __HAL_LINKDMA(&dac,DMA_Handle2,dma);
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn,2,0); HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn,2,0); HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    timer.Instance=TIM6; timer.Init.Prescaler=0;
    timer.Init.CounterMode=TIM_COUNTERMODE_UP; timer.Init.Period=90000000u/SI_AUDIO_RATE-1u;
    timer.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&timer) != HAL_OK) si_board_fail(5);
    master.MasterOutputTrigger=TIM_TRGO_UPDATE; master.MasterSlaveMode=TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&timer,&master) != HAL_OK) si_board_fail(5);
    for(i=0;i<256;++i) buffer[i]=2048;
    if (HAL_DAC_Start_DMA(&dac,DAC_CHANNEL_2,(uint32_t *)buffer,256,DAC_ALIGN_12B_R) != HAL_OK) si_board_fail(5);
    if (HAL_TIM_Base_Start(&timer) != HAL_OK) si_board_fail(5);
}
void si_audio_update(uint32_t now,uint8_t sounds,uint8_t volume,uint8_t paused) {
    uint8_t next = sounds & SI_END ? 5 : sounds & SI_HURT ? 4 : sounds & SI_NEXT_WAVE ? 3 : sounds & SI_HIT ? 2 : sounds & SI_SHOT ? 1 : 0;
    uint32_t t, frequency=0, duration=0;
    if(paused) { effect=0; amplitude=increment=0; return; }
    if(next && (!effect || next>=effect)) { effect=next; began=now; }
    t=now-began;
    switch(effect) {
        case 1: duration=65; frequency=1500u-(t<65?t*15u:0); break;
        case 2: duration=110; frequency=t<55?900:600; break;
        case 3: duration=300; frequency=t<100?523:t<200?659:784; break;
        case 4: duration=240; frequency=t<80?220:t<160?165:110; break;
        case 5: duration=600; frequency=t<200?392:t<400?294:196; break;
        default: break;
    }
    if(t>=duration) { effect=0; frequency=0; }
    increment=frequency*(32u*65536u)/SI_AUDIO_RATE;
    amplitude=frequency?((uint32_t)volume*SI_AUDIO_PEAK/100u):0;
}
#else
void si_audio_init(void) { }
void si_audio_update(uint32_t now,uint8_t sounds,uint8_t volume,uint8_t paused) { (void)now;(void)sounds;(void)volume;(void)paused; }
void si_audio_irq(void) { }
#endif
