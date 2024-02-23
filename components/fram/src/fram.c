#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "i2c.h"

#include "fram.h"

int __i2c_fram_read_bytes(uint16_t addr, size_t length, uint8_t* data);
int __i2c_fram_read_word(uint16_t addr, uint16_t* data);
int __i2c_fram_read_byte(uint16_t addr, uint8_t* data);
int __i2c_fram_write_bytes(uint16_t addr, size_t length, uint8_t* data);
int __i2c_fram_write_word(uint16_t addr, uint16_t data);
int __i2c_fram_write_byte(uint16_t addr, uint8_t data);

int __i2c_fram_read_bytes(uint16_t addr, size_t length, uint8_t* data)
{
    uint8_t upper_addr_bit = (uint8_t)((addr & 0x100)>>8);
    uint8_t lower_addr = (uint8_t)(addr & 0xFF);

    // TODO: Replace with ESP32 API
    // if(i2c_write_blocking(MB85RC_DEFAULT_ADDRESS | upper_addr_bit, &lower_addr, 1, true) == PICO_ERROR_GENERIC){
	// 	return -1;
	// }

	// int bytes_read = i2c_read_blocking(MB85RC_DEFAULT_ADDRESS| upper_addr_bit, data, length, false);
    // if( bytes_read == PICO_ERROR_GENERIC){
	// 	return -1;
	// }
	// return bytes_read;

    return 0; // TODO: Remove this line
}

int __i2c_fram_read_byte(uint16_t addr, uint8_t* data)
{
    // TODO: Replace with ESP32 API
	// int num_bytes_read = __i2c_fram_read_bytes(addr, 1, data);
	// return num_bytes_read;

    return 0; // TODO: Remove this line
}

int __i2c_fram_read_word(uint16_t addr, uint16_t* data)
{
    // TODO: Replace with ESP32 API
	// uint8_t buf[2];
	// int num_bytes_read = __i2c_fram_read_bytes(addr, 2, &buf[0]);
	// *data = (buf[0] << 8) + (buf[1] & 0xFF);
	// return num_bytes_read;

    return 0; // TODO: Remove this line
}

int __i2c_fram_write_bytes(uint16_t addr, size_t length, uint8_t* data){
    uint8_t upper_addr_bit = (uint8_t)((addr & 0x100)>>8);
	uint8_t writeData[length+1];
	writeData[0] = (uint8_t)(addr & 0xFF);
	for(int i=0; i<length; i++) writeData[i+1]=data[i];

    // TODO: Replace with ESP32 API
	// if(i2c_write_blocking(MB85RC_DEFAULT_ADDRESS | upper_addr_bit, &writeData[0], length+1, false) == PICO_ERROR_GENERIC){
	// 	return -1;
	// }

	return 0;
}

int __i2c_fram_write_byte(uint16_t addr, uint8_t data)
{
    // TODO: Replace with ESP32 API
	// if(__i2c_fram_write_bytes(addr, 1, &data) == PICO_ERROR_GENERIC){
	// 	return -1;
	// }
	return 0;
}

int __i2c_fram_write_word(uint16_t addr, uint16_t data)
{
    uint8_t buf[2];
	buf[0] = (data >> 8) & 0xFF;
	buf[1] = (data & 0xFF);

    // TODO: Replace with ESP32 API
	// if(__i2c_fram_write_bytes(addr, 2, &buf[0]) == PICO_ERROR_GENERIC){
	// 	return -1;
	// }

	return 0;
}


int __get_device_id(uint16_t *manuf_id, uint16_t *prod_id)
{
    // This function is broken, and thus the init fuction is broken
    // The problem seems to be because the MASTER_CODE is not an allowed address
    uint8_t buf[3] = {0};
    //uint8_t master_code = MASTER_CODE;
    uint8_t addr = FRAM_DEFAULT_ADDRESS;

    // TODO: Replace with ESP32 API
    // i2c_write_blocking(MASTER_CODE, &addr, 1, true);
    // i2c_read_blocking(MASTER_CODE, &buf[0], 3, false);

    *manuf_id = (buf[0] << 4) + (buf[1] >> 4);
    *prod_id = ((buf[1] & 0x0F) << 8) + buf[2];
    return 0;
}

// static i2c_inst_t* i2c;

/**
* functions for external use
**/
int fram_init()
{
    // TODO: Replace with ESP32 API
    // i2c = I2C_FRAM;
    // init_i2c();

    // This function is broken, because __get_device_id is broken
    // We can still read and write to mem, just not check on startup that its the right chip
    uint16_t manuf_id, prod_id;
    __get_device_id(&manuf_id, &prod_id);
    printf("IDs: %X, %X\n", manuf_id, prod_id);
    if (manuf_id != FUJITSU_MANUF_ID){
        printf("ERROR: manuf_id does not match FUJITSU_MANUF_ID\n");
        //return -1;
    }
    if (prod_id != FRAM_PROD_ID){
        printf("ERROR: prod_id does not match PROD_ID_MB85RC04V\n");
        //return -1;
    }
    return 0;
}

int fram_read(uint16_t addr, size_t length, uint8_t* data)
{
    // TODO: Replace with ESP32 API
    // uint32_t irq_status = save_and_disable_interrupts(); //Prevent IMU interrupts

    int ret = __i2c_fram_read_bytes(addr, length, data);

    // TODO: Replace with ESP32 API
    // restore_interrupts(irq_status); //Restore IRQs

    return ret;
}

int fram_write(uint16_t addr, size_t length, uint8_t* data)
{
    // TODO: Replace with ESP32 API
    // uint32_t irq_status = save_and_disable_interrupts(); //Prevent IMU interrupts

    int ret = __i2c_fram_write_bytes(addr, length, data);

    // TODO: Replace with ESP32 API
    // restore_interrupts(irq_status); //Restore IRQs

    return ret;
}

int fram_read_word(uint16_t addr, uint16_t* data)
{
    int ret = __i2c_fram_read_word(addr, data);
    return ret;
}

int fram_write_word(uint16_t addr, uint16_t data)
{
    int ret = __i2c_fram_write_word(addr, data);
    return ret;
}

int erase(void)
{   
    int16_t i = 0;
    int ret = 0;
    while((i < FRAM_MAX_ADDRESS) & (ret == 0)){
        ret = fram_write_word(i, 0x0000);
        i++;
    }
    if(ret != 0){
        printf("ERROR: FRAM erase stopped at addr: 0x%X", i);
    }
    return ret;
}