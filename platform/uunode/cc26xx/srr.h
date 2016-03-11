#ifndef __SRR_H__
#define __SRR_H__
/*---------------------------------------------------------------------------*/
bool srr_read(const uint8_t addr, uint8_t *buf);
bool srr_write(const uint8_t addr, const uint8_t buf);
bool srr_cmd(const uint8_t addr);
void srr_reset();

bool srr_open();
void srr_close();
void srr_config();
void srr_start();
/*---------------------------------------------------------------------------*/
/** @} */
#endif