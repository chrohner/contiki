/*---------------------------------------------------------------------------*/
/**
 * \addtogroup sensortag-cc26xx-sportident-srr
 * @{
 *
 * \file
 *  Driver for Sportident SRR
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "ti-lib.h"
#include "board-spi.h"

#include "board-usb.h"
#include "leds.h"

// FT1248 commands
#define USB_CMD_WRITE                   0x00
#define USB_CMD_READ                    0x01
#define USB_CMD_READ_MODEM_STATUS       0x02
#define USB_CMD_WRITE_MODEM_STATUS      0x03
#define USB_CMD_WRITE_BUFFER_FLUSH      0x04
#define USB_CMD_ADDRESS_EEPROM          0x05
#define USB_CMD_WRITE_EEPROM            0x06
#define USB_CMD_READ_EEPROM             0x07
#define USB_CMD_READ_USB_STATUS         0x08


/*---------------------------------------------------------------------------*/
/**
 * Clear external flash CSN line
 */
static void
select_on_bus(void)
{
  ti_lib_gpio_pin_write(BOARD_USB_CS, 0);
}
/*---------------------------------------------------------------------------*/
/**
 * Set external flash CSN line
 */
static void
deselect(void)
{
  ti_lib_gpio_pin_write(BOARD_USB_CS, 1);
}
/*---------------------------------------------------------------------------*/
bool
usb_open()
{
  board_spi_open(6000000, BOARD_IOID_SPI_CLK);

  /* GPIO pin configuration */
  ti_lib_ioc_pin_type_gpio_output(BOARD_IOID_SPI_CC2500_1_CS);

  /* Default output to clear chip select */
  deselect();

  return true;
}
/*---------------------------------------------------------------------------*/
void
usb_close()
{
  /* Put the part in low power mode */
  // power_down();

  board_spi_close();
}
/*---------------------------------------------------------------------------*/
bool
usb_read(uint8_t *buf)
{
    return false;
}
/*---------------------------------------------------------------------------*/
bool
usb_write(const uint8_t buf)
{
    select_on_bus();

    board_spi_write(USB_CMD_WRITE, 1);
    
    if (board_spi_write(&buf, 1) == false) {
        /* failure */
        deselect();
        return false;
    };
    
    deselect();

  return true;
}
/*---------------------------------------------------------------------------*/
/** @} */
