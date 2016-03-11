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
                                IOC_NO_IOPULL    | IOC_SLEW_DISABLE  | \
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
        leds_toggle(LEDS_GREEN);
//        if (ti_lib_gpio_pin_read(BOARD_SRR1_GDO0) == 1) {
//        }
    }

    if (ioid == BOARD_IOID_KEY_LEFT) {
        leds_toggle(LEDS_GREEN);
        if (ti_lib_gpio_pin_read(BOARD_KEY_LEFT) == 1) {
        }
    }

    
}

static void
int_enable(uint8_t ioid)
{
    ti_lib_gpio_event_clear(1 << ioid);
    ti_lib_ioc_port_configure_set(ioid, IOC_PORT_GPIO, GDOx_GPIO_CFG);
    ti_lib_gpio_dir_mode_set(1 << ioid, GPIO_DIR_MODE_IN);
    gpio_interrupt_register_handler(ioid, int_handler);
    ti_lib_ioc_int_enable(ioid);
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
  board_spi_open(6000000, BOARD_IOID_SPI_CLK);

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
    
    // configure IO to set the reset signals
    ti_lib_ioc_pin_type_gpio_output(BOARD_IOID_SPI_MOSI);
    ti_lib_ioc_pin_type_gpio_input(BOARD_IOID_SPI_MISO);
    ti_lib_ioc_pin_type_gpio_output(BOARD_IOID_SPI_CLK);
    ti_lib_ioc_pin_type_gpio_output(BOARD_IOID_SPI_CC2500_1_CS);
    
    // SCLK=1, SI=0, strobe CSn
    ti_lib_gpio_pin_write(1 << BOARD_IOID_SPI_CLK, 1);
    ti_lib_gpio_pin_write(1 << BOARD_IOID_SPI_MOSI, 0);
    ti_lib_gpio_pin_write(BOARD_SRR1_CS, 0);
    clock_delay_usec(10);
    ti_lib_gpio_pin_write(BOARD_SRR1_CS, 1);

    // wait 40us, pull CSn low, wait for SO to go low
    clock_delay_usec(40);
    ti_lib_gpio_pin_write(BOARD_SRR1_CS, 0);
    while (ti_lib_gpio_pin_read(1 << BOARD_IOID_SPI_MISO) == 1) {
        clock_delay_usec(1);
    }; // TODO: don't wait forever!

    // SRES strobe
    board_spi_open(6000000, BOARD_IOID_SPI_CLK);
    board_spi_write(&res, 1);

    // release CSn, wait for SO to go low
    ti_lib_gpio_pin_write(BOARD_SRR1_CS, 1);
    // ti_lib_ioc_pin_type_gpio_input(BOARD_IOID_SPI_MISO);
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
    
    srr_open();

    // write the configuration
    
    while (i < sizeof(cc2500_srr_config)-1) {
        // set configuration
        srr_write(cc2500_srr_config[i], cc2500_srr_config[i+1]);
        clock_delay_usec(100); // not necessary
        i += 2;
    }
    
    // read CC2500_CHANNR to verify
    ret = srr_read(CC2500_READ | CC2500_IOCFG0, &buf);
    
    if (ret) {
        leds_off(LEDS_GREEN);
    }
    if (buf == IOCFG_GDO_CFG_PKT_SYNCW_EOP) {
        leds_off(LEDS_RED);

        // configure callbacks (GDO0)
        int_enable(BOARD_IOID_SPI_CC2500_1_GDO0);
        // int_enable(BOARD_IOID_KEY_LEFT);
    }
    
    // SIDLE, SNOP, SRFRX
    
    while (i < sizeof(cc2500_srr_idle)-1) {
        // set configuration
        srr_cmd(cc2500_srr_idle[i]);
        clock_delay_usec(100); // not necessary
        i += 1;
    }

    //srr_cmd(CC2500_SRX);
    clock_delay_usec(100);
    
//    srr_close();

}

/*---------------------------------------------------------------------------*/
void
srr_start() {
//    srr_open();
    
    srr_cmd(CC2500_SIDLE);
    srr_cmd(CC2500_SNOP);
    srr_cmd(CC2500_SNOP);

    clock_delay_usec(809);   // IDLE->RX with calibration (section 19.6)

//    srr_close();
}
/*---------------------------------------------------------------------------*/
/** @} */
