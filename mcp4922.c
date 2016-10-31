#include "mcp4922.h"

void mcp4922_send_word(uint16_t data){
    uint8_t i;
    WRITE(PIN_MCP4922_NCS, LOW);
    for(i=0; i<16; i++) {
        WRITE(PIN_MCP4922_CLK, LOW);
        WRITE(PIN_MCP4922_DAT, (data & 0x8000) ? HIGH : LOW);
        WRITE(PIN_MCP4922_CLK, HIGH);
        data = data << 1;
    }
    WRITE(PIN_MCP4922_NCS, HIGH);
    WRITE(PIN_MCP4922_CLK, LOW);
    WRITE(PIN_MCP4922_DAT, LOW);
}

void mcp4922_init(void){
    CLR_PIN(PIN_MCP4922_NCS);
    SET_OUTPUT(PIN_MCP4922_NCS);
    CLR_PIN(PIN_MCP4922_DAT);
    SET_OUTPUT(PIN_MCP4922_DAT);
    CLR_PIN(PIN_MCP4922_CLK);
    SET_OUTPUT(PIN_MCP4922_CLK);
    mcp4922_send_word(MCP4922_CHANNEL_A|MCP4922_GAIN1X);
    mcp4922_send_word(MCP4922_CHANNEL_B|MCP4922_GAIN1X);
}

void mcp4922_set_master(uint16_t value) {
    mcp4922_send_word(MCP4922_SET_MASTER|(value & 0x0fff));
}

void mcp4922_set_modulation(uint8_t value) {
    uint16_t tmp = (value << 4) | (value >> 4);
    mcp4922_send_word(MCP4922_SET_MODULATION|tmp);
}
