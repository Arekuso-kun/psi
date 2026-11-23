#ifndef MC_BOARD_H
#define MC_BOARD_H
#include <stdint.h>
void mc_board_init(void);
uint32_t mc_board_now(void);
uint8_t mc_board_keys(void);
int mc_board_read_byte(void);
uint8_t mc_board_send(const char *line);
uint16_t *mc_board_backbuffer(void);
void mc_board_present(void);
void mc_board_led(uint8_t pc_connected, uint8_t error);
uint32_t mc_board_uart_errors(void);
void mc_board_fail(unsigned code);
#endif
