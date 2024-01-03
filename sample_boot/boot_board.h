/*
 * hal.h
 *
 *  Created on: 2019��2��14��
 *      Author: Fred
 */

#ifndef BOARD_H_
#define BOARD_H_
#include <stdbool.h>

void board_hw_init(void);
unsigned char id_pin_read(void);
void can_set_transciever_mode(unsigned char channel, bool power_enable, bool trans_enable, bool stbn_enable);
unsigned char hw_rev_pin_read(void);

#endif /* BOARD_H_ */
