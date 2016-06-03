/* 
	Sportident SRR initialization 


	- SRR is using a CC2500 chip that communicates over SPI
	- Reverse engineered by Christian Rohner, May 2015
*/

#ifndef SRR_CONST_H
#define SRR_CONST_H

#include "cc2500-const.h"

#define SRR_CHANNEL_RED                     0x92
#define SRR_CHANNEL_BLUE                    0xBA

// --- strobes

// reset chip
static const uint8_t cc2500_srr_reset[] = {
    0x00,
    CC2500_SRES
};

// go into idle, flush RX buffer
static const uint8_t cc2500_srr_idle[] = {
	CC2500_SIDLE,
	CC2500_SNOP,
	CC2500_SFRX
};

// enable RX
static const uint8_t cc2500_srr_rx[] = {
    CC2500_SRX
};


// --- version

static const uint8_t cc2500_srr_version[] = {
	CC2500_PARTNUM,
	CC2500_VERSION
};


// --- config:

static const uint8_t cc2500_srr_config[] = {

CC2500_IOCFG2,      IOCFG_GDO_CFG_CS,	// carrier sense
CC2500_IOCFG0,      IOCFG_GDO_CFG_PKT_SYNCW_EOP,

CC2500_FIFOTHR,     0x15,   // (TX: 1) RX: 64
    
// CC2500_MCSM2
CC2500_MCSM1,       0x3F,	// CC threshold, Rx->Rx, Tx->Rx
CC2500_MCSM0,       0x18,	// calibration IDLE->{RX,TX}, PO timeout 150us, XOSC off in SLEEP

CC2500_PKTLEN,      0x29,
CC2500_PKTCTRL1,    0x08,   // autoflush (CRC check)
CC2500_PKTCTRL0,    0x05,	// no withening, Normal mode, use FIFOs for RX and TX
                            // CRC, Variable packet length mode
CC2500_PATABLE,     0xFE,  	// 0dBm output power
CC2500_ADDR,        0x00,

CC2500_CHANNR,      SRR_CHANNEL_RED,

CC2500_FSCTRL1,     0x07,	// FREQ_IF 26MHz/2**10 * 7 = 177,7kHz
CC2500_FSCTRL0,     0x00,	// FREQOFF 0 Hz

CC2500_FREQ2,       0x5D,	// FREQ 26MHz/2**16 * 6112492 = 2,425MHz
CC2500_FREQ1,       0x44,
CC2500_FREQ0,       0xEC,

CC2500_MDMCFG4,     0x2D,	// 541,6kHz Bandwidth
CC2500_MDMCFG3,     0x3B,	// 250kBaud data rate  (sensitivity: see Figure 24)
CC2500_MDMCFG2,     0x73,	// MSK, 30 of 32 bits
CC2500_MDMCFG1,     0x23,	// 4B pre-amble
CC2500_MDMCFG0,     0x3B,	// 250kHz channel spacing (m=59, e=3)

CC2500_DEVIATN,     0x01,

CC2500_FOCCFG,      0x1D,
CC2500_BSCFG,       0x1C,

CC2500_AGCCTRL2,    0xC7,
CC2500_AGCCTRL1,    0x00,
CC2500_AGCCTRL0,    0xB0,

// CC2500_WOREVT1,  0x87,	// Event 0 timeout 1s
// CC2500_WOREVT0,  0x6B,

CC2500_FREND1,      0xB6,
CC2500_FREND0,      0x10,

CC2500_FSCAL3,      0xEA,
CC2500_FSCAL2,      0x0A,
CC2500_FSCAL1,      0x00,
CC2500_FSCAL0,      0x11,

// CC2500_RCCTRL1,  0x41,
// CC2500_RCCTRL0,  0x00,

// CC2500_FSTEST,   0x59,	// do not write
// CC2500_PTEST,    0x7F,	// 0xBF: on-chip temperature
// CC2500_AGCTEST,  0x3F,	// do not write

CC2500_TEST2,       0x88,
CC2500_TEST1,       0x31,
CC2500_TEST0,       0x0B
};



// --- procedure

// SRES

// read part number, version

// config

// wait for calibration to finish (preamble quality, channel clear, RX state)
// - read 0xF8 until staus 1x (phase 1) and 2x (phase 2)
// - read 0xF8 until staus 1x (phase 1) and 3x (phase 2)  (or bits 4,5 set)

// measure noise floor
// - read 0xF4 (16 times)

// SIDLE, SNOP, SFRX  (go into idle, flush RX buffer)
// read GDO0 configuration

// SIDLE, SNOP, SFRX  (go into idle, flush RX buffer)
// set RED channel    (set again below, could probably be optimized)
// enable RX

// SIDLE, SNOP, SFRX  (go into idle, flush RX buffer)
// set RED/BLUE channel  {blue: 0xBA, red: 0x92}

#endif




