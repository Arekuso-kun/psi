#include "board.h"
#include "board_config.h"
#include "input.h"
#include "stm32f429i_discovery_lcd.h"
#include <string.h>

extern LTDC_HandleTypeDef LtdcHandler;
static UART_HandleTypeDef uart;
static SPI_HandleTypeDef lcd_spi;
static volatile uint8_t rx[256], tx[1024];
static volatile uint16_t rx_head, rx_tail, tx_head, tx_tail;
static volatile uint32_t uart_errors;
static uint8_t back = 1;
static uint8_t debounced, candidate;
static uint32_t candidate_since;
static const uint32_t frames[2] = {0xD0000000u, 0xD0040000u};

static void clock_init(void) {
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM = 16;
    osc.PLL.PLLN = 360;
    osc.PLL.PLLP = RCC_PLLP_DIV2;
    osc.PLL.PLLQ = 8; /* USB is not used. */
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) si_board_fail(1);
    if (HAL_PWREx_EnableOverDrive() != HAL_OK) si_board_fail(1);
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) si_board_fail(1);
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

static void gpio_init(void) {
    GPIO_InitTypeDef p = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOG_CLK_ENABLE(); __HAL_RCC_GPIOE_CLK_ENABLE();
    p.Pin = GPIO_PIN_13 | GPIO_PIN_14;
    p.Mode = GPIO_MODE_OUTPUT_PP; p.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOG, &p);
    HAL_GPIO_WritePin(GPIOG, p.Pin, GPIO_PIN_RESET);
    p.Pin = GPIO_PIN_0; p.Mode = GPIO_MODE_INPUT; p.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &p);
    p.Pull = GPIO_PULLUP;
    p.Pin = SI_JOY_L_PIN; HAL_GPIO_Init(SI_JOY_L_PORT, &p);
    p.Pin = SI_JOY_R_PIN; HAL_GPIO_Init(SI_JOY_R_PORT, &p);
    p.Pin = SI_JOY_U_PIN; HAL_GPIO_Init(SI_JOY_U_PORT, &p);
    p.Pin = SI_JOY_D_PIN; HAL_GPIO_Init(SI_JOY_D_PORT, &p);
    p.Pin = SI_JOY_C_PIN; HAL_GPIO_Init(SI_JOY_C_PORT, &p);
}

static void uart_init(void) {
    GPIO_InitTypeDef p = {0};
    __HAL_RCC_USART1_CLK_ENABLE();
    p.Pin = GPIO_PIN_9 | GPIO_PIN_10; p.Mode = GPIO_MODE_AF_PP;
    p.Pull = GPIO_PULLUP; p.Speed = GPIO_SPEED_FREQ_VERY_HIGH; p.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &p);
    uart.Instance = USART1;
    uart.Init.BaudRate = SI_UART_BAUD;
    uart.Init.WordLength = UART_WORDLENGTH_8B;
    uart.Init.StopBits = UART_STOPBITS_1;
    uart.Init.Parity = UART_PARITY_NONE;
    uart.Init.Mode = UART_MODE_TX_RX;
    uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&uart) != HAL_OK) si_board_fail(2);
    USART1->CR1 |= USART_CR1_RXNEIE;
    HAL_NVIC_SetPriority(USART1_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void USART1_IRQHandler(void) {
    uint32_t status = USART1->SR;
    if (status & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)) {
        uint8_t byte = (uint8_t)USART1->DR; /* SR then DR clears UART error flags. */
        if (status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)) ++uart_errors;
        else {
            uint16_t next = (uint16_t)((rx_head + 1u) & 255u);
            if (next != rx_tail) { rx[rx_head] = byte; rx_head = next; }
            else ++uart_errors;
        }
    }
    if ((status & USART_SR_TXE) && (USART1->CR1 & USART_CR1_TXEIE)) {
        if (tx_tail != tx_head) {
            USART1->DR = tx[tx_tail]; tx_tail = (uint16_t)((tx_tail + 1u) & 1023u);
        } else USART1->CR1 &= ~USART_CR1_TXEIE;
    }
}

int si_board_read_byte(void) {
    int byte;
    if (rx_head == rx_tail) return -1;
    byte = rx[rx_tail]; rx_tail = (uint16_t)((rx_tail + 1u) & 255u);
    return byte;
}
uint8_t si_board_send(const char *line) {
    size_t len = strlen(line), i;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (len > (size_t)((tx_tail - tx_head - 1u) & 1023u)) {
        __set_PRIMASK(primask); return 0;
    }
    for (i = 0; i < len; ++i) { tx[tx_head] = (uint8_t)line[i]; tx_head = (uint16_t)((tx_head + 1u) & 1023u); }
    USART1->CR1 |= USART_CR1_TXEIE;
    __set_PRIMASK(primask);
    return 1;
}

static uint8_t raw_keys(void) {
    uint8_t m = 0;
    if (!HAL_GPIO_ReadPin(SI_JOY_L_PORT, SI_JOY_L_PIN)) m |= SI_LEFT;
    if (!HAL_GPIO_ReadPin(SI_JOY_R_PORT, SI_JOY_R_PIN)) m |= SI_RIGHT;
    if (!HAL_GPIO_ReadPin(SI_JOY_U_PORT, SI_JOY_U_PIN)) m |= SI_VOL_UP;
    if (!HAL_GPIO_ReadPin(SI_JOY_D_PORT, SI_JOY_D_PIN)) m |= SI_VOL_DOWN;
    if (!HAL_GPIO_ReadPin(SI_JOY_C_PORT, SI_JOY_C_PIN)) m |= SI_PAUSE;
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)) m |= SI_FIRE;
    return m;
}
uint8_t si_board_keys(void) {
    uint8_t raw = raw_keys(); uint32_t now = HAL_GetTick();
    if (raw != candidate) { candidate = raw; candidate_since = now; }
    if (now - candidate_since >= 20) debounced = candidate;
    return debounced;
}

static void display_init(void) {
    LTDC_LayerCfgTypeDef layer = {0};
    volatile uint32_t *test = (volatile uint32_t *)0xD0080000u;
    unsigned i;
    if (BSP_LCD_Init() != LCD_OK) si_board_fail(3);
    /* BSP initializes SDRAM. Test a separate, unused region before using buffers. */
    for (i = 0; i < 1024; ++i) test[i] = 0x5AA50000u ^ (i * 0x10203u);
    __DSB();
    for (i = 0; i < 1024; ++i) if (test[i] != (0x5AA50000u ^ (i * 0x10203u))) si_board_fail(4);
    memset((void *)frames[0], 0, SI_WIDTH * SI_HEIGHT * sizeof(uint16_t));
    memset((void *)frames[1], 0, SI_WIDTH * SI_HEIGHT * sizeof(uint16_t));
    layer.WindowX1 = SI_WIDTH; layer.WindowY1 = SI_HEIGHT;
    layer.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
    layer.FBStartAdress = frames[0]; layer.Alpha = 255; layer.Alpha0 = 0;
    layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    layer.ImageWidth = SI_WIDTH; layer.ImageHeight = SI_HEIGHT;
    if (HAL_LTDC_ConfigLayer(&LtdcHandler, &layer, 0) != HAL_OK) si_board_fail(3);
}

uint16_t *si_board_backbuffer(void) { return (uint16_t *)frames[back]; }
void si_board_present(void) {
    uint32_t began;
    __DSB();
    if (HAL_LTDC_SetAddress_NoReload(&LtdcHandler, frames[back], 0) != HAL_OK) si_board_fail(3);
    if (HAL_LTDC_Reload(&LtdcHandler, LTDC_RELOAD_VERTICAL_BLANKING) != HAL_OK) si_board_fail(3);
    began = HAL_GetTick();
    while (LTDC->SRCR & LTDC_SRCR_VBR) if (HAL_GetTick() - began > 40) si_board_fail(3);
    back ^= 1;
}

/* SPI5 is used only to configure the ILI9341. Pixel scanout uses LTDC RGB. */
void LCD_IO_Init(void) {
    GPIO_InitTypeDef p = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE(); __HAL_RCC_GPIOD_CLK_ENABLE(); __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_SPI5_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_SET); /* Gyro deselected too. */
    p.Pin = GPIO_PIN_1 | GPIO_PIN_2; p.Mode = GPIO_MODE_OUTPUT_PP;
    p.Speed = GPIO_SPEED_FREQ_VERY_HIGH; HAL_GPIO_Init(GPIOC, &p);
    p.Pin = GPIO_PIN_13; HAL_GPIO_Init(GPIOD, &p);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
    p.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    p.Mode = GPIO_MODE_AF_PP; p.Alternate = GPIO_AF5_SPI5;
    HAL_GPIO_Init(GPIOF, &p);
    lcd_spi.Instance = SPI5;
    lcd_spi.Init.Mode = SPI_MODE_MASTER; lcd_spi.Init.Direction = SPI_DIRECTION_2LINES;
    lcd_spi.Init.DataSize = SPI_DATASIZE_8BIT; lcd_spi.Init.CLKPolarity = SPI_POLARITY_LOW;
    lcd_spi.Init.CLKPhase = SPI_PHASE_1EDGE; lcd_spi.Init.NSS = SPI_NSS_SOFT;
    lcd_spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    lcd_spi.Init.FirstBit = SPI_FIRSTBIT_MSB; lcd_spi.Init.TIMode = SPI_TIMODE_DISABLE;
    lcd_spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE; lcd_spi.Init.CRCPolynomial = 7;
    if (HAL_SPI_Init(&lcd_spi) != HAL_OK) si_board_fail(3);
}
static void lcd_byte(uint8_t value, GPIO_PinState data) {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, data);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(&lcd_spi, &value, 1, 100) != HAL_OK) si_board_fail(3);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
}
void LCD_IO_WriteData(uint16_t value) { lcd_byte((uint8_t)value, GPIO_PIN_SET); }
void LCD_IO_WriteReg(uint8_t reg) { lcd_byte(reg, GPIO_PIN_RESET); }
uint32_t LCD_IO_ReadData(uint16_t reg, uint8_t size) { (void)reg; (void)size; return 0; }
void LCD_Delay(uint32_t delay) { HAL_Delay(delay); }
void DMA2_Stream0_IRQHandler(void) { BSP_SDRAM_DMA_IRQHandler(); }
void SysTick_Handler(void) { HAL_IncTick(); }
void HAL_MspInit(void) { __HAL_RCC_SYSCFG_CLK_ENABLE(); __HAL_RCC_PWR_CLK_ENABLE(); HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4); }
uint32_t si_board_now(void) { return HAL_GetTick(); }
uint32_t si_board_uart_errors(void) { return uart_errors; }
void si_board_led(uint8_t pc, uint8_t hurt) {
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, pc ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, hurt ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
void si_board_init(void) { HAL_Init(); clock_init(); gpio_init(); uart_init(); display_init(); }
void si_board_fail(unsigned code) {
    unsigned i;
    __HAL_RCC_GPIOG_CLK_ENABLE();
    {
        GPIO_InitTypeDef p = {0}; p.Pin = GPIO_PIN_14; p.Mode = GPIO_MODE_OUTPUT_PP;
        HAL_GPIO_Init(GPIOG, &p);
    }
    for (;;) {
        for (i = 0; i < code; ++i) {
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET); HAL_Delay(180);
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET); HAL_Delay(180);
        }
        HAL_Delay(1000);
    }
}
void HardFault_Handler(void) { for (;;) { } }
void MemManage_Handler(void) { for (;;) { } }
void BusFault_Handler(void) { for (;;) { } }
void UsageFault_Handler(void) { for (;;) { } }
