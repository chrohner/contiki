#ifndef __BOARD_USB_H__
#define __BOARD_USB_H__
/*---------------------------------------------------------------------------*/
bool usb_read(uint8_t *buf);
bool usb_write(const uint8_t buf);

bool usb_open();
void usb_close();
/*---------------------------------------------------------------------------*/
/** @} */
#endif