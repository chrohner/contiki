#ifndef __BOARD_USB_H__
#define __BOARD_USB_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
bool usb_read(uint8_t *buf);
bool usb_write(const uint8_t* buf, uint8_t len);

bool usb_open();
void usb_close();
/*---------------------------------------------------------------------------*/
/** @} */
#endif