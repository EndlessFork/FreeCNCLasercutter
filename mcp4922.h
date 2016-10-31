#ifndef _4094_H_
#define _4094_H_

#include <inttypes.h>

#define LOW 0
#define HIGH 1

#include "pins.h"


#define PIN_MCP4922_CLK  PIN_F4
#define PIN_MCP4922_DAT  PIN_F5
#define PIN_MCP4922_NCS  PIN_F6

#define MCP4922_CHANNEL_A  0x0000
#define MCP4922_CHANNEL_B  0x8000
#define MCP4922_UNBUFFERED 0x0000
#define MCP4922_BUFFERED   0x4000
#define MCP4922_GAIN2X     0x0000
#define MCP4922_GAIN1X     0x2000
#define MCP4922_ACTIVE     0x0000
#define MCP4922_SHUTDOWN   0x1000

#define MCP4922_SET_MASTER (MCP4922_CHANNEL_A|MCP4922_UNBUFFERED|MCP4922_GAIN1X|MCP4922_ACTIVE)
#define MCP4922_SET_MODULATION (MCP4922_CHANNEL_B|MCP4922_UNBUFFERED|MCP4922_GAIN1X|MCP4922_ACTIVE)

void mcp4922_init(void);
void mcp4922_set_master(uint16_t); // set 12 bit value on channel A
void mcp4922_set_modulation(uint8_t);   // set 8 bit value on channel B

#endif
