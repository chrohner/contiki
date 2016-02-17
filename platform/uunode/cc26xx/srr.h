#ifndef __SRR_H__
#define __SRR_H__
/*---------------------------------------------------------------------------*/
void srr_close();
bool srr_read(const uint8_t addr, uint8_t *buf);
bool srr_write(const uint8_t addr, const uint8_t buf);
void srr_init();
void srr_reset();
void srr_config();
void srr_init();
void srr_start();
/*---------------------------------------------------------------------------*/
/** @} */
#endif