/*
* config.c
*
* Created: 10-Jun-23 05:49:15 PM
*  Author: Elmah
*/

#include "config.h"
#include "avr/eeprom.h"
#include "commandHandler.h"

#define CONFIG_EEPROM_ADDR ((void*)0)

config_CoreConfigS config_coreConfig;

static void config_handleEEPROMBusy();

config_CoreConfigS* config_getConfig()
{
    return &config_coreConfig;
}

bool config_saveConfig()
{
    bool saveResult=false;

    if(eeprom_is_ready())
    {
        eeprom_write_block(&config_coreConfig,CONFIG_EEPROM_ADDR,sizeof(config_coreConfig));
        saveResult=true;
        commandHandler_sendCmdOkResNoData(CMD_TYPE_SETTINGS_SAVE);
    }
    else
    {
        config_handleEEPROMBusy();
    }

    return saveResult;
}

static void config_handleEEPROMBusy()
{
    uint8_t msgBuffer[USART_MSG_CMD_RES_HEADER_SIZE];
    uint8_t msgBufferSize=sizeof(msgBuffer)/sizeof(msgBuffer[0]);
    uint8_t dataSize=msgBufferSize-USART_MSG_CMD_RES_HEADER_SIZE;
    commandHandler_buildCmdResponseHeader(msgBuffer,dataSize,ERROR_TYPE_STORAGE_BUSY);
    usart_writeBuf(msgBuffer,msgBufferSize);
}

void config_init()
{
    eeprom_busy_wait();
    eeprom_read_block(&config_coreConfig,CONFIG_EEPROM_ADDR,sizeof(config_coreConfig));
}