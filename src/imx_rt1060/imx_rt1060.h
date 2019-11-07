// Copyright © 2019 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef IMX_RT1060_H
#define IMX_RT1060_H

#include <cstdint>

typedef struct {
	const uint32_t VERID;
	const uint32_t PARAM;
	const uint32_t unused1;
	const uint32_t unused2;
	volatile uint32_t MCR;			// 010
	volatile uint32_t MSR;			// 014
	volatile uint32_t MIER;			// 018
	volatile uint32_t MDER;			// 01C
	volatile uint32_t MCFGR0;		// 020
	volatile uint32_t MCFGR1;		// 024
	volatile uint32_t MCFGR2;		// 028
	volatile uint32_t MCFGR3;		// 02C
	volatile uint32_t unused3[4];
	volatile uint32_t MDMR;			// 040
	volatile uint32_t unused4;
	volatile uint32_t MCCR0;		// 048
	volatile uint32_t unused5;
	volatile uint32_t MCCR1;		// 050
	volatile uint32_t unused6;
	volatile uint32_t MFCR;			// 058
	volatile uint32_t MFSR;			// 05C
	volatile uint32_t MTDR;			// 060
	volatile uint32_t unused7[3];
	volatile uint32_t MRDR;			// 070
	volatile uint32_t unused8[39];
	volatile uint32_t SCR;			// 110
	volatile uint32_t SSR;			// 114
	volatile uint32_t SIER;			// 118
	volatile uint32_t SDER;			// 11C
	volatile uint32_t unused9;
	volatile uint32_t SCFGR1;		// 124
	volatile uint32_t SCFGR2;		// 128
	volatile uint32_t unused10[5];
	volatile uint32_t SAMR;			// 140
	volatile uint32_t unused11[3];
	volatile uint32_t SASR;			// 150
	volatile uint32_t STAR;			// 154
	volatile uint32_t unused13[2];
	volatile uint32_t STDR;			// 160
	volatile uint32_t unused14[3];
	volatile uint32_t SRDR;			// 170
} IMXRT_LPI2C_Registers;
#define LPI2C1		(*(IMXRT_LPI2C_Registers *)0x403F0000)
#define LPI2C2		(*(IMXRT_LPI2C_Registers *)0x403F4000)  // Not connected to any pins on the Teensy 4.0
#define LPI2C3		(*(IMXRT_LPI2C_Registers *)0x403F8000)
#define LPI2C4		(*(IMXRT_LPI2C_Registers *)0x403FC000)

#endif //IMX_RT1060_H