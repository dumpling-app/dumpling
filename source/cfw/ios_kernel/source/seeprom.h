#include <stdint.h>
#include <stdbool.h>

// Based on mini implementation by sven peter
#define	LT_TIMER            0x0d800010
#define LT_GPIO_OUT         0x0d8000e0
#define LT_GPIO_IN          0x0d8000e8

#define EEPROM_SPI_CS       0x400
#define EEPROM_SPI_SCK      0x800
#define EEPROM_SPI_MOSI     0x1000
#define EEPROM_SPI_MISO     0x2000

#define HW_REG(reg)         (*(volatile unsigned int*)(reg))

static inline void regDelay(uint32_t ticks) {
	uint32_t now = HW_REG(LT_TIMER);
	while((HW_REG(LT_TIMER) - now) < ticks);
}

void readSeeprom(uint16_t* seepromBuffer);