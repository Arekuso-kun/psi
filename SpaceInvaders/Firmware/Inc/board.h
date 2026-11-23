#ifndef SI_BOARD_H
#define SI_BOARD_H
#include <stdint.h>
void si_board_init(void);
uint32_t si_board_now(void);
uint8_t si_board_keys(void);
int si_board_read_byte(void);
uint8_t si_board_send(const char *line);
uint16_t *si_board_backbuffer(void);
void si_board_present(void);
void si_board_led(uint8_t pc_connected, uint8_t hurt);
uint32_t si_board_uart_errors(void);
void si_board_fail(unsigned code);
#endif
