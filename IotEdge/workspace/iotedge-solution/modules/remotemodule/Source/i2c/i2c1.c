#include "i2c1.h"

struct bcm2835_peripheral bsc1 = {.addr_p = BSC1_BASE};

int i2c1_init()
{
  if (map_peripheral(&bsc1) == -1)
  {
    printf("Failed to map I2C1 (BC1) peripheral.\n");
    return -1;
  }
  return 0;
}

void dump_bsc1_status()
{
  unsigned int s = BSC1_S;
  printf("BSC1_S: ERR=%d  RXF=%d  TXE=%d  RXD=%d  TXD=%d  RXR=%d  TXW=%d  DONE=%d  TA=%d\n",
         (s & BSC_S_ERR) != 0,
         (s & BSC_S_RXF) != 0,
         (s & BSC_S_TXE) != 0,
         (s & BSC_S_RXD) != 0,
         (s & BSC_S_TXD) != 0,
         (s & BSC_S_RXR) != 0,
         (s & BSC_S_TXW) != 0,
         (s & BSC_S_DONE) != 0,
         (s & BSC_S_TA) != 0);
}

void wait_i2c1_done()
{
  int timeout = 50;
  while ((!((BSC1_S)&BSC_S_DONE)) && --timeout)
  {
    usleep(1000);
  }
  if (timeout == 0)
    printf("wait_i2c_done() timeout. Something went wrong.\n");
}

int i2c1_write_register(uint8_t i2c_addr, uint8_t reg_addr, const uint8_t *data, uint32_t len)
{
  BSC1_S = BSC1_CLEAR_STATUS;
  BSC1_A = i2c_addr;
  BSC1_DLEN = len + 1; // Register address + data
  BSC1_FIFO = reg_addr;
  for (uint32_t i = 0; i < len; ++i)
  {
    BSC1_FIFO = data[i];
  }
  BSC1_C = BSC1_START_WRITE;
  wait_i2c1_done();
  if (BSC1_S & BSC1_S_ERR)
  {
    printf("I2C1 write error\n");
    return -1;
  }
  return 0;
}

int i2c1_read_register(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *data, uint32_t len)
{
  BSC1_S = BSC1_CLEAR_STATUS;
  BSC1_A = i2c_addr;
  BSC1_DLEN = 1;
  BSC1_FIFO = reg_addr;
  BSC1_C = BSC1_START_WRITE;
  wait_i2c1_done();

  BSC1_S = BSC1_CLEAR_STATUS;
  BSC1_A = i2c_addr;
  BSC1_DLEN = len;
  BSC1_C = BSC1_START_READ;
  wait_i2c1_done();

  if (BSC1_S & BSC1_S_ERR)
  {
    printf("I2C1 read error\n");
    return -1;
  }
  for (uint32_t i = 0; i < len; ++i)
  {
    data[i] = BSC1_FIFO & 0xFF;
  }
  return 0;
}