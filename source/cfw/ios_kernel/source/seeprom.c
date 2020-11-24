#include "seeprom.h"

static inline void regDelay(uint32_t ticks) {
	uint32_t now = HW_REG(LT_TIMER);
	while((HW_REG(LT_TIMER) - now) < ticks);
}

uint32_t spiRead(int32_t bitCount) {
    uint32_t word = 0;
    while(bitCount--) {
        word <<= 1;
        HW_REG(LT_GPIO_OUT) |= EEPROM_SPI_SCK;
        regDelay(9);

        HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_SCK;
        regDelay(9);

        word |= ((HW_REG(LT_GPIO_IN) & EEPROM_SPI_MISO) != 0);
    }
    return word;
}

void spiWrite(uint32_t word, int32_t bitCount) {
    while(bitCount--) {
        if (word & (1 << bitCount)) HW_REG(LT_GPIO_OUT) |= EEPROM_SPI_MOSI;
        else HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_MOSI;
        regDelay(9);

        HW_REG(LT_GPIO_OUT) |= EEPROM_SPI_SCK;
        regDelay(9);

        HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_SCK;
        regDelay(9);
    }
}

void readSeeprom(uint16_t* seepromBuffer) {
    int32_t offset = 0 >> 1;
	int32_t size = 512 >> 1;
	HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_SCK;
	HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_CS;
	regDelay(9);
		
	for (int32_t i=0; i<size; ++i, ++offset) {
		HW_REG(LT_GPIO_OUT) |= EEPROM_SPI_CS;
		spiWrite((0x600 | offset), 11);
		
		seepromBuffer[i] = spiRead(16);
		
		HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_CS;
		regDelay(9);
	}
}