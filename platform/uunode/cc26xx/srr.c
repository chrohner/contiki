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
#include "gpio-interrupt.h"

#include "srr.h"
#include "srr-const.h"
#include "leds.h"

/*---------------------------------------------------------------------------*/
#define GDOx_GPIO_CFG          (IOC_CURRENT_2MA  | IOC_STRENGTH_AUTO | \
                                IOC_IOPULL_UP    | IOC_SLEW_DISABLE  | \
                                IOC_HYST_DISABLE | IOC_BOTH_EDGES    | \
                                IOC_INT_ENABLE   | IOC_IOMODE_NORMAL | \
                                IOC_NO_WAKE_UP   | IOC_INPUT_ENABLE)

/*---------------------------------------------------------------------------*/
/**
 * interrupt handler
 */
static void
int_handler(uint8_t ioid)
{
    if (ioid == BOARD_IOID_SPI_CC2500_1_GDO0) {
        leds_toggle(LEDS_RED);
        if (ti_lib_gpio_pin_read(BOARD_SRR1_GDO0) == 1) {
        }
    }
}


/*---------------------------------------------------------------------------*/
/**
 * Clear external flash CSN line
 */
static void
select_on_bus(void)
{
  ti_lib_gpio_pin_write(BOARD_SRR1_CS, 0);
}
/*---------------------------------------------------------------------------*/
/**
 * Set external flash CSN line
 */
static void
deselect(void)
{
  ti_lib_gpio_pin_write(BOARD_SRR1_CS, 1);
}
/*---------------------------------------------------------------------------*/
bool
srr_open()
{
  board_spi_open(1000000, BOARD_IOID_SPI_CLK);

  /* GPIO pin configuration */
  ti_lib_ioc_pin_type_gpio_output(BOARD_IOID_SPI_CC2500_1_CS);

  /* Default output to clear chip select */
  deselect();

  return true;
}
/*---------------------------------------------------------------------------*/
void
srr_close()
{
  /* Put the part in low power mode */
  // power_down();

  board_spi_close();
}
/*---------------------------------------------------------------------------*/
bool
srr_read(const uint8_t addr, uint8_t *buf)
{
    bool ret;
    select_on_bus();

    board_spi_write(&addr, 1);
    clock_delay_usec(1);
    ret = board_spi_read(buf, 1);
    clock_delay_usec(1);

    deselect();

    return ret;
}
/*---------------------------------------------------------------------------*/
bool
srr_write(const uint8_t addr, const uint8_t buf)
{
    select_on_bus();

    if (board_spi_write(&addr, 1) == false) {
        /* failure */
        deselect();
        return false;
    };
    clock_delay_usec(1);

    if (board_spi_write(&buf, 1) == false) {
      /* failure */
      deselect();
      return false;
    }
    clock_delay_usec(1);
    
    deselect();

  return true;
}
/*---------------------------------------------------------------------------*/
/**
 * Configure CC2500 for Sportident SRR
 */
bool
srr_cmd(uint8_t cmd) {
    select_on_bus();
    
    if (board_spi_write(&cmd, 1) == false) {
        /* failure */
        deselect();
        return false;
    };
    clock_delay_usec(1);
    
    deselect();
    
    return true;
}
/*---------------------------------------------------------------------------*/
/**
 * Reset CC2500 (section 19.1.2)
 */
void
srr_reset(void) {
    
    uint8_t res = CC2500_SRES;
    
    // switch off SPI  (open it again after reset!)
    srr_close();
    
    // configure IO to set the reset signals
    ti_lib_ioc_pin_type_gpio_output(BOARD_IOID_SPI_MOSI);
    ti_lib_ioc_pin_type_gpio_input(BOARD_IOID_SPI_MISO);
    ti_lib_ioc_pin_type_gpio_output(BOARD_IOID_SPI_CLK);
    ti_lib_ioc_pin_type_gpio_output(BOARD_IOID_SPI_CC2500_1_CS);
    
    // SCLK=1, SI=0
    ti_lib_gpio_pin_write(1 << BOARD_IOID_SPI_CLK, 1);
    ti_lib_gpio_pin_write(1 << BOARD_IOID_SPI_MOSI, 0);
    // strobe CSn
    clock_delay_usec(10);
    ti_lib_gpio_pin_write(BOARD_SRR1_CS, 0);
    clock_delay_usec(10);
    ti_lib_gpio_pin_write(BOARD_SRR1_CS, 1);

    // wait 40us
    clock_delay_usec(40);
    // pull CSn low
    ti_lib_gpio_pin_write(BOARD_SRR1_CS, 0);
    // wait for SO to go low
    while (ti_lib_gpio_pin_read(1 << BOARD_IOID_SPI_MISO) == 1) {
        clock_delay_usec(1);
    }; // TODO: don't wait forever!

    // SRES strobe
    board_spi_open(1000000, BOARD_IOID_SPI_CLK);
    board_spi_write(&res, 1);
    board_spi_close();
    
    // release CSn
    ti_lib_gpio_pin_write(BOARD_SRR1_CS, 1);

    // wait for SO to go low
    clock_delay_usec(10000);
    ti_lib_ioc_pin_type_gpio_input(BOARD_IOID_SPI_MISO);
    while (ti_lib_gpio_pin_read(1 << BOARD_IOID_SPI_MISO) == 1) {
        clock_delay_usec(1);
    }; // TODO: don't wait forever!

}
/*---------------------------------------------------------------------------*/
/**
 * Configure CC2500 register and GDOx for Sportident SRR
 */
void
srr_config(void) {

    uint8_t buf = 0x00;
    bool ret;
    static uint8_t i = 0;
    
    leds_on(LEDS_RED);
    leds_on(LEDS_GREEN);
    
    srr_open();

    while (i < sizeof(cc2500_srr_config)-1) {
        // set configuration
        srr_write(cc2500_srr_config[i], cc2500_srr_config[i+1]);
        clock_delay_usec(100); // not necessary
        i += 2;
    }
    
    // read register to verify
    ret = srr_read(CC2500_READ | CC2500_CHANNR, &buf);
    
    if (ret) {
        leds_off(LEDS_GREEN);
    }
    if (buf == SRR_CHANNEL_BLUE) {
        leds_off(LEDS_RED);
    }
    
    srr_close();


    // configure callbacks (GDO0)
    ti_lib_gpio_event_clear(BOARD_SRR1_GDO0);
    ti_lib_ioc_port_configure_set(BOARD_IOID_SPI_CC2500_1_GDO0, IOC_PORT_GPIO, GDOx_GPIO_CFG);
    ti_lib_gpio_dir_mode_set(BOARD_SRR1_GDO0, GPIO_DIR_MODE_IN);
    gpio_interrupt_register_handler(BOARD_IOID_SPI_CC2500_1_GDO0, int_handler);
    ti_lib_ioc_int_enable(BOARD_IOID_SPI_CC2500_1_GDO0);
}

/*---------------------------------------------------------------------------*/
void
srr_start() {
    /*
     cmd(CC2500_SIDLE);
     cmd(CC2500_SNOP);
     cmd(CC2500_SRX);
     */
}
/*---------------------------------------------------------------------------*/
/** @} */
