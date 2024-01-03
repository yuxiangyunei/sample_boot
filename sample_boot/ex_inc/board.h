#ifndef BOARD_H_
#define BOARD_H_
#include <stdbool.h>
#include "hwio.h"

typedef enum
{
    LED_NULL = 0x00,
    LED_GREEN = 0x01,
    LED_RED = 0x02,
    LED_ORANGE = 0x03,
    LED_GREEN_TRIG = 0x04,
    LED_RED_TRIG = 0x05,
    LED_ORANGE_TRIG = 0x06,
} vci_led_mode_t;

void board_hw_init(void);
unsigned char id_pin_read(void);
void flexray_set_transciever_mode(unsigned char channel, bool power_enable, bool trans_enable);
void can_set_transciever_mode(unsigned char channel, bool power_enable, bool trans_enable, bool stbn_enable);
unsigned char hw_rev_pin_read(void);
void hw_set_gpo_output(uint32_t gpo_output_mask);
void hw_set_ext_wup_enable(uint32_t wup_en_mask);
void hw_power_on(void);
void hw_power_off(void);

void can_set_led_status(unsigned char channel, uint8_t status);
void hw_set_led_status(uint8_t status);
void lin_set_led_status(unsigned char channel, uint8_t status);
void flexray_set_led_status(unsigned char channel, uint8_t status);

#endif /* BOARD_H_ */
