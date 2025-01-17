
#include "types.h"
#include "myDSiMode.h"

#include <stdlib.h>
#include <string.h>

#include <nds.h>
#include "rom.h"
#include "io.h"
#include "gpu.h"

u8 *staticRam;
u8 *internalRam;
u8 *externalEeprom;
FILE* wsRom;
u32 wsRomSize;

u32	sramAddressMask;
u32	eepromAddressMask;
u32	romAddressMask;

void cpuWriteByte(u32 addr, u8 value)
{
	u32 offset = addr & 0xFFFF;
	u32 bank = addr >> 16;

		// 0 - RAM - 16 KB (WS) / 64 KB (WSC) internal RAM
	if (!bank)
		gpuWriteByte(offset, value);
	else if(bank == 1) // SRAM
		staticRam[offset & sramAddressMask] = value;
}

u8 cpuReadByte(u32 addr)
{
	u32 romBank;
	u8 bank = addr >> 16;
	u32 offset = addr & 0xFFFF;

	switch (bank) {
		case 0:
				// 0 - RAM - 16 KB (WS) / 64 KB (WSC) internal RAM
			if(gpuIsColor)
				return internalRam[offset];
			else if(offset < 0x4000)
				return internalRam[offset];

			return 0xFF;
		case 1:
				// 1 - SRAM (cart)
			return staticRam[offset & sramAddressMask];
		case 2:
		case 3:
				// 2 - ROM Bank (initial bank = last)
				// 3 - ROM Bank (lnitial bank = last)
			return romGetByte(offset + ((ioPort[0xC0 + bank] & ((wsRomSize >> 16)-1)) << 16));
		default:
				// Banks 4 - F
			romBank = 256 - ((ioPort[0xC0] & 0x0F) << 4 | (bank & 0x0F));
			return romGetByte(offset + wsRomSize - (romBank << 16));
	}
}

void memInit(const char* filename)
{
		// Open ROM file
	wsRom = fopen(filename, "rb");

	// Cache file size
	fseek(wsRom, 0, SEEK_END);
	wsRomSize = ftell(wsRom);

		// Get header
	pRomHeader RomHeader = romGetHeader();

		// Allocate buffers and static RAM
	romBuffer = (u8*)malloc(dsiFeatures() ? ROM_BUFFER_SIZE_LARGE : ROM_BUFFER_SIZE);
	internalRam = (u8*)malloc(0x10000);
	staticRam = (u8*)malloc(0x10000);
	externalEeprom = (u8*)malloc(romEepromSize());

		// Bit masks
	sramAddressMask = romSramSize() - 1;
	eepromAddressMask = romEepromSize() - 1;
	romAddressMask = wsRomSize - 1;

		// Game Compatibility
	gpuIsColor = RomHeader->minimumSupportSystem;

		// Free header
	free(RomHeader);

		// Fill ROM buffer from the beginning
	romFillBuffer(0);
}
