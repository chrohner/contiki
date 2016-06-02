/*---------------------------------------------------------------------------*/
/**
 * \addtogroup sensortag-cc26xx-sportident-srr
 * @{
 *
 * \file
 *  Driver for Sportident SRR
 */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "ti-lib.h"
#include "board-spi.h"
#include "board-usb.h"
#include "leds.h"

#include "srr.h"
#include "srr-const.h"


/*---------------------------------------------------------------------------*/
struct __attribute__((__packed__)) s_srr_header {
    uint32_t dest;
    uint32_t src;
    uint8_t type;
    uint8_t p1;
    uint8_t p2;
    uint8_t p3;
    uint8_t p4;
};

struct s_srr_associate_03_request {
    uint8_t p1;
    uint8_t p2;
    uint8_t p3;
    uint8_t p4;
    uint8_t p5;
    uint8_t p6;
};

struct __attribute__((__packed__)) s_punch {
    uint16_t x;         // typically 0x6bb6
    uint8_t cmd;        // 0xd3
    uint8_t len;        // 0x0d
    uint16_t siCode;
    uint32_t cardId;
    uint32_t time;      // {TD, TH, TL, TSS}
    uint8_t mem[3];
};


const uint8_t srr_associate_02_response[] = { 0x20, 0x00 };
const uint8_t srr_associate_03_response[] = { 0xef, 0xbe, 0xae, 0xde, 0x00 };
const uint8_t srr_ping_ack[] = { 0x00, 0x00, 0x4f, 0x78 };

bool srr_sniffer_mode = false;

/*---------------------------------------------------------------------------*/
/**
 * CRC calculation
 */
#define POLYNOM 0x8005

uint16_t crc(uint8_t uiCount,uint8_t *pucDat)
{
    uint8_t iTmp;
    uint16_t uiTmp,uiTmp1,uiVal;
    uint8_t *pucTmpDat;
    
    if (uiCount < 2) return(0);        // response value is "0" for none or one data byte
    pucTmpDat = pucDat;
    
    uiTmp1 = *pucTmpDat++;
    uiTmp1 = (uiTmp1<<8) + *pucTmpDat++;
    
    if (uiCount == 2) return(uiTmp1);   // response value is CRC for two data bytes
    for (iTmp=(int)(uiCount>>1);iTmp>0;iTmp--)
    {
        
        if (iTmp>1)
        {
            uiVal = *pucTmpDat++;
            uiVal= (uiVal<<8) + *pucTmpDat++;
        }
        else
        {
            if (uiCount&1)               // odd number of data bytes, complete with "0"
            {
                uiVal = *pucTmpDat;
                uiVal= (uiVal<<8);
            }
            else
            {
                uiVal=0; //letzte Werte mit 0
            }
        }
        
        for (uiTmp=0;uiTmp<16;uiTmp++)
        {
            if (uiTmp1 & 0x8000)
            {
                uiTmp1  <<= 1;
                if (uiVal & 0x8000)uiTmp1++;
                uiTmp1 ^= POLYNOM;
            }
            else
            {
                uiTmp1  <<= 1;
                if (uiVal & 0x8000)uiTmp1++;
            }
            uiVal <<= 1;
        }
    }
    return(uiTmp1);
}

void calculateCrc(uint8_t** pCmd) {
    uint8_t* cmd = *pCmd;
    uint8_t* tmp = &cmd[1];
    
    uint8_t len = cmd[2]&0xff;
    uint16_t crcValue = 0;
    
    crcValue = crc(len+2, (unsigned char*)tmp);
    
    cmd[len+3] = ((crcValue&0xff00) >> 8);
    cmd[len+4] = ((crcValue&0xff));
}


/*---------------------------------------------------------------------------*/
/**
 * punch
 */



/*---------------------------------------------------------------------------*/
/**
 * SRR protocol logic
 */


static void
srr_protocol(uint8_t *buf, uint8_t len)
{
    struct s_srr_header *h;
    struct s_punch *p;
    static uint32_t node_id = 0x01020304;
    static uint8_t cnt = 0;
    static uint8_t d3[17];
    uint8_t *_d3 = &(d3[0]);

    h = (struct s_srr_header*)buf;
    
    // prepare reply packet
    h->dest = h->src;
    h->src = (uint32_t)node_id;

    srr_cmd(CC2500_SIDLE);
    srr_cmd(CC2500_SNOP);
    srr_cmd(CC2500_SFTX);
    
    switch (h->type) {
        case 0x02:
            // associate step 3
            h->p1 = 0x20;
            h->p2 = cnt++;
            h->p3 |= 0x80;
            memcpy(buf+sizeof(struct s_srr_header), srr_associate_02_response, sizeof(srr_associate_02_response));
            if (!srr_sniffer_mode) {
                srr_write(CC2500_TXFIFO | CC2500_BURSTWRITE, buf, sizeof(struct s_srr_header)+sizeof(srr_associate_02_response));
                srr_cmd(CC2500_STX);
            }
            break;
        case 0x03:
            // associate step 2
            h->p1 |= 0x20;
            h->p2 = cnt++;
            h->p3 |= 0x80;
            memcpy(buf+sizeof(struct s_srr_header), srr_associate_03_response, sizeof(srr_associate_03_response));
            if (!srr_sniffer_mode) {
                srr_write(CC2500_TXFIFO | CC2500_BURSTWRITE, buf, sizeof(struct s_srr_header)+sizeof(srr_associate_03_response));
                srr_cmd(CC2500_STX);
            }
            break;
        case 0x05:
            // associate step 1 (broadcast)
            h->p1 |= 0x20;
            h->p3 |= 0x80;
            if (!srr_sniffer_mode) {
                srr_write(CC2500_TXFIFO | CC2500_BURSTWRITE, buf, sizeof(struct s_srr_header));
                srr_cmd(CC2500_STX);
            }
            break;
        case 0x20:
            if (buf[sizeof(struct s_srr_header) + 1] == 0xb6) {
                // punch

                // ack
                h->type = 0x3d;
                h->p1 |= 0x20;
                h->p2 = cnt++;
                h->p3 = 0x73;
                h->p4 = 0x60;
                if (!srr_sniffer_mode) {
                    srr_cmd(CC2500_SFTX);
                    srr_write(CC2500_TXFIFO | CC2500_BURSTWRITE, buf, sizeof(struct s_srr_header));
                    srr_cmd(CC2500_STX);
                }

                // pass to application (TODO: move to separate function, maintain buffer, etc)
                p = (struct s_punch*)(buf+sizeof(struct s_srr_header));
                memcpy(d3+1, p+2, sizeof(struct s_punch)-2);
                d3[0] = 0x02;
                d3[sizeof(d3)-1] = 0x03;
                calculateCrc(&_d3);
                //usb_write(d3, sizeof(d3));
            
            } else {
                // ping
                
                // ack
                h->type = 0x3d;
                h->p1 |= 0x20;
                h->p2 = cnt++;
                h->p3 = 0x73;
                h->p4 = 0x63;
                memcpy(buf+sizeof(struct s_srr_header), srr_ping_ack, sizeof(srr_ping_ack));
                if (!srr_sniffer_mode) {
                    srr_write(CC2500_TXFIFO | CC2500_BURSTWRITE, buf, sizeof(struct s_srr_header)+sizeof(srr_ping_ack));
                    srr_cmd(CC2500_STX);
                }
            }
            break;
        default:
            // unknown type (suspect error, flush RXFIFO)
            srr_cmd(CC2500_SFRX);
            srr_cmd(CC2500_SRX);
            break;
    }

}


/*---------------------------------------------------------------------------*/
/**
 * handling with interrupts (read data, or switch into rx)
 */

void
srr_rx_data(uint8_t ioid) {
    uint8_t buf[64];
    uint8_t num;
    uint8_t i;
    uint8_t rssi_lqi[2];

    leds_on(LEDS_RED);
    printf("SRR RX: ");

    // len
    srr_read(CC2500_RXFIFO | CC2500_READ, &num, 1);
    printf("%02x | ", num);
    if (num>63) {
        // limit to the buf size (but probably out of synch)
        num = 63;
        printf("(limited to %02x) | ", num);
    } else if (num>0) {
        // data
        if (srr_read(CC2500_RXFIFO | CC2500_BURSTREAD, buf, num)) {
            for (i=0; i<num; i++) {
                printf("%02x ", buf[i]);
            }
        }
        // crc
        printf("| ");
        if (srr_read(CC2500_RXFIFO | CC2500_BURSTREAD, buf+num, 2)) {
            for (i=0; i<2; i++) {
                printf("%02x ", buf[num+i]);
            }
        }
        if (srr_read(CC2500_RXFIFO | CC2500_BURSTREAD, rssi_lqi, 2)) {
            if (rssi_lqi[1]&0x80) {
                // CRC ok
                srr_protocol(buf, num);
            } else {
                printf("(error)");
            }
        }
    }
    printf("\r\n");
    
    leds_off(LEDS_RED);
}

void
srr_handle_interrupt(uint8_t ioid) {
    uint8_t state;

    if (srr_read(CC2500_MARCSTATE | CC2500_READ, &state, 1)) {
        printf("state: %02x\r\n", state);
        switch (state) {
            case 0x0d: // RX
            case 0x0e: // RX END
            case 0x0f: // RX RST
                // received packet
                srr_rx_data(ioid);
                break;
            case 0x13: // TX
            case 0x14: // TX END
                // TX finished
                srr_cmd(CC2500_SRX);
                break;
            default:
                // under/overflow (or unexpected state)
                srr_cmd(CC2500_SIDLE);
                srr_cmd(CC2500_SNOP);
                srr_cmd(CC2500_SFRX);
                srr_cmd(CC2500_SFTX);
                srr_cmd(CC2500_SRX);
                break;
        }
    }
}


/*---------------------------------------------------------------------------*/
/**
 * chip select
 */
static void
select_on_bus(void)
{
    ti_lib_gpio_pin_write(BOARD_SRR1_CS, 0);
}

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
uint32_t
srr_read(const uint8_t addr, uint8_t *buf, uint8_t len)
{
    uint32_t ret;
    
    srr_open();
    select_on_bus();
  
    ret = board_spi_write(&addr, 1);
    board_spi_read(buf, len);
    
    deselect();
    return ret;
}
/*---------------------------------------------------------------------------*/
bool
srr_write(const uint8_t addr, const uint8_t *buf, uint8_t len)
{
    srr_open();
    select_on_bus();

    if (board_spi_write(&addr, 1) == false) {
        /* failure */
        deselect();
        return false;
    }
    
    if (addr == (CC2500_TXFIFO | CC2500_BURSTWRITE)) {
        board_spi_write(&len, 1);
    }
    
    if (board_spi_write(buf, len) == false) {
      /* failure */
      deselect();
      return false;
    }
    
    deselect();
    return true;
}
/*---------------------------------------------------------------------------*/
/**
 * Configure CC2500 for Sportident SRR
 */
uint32_t
srr_cmd(uint8_t cmd) {
    uint32_t ret;
    
    srr_open();
    select_on_bus();
    
    ret = board_spi_write(&cmd, 1);
    clock_delay_usec(1);
    
    deselect();
    
    return ret;
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
    //ti_lib_ioc_pin_type_gpio_output(BOARD_IOID_SPI_CLK);
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
    //board_spi_open(6000000, BOARD_IOID_SPI_CLK);   // roh: spi opened at
    board_spi_write(&res, 1);

    // release CSn, wait for SO to go low
    ti_lib_gpio_pin_write(BOARD_SRR1_CS, 1);
    // ti_lib_ioc_pin_type_gpio_input(BOARD_IOID_SPI_MISO);
    while (ti_lib_gpio_pin_read(1 << BOARD_IOID_SPI_MISO) == 1) {
        clock_delay_usec(1);
    }; // TODO: don't wait forever!

    //board_spi_close();
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
    
    // write the configuration
    for (i=0; i < sizeof(cc2500_srr_config); i+=2) {
        srr_write(cc2500_srr_config[i], &cc2500_srr_config[i+1], 1);
        clock_delay_usec(10); // not necessary
    }

    // debug configuration
    printf("SRR configuration (read back)\r\n");
    for (i=0; i < sizeof(cc2500_srr_config); i+=2) {
        ret = srr_read(CC2500_READ | cc2500_srr_config[i], &buf, 1);
        srr_close();
        if (ret) {
            printf("  %02x: %02x  (%02x)\r\n", cc2500_srr_config[i], buf, ret);
        } else {
            printf("  %02x: -\r\n", cc2500_srr_config[i]);
        }
    }
    
    clock_delay_usec(100);
}

/*---------------------------------------------------------------------------*/
void
srr_start() {
    srr_cmd(CC2500_SIDLE);
    srr_cmd(CC2500_SNOP);
    srr_cmd(CC2500_SFRX);
    srr_cmd(CC2500_SRX);
    
    clock_delay_usec(809);   // IDLE->RX with calibration (section 19.6)

}
/*---------------------------------------------------------------------------*/
/** @} */
