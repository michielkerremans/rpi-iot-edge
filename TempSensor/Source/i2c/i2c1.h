#ifndef I2C1_H
#define I2C1_H

#include "PJ_RPI.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

// I2C1 (BC1) base address for RPi 4
#define BSC1_BASE 0xFE804000

// Extern for the I2C1 peripheral
extern struct bcm2835_peripheral bsc1;

// I2C1 Control register
#define BSC1_C *(bsc1.addr + 0x00)

#define BSC1_C_I2CEN (1 << 15) // I2C Enable
#define BSC1_C_INTR (1 << 10)  // Interrupt on RX
#define BSC1_C_INTT (1 << 9)   // Interrupt on TX
#define BSC1_C_INTD (1 << 8)   // Interrupt on Done
#define BSC1_C_ST (1 << 7)     // Start transfer
#define BSC1_C_CLEAR (1 << 4)  // Clear FIFO
#define BSC1_C_READ 1          // Read transfer

#define BSC1_START_READ BSC1_C_I2CEN | BSC1_C_ST | BSC1_C_CLEAR | BSC1_C_READ
// I2C Enable + Start transfer + Clear FIFO + Read transfer
#define BSC1_START_WRITE BSC1_C_I2CEN | BSC1_C_ST
// I2C Enable + Start transfer

// I2C1 Status register
#define BSC1_S *(bsc1.addr + 0x01)

#define BSC1_S_CLKT (1 << 9) // Clock stretch timeout
#define BSC1_S_ERR (1 << 8)  // ACK error
#define BSC1_S_RXF (1 << 7)  // RX FIFO full
#define BSC1_S_TXE (1 << 6)  // TX FIFO empty
#define BSC1_S_RXD (1 << 5)  // RX FIFO contains data
#define BSC1_S_TXD (1 << 4)  // TX FIFO can accept data
#define BSC1_S_RXR (1 << 3)  // RX FIFO needs reading (full)
#define BSC1_S_TXW (1 << 2)  // TX FIFO needs writing (full)
#define BSC1_S_DONE (1 << 1) // Transfer Done
#define BSC1_S_TA 1          // Transfer Active

#define BSC1_CLEAR_STATUS BSC1_S_CLKT | BSC1_S_ERR | BSC1_S_DONE
// Clock stretch timeout + ACK error + Transfer Done

// I2C1 Data Length register
#define BSC1_DLEN *(bsc1.addr + 0x02)

// I2C1 Slave Address register
#define BSC1_A *(bsc1.addr + 0x03)

// I2C1 Data FIFO register
#define BSC1_FIFO *(bsc1.addr + 0x04)

// Function prototypes
int i2c1_init();
void dump_bsc1_status();
void wait_i2c1_done();
int i2c1_write_register(uint8_t i2c_addr, uint8_t reg_addr, const uint8_t *data, uint32_t len);
int i2c1_read_register(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *data, uint32_t len);

#endif