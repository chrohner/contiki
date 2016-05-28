#ifndef __SRR_H__
#define __SRR_H__
/*---------------------------------------------------------------------------*/
extern bool srr_sniffer_mode;

void srr_rx_data();

uint32_t srr_read(const uint8_t addr, uint8_t *buf, uint8_t len);
bool srr_write(const uint8_t addr, const uint8_t buf);
uint32_t srr_cmd(const uint8_t addr);
void srr_reset();

bool srr_open();
void srr_close();
void srr_config();
void srr_start();
/*---------------------------------------------------------------------------*/
/** @} */
#endif