/*
* commandHandler.c
*
* Created: 31-May-23 10:14:34 PM
*  Author: Elmah
*/

#include "usart.h"
#include <string.h>
#include "config.h"
#include "usart.h"
#include "commandHandler.h"
#include <stdbool.h>
#include <avr/io.h>
#include <avr/portpins.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

static void commandHandler_resetMsg();
/*
Initializes timer for command timeout handling
*/
static void commandHandler_timerInit();
static void commandHandler_handleInvalidMsgType();
static void commandHandler_handleInvalidCmdType();
static void commandHandler_onCommandTimeout();
static void commandHandler_handleMsg();
static void commandHandler_handleRequestCmd();
static void commandHandler_handleSystemBusy();
static void commandHandler_handleEEPROMBusy();
static void commandHandler_handleInvalidDataSize(uint8_t expectedDataSize,uint8_t actualDataSize);

/*
Returns pointer to buffer that is used to store incoming command
*/
static commandHandler_MsgS* commandHandler_getMsg();

static commandHandler_MsgS commandHandler_msgStorage;

static void commandHandler_handleInvalidMsgType()
{
    commandHandler_MsgS* msg=commandHandler_getMsg();

    uint8_t msgBuffer[USART_MSG_CMD_RES_HEADER_SIZE+1];
    uint8_t msgBufferSize=sizeof(msgBuffer)/sizeof(msgBuffer[0]);
    uint8_t dataSize=msgBufferSize-USART_MSG_CMD_RES_HEADER_SIZE;

    commandHandler_buildCmdResponseHeader(msgBuffer,dataSize,ERROR_TYPE_INVALID_MSG_TYPE);

    uint8_t dataIdx=USART_MSG_CMD_RES_HEADER_SIZE;
    //Send value of invalid msg type in response data
    msgBuffer[dataIdx++]=msg->buffer[USART_MSG_MSG_TYPE_POS];

    usart_writeBuf(msgBuffer,msgBufferSize);
}

void commandHandler_buildCmdResponseHeader(uint8_t* buf, uint8_t dataSize, enum ERROR_TYPE errType)
{
    buf[0]=MSG_TYPE_CMD_RESPOND;
    buf[1]=errType;
    buf[2]=dataSize;
}

static void commandHandler_handleRequestCmd()
{
    commandHandler_MsgS* msg=commandHandler_getMsg();
    uint8_t cmdType=msg->buffer[USART_MSG_CMD_REQ_CMD_TYPE_POS];
    uint8_t dataSize=msg->buffer[USART_MSG_DATA_SIZE_POS];

    switch(cmdType)
    {
        case CMD_TYPE_SET_SEGMENT_DISPLAY_NUM:
        {
            uint8_t msgResponse[4];
            uint8_t responseSize=sizeof(msgResponse)/sizeof(msgResponse[0]);
            uint8_t dataSize=responseSize-USART_MSG_CMD_RES_HEADER_SIZE;
            commandHandler_buildCmdResponseHeader(msgResponse,dataSize,ERROR_TYPE_OK);
            uint8_t dataIdx=USART_MSG_CMD_RES_HEADER_SIZE;
            msgResponse[dataIdx]=msg->buffer[dataIdx];
            usart_writeBuf(msgResponse,responseSize);
            break;
        }

        case CMD_TYPE_SETTINGS_SET_TARGET_ADDR:
        {
            if (dataSize<=CONFIG_CONN_ADDR_LENGTH)
            {
                config_CoreConfigS* coreCfg=config_getConfig();
                memcpy(&coreCfg->connAddr[0],&msg->buffer[USART_MSG_CMD_RES_HEADER_SIZE],dataSize);
                coreCfg->connAddrLen=dataSize;
                commandHandler_sendCmdOkResNoData(CMD_TYPE_SETTINGS_SET_TARGET_ADDR);
            }
            else
            {
                commandHandler_handleInvalidDataSize(CONFIG_CONN_ADDR_LENGTH,dataSize);
            }

            break;
        }

        case CMD_TYPE_SETTINGS_GET_TARGET_ADDR:
        {
            config_CoreConfigS* coreCfg=config_getConfig();
            uint8_t msgBuf[USART_MSG_CMD_RES_OK_BUF_SIZE + CONFIG_CONN_ADDR_LENGTH];
            uint8_t msgBufSize=USART_MSG_CMD_RES_OK_BUF_SIZE+coreCfg->connAddrLen;

            commandHandler_buildCmdResponseHeader(msgBuf,(1+coreCfg->connAddrLen),ERROR_TYPE_OK);
            msgBuf[USART_MSG_CMD_RES_HEADER_SIZE]=CMD_TYPE_SETTINGS_GET_TARGET_ADDR;
            memcpy(&msgBuf[USART_MSG_CMD_RES_OK_BUF_SIZE],&coreCfg->connAddr[0],coreCfg->connAddrLen);

            usart_writeBuf(msgBuf,msgBufSize);

            break;
        }

        case CMD_TYPE_SETTINGS_SAVE:
        {
            config_saveConfig();
            break;
        }
    }
}

static commandHandler_MsgS* commandHandler_getMsg()
{
    return &commandHandler_msgStorage;
}

static void commandHandler_handleMsg()
{
    commandHandler_MsgS* msg=commandHandler_getMsg();
    uint8_t msgType=msg->buffer[USART_MSG_MSG_TYPE_POS];

    switch (msgType)
    {
        case MSG_TYPE_CMD_REQUEST:
        commandHandler_handleRequestCmd();
        break;
        default:
        commandHandler_handleInvalidMsgType();
        break;
    }

    //Release msg storage so new message can be received
    commandHandler_resetMsg();
}

static void commandHandler_handleSystemBusy()
{
    uint8_t msgBuffer[USART_MSG_CMD_RES_HEADER_SIZE];
    uint8_t msgBufferSize=sizeof(msgBuffer)/sizeof(msgBuffer[0]);
    uint8_t dataSize=USART_MSG_CMD_RES_HEADER_SIZE-msgBufferSize;
    commandHandler_buildCmdResponseHeader(msgBuffer,dataSize,ERROR_TYPE_SYSTEM_BUSY);
    usart_writeBuf(msgBuffer,msgBufferSize);
}

static void commandHandler_resetMsg()
{
    memset(&commandHandler_msgStorage,0,sizeof(commandHandler_msgStorage));
}

static void commandHandler_onUsartDataReceived(uint8_t data)
{
    commandHandler_MsgS* msg=commandHandler_getMsg();

    if (false==msg->ready)
    {
        msg->receieveInProgress=true;
        msg->buffer[msg->bufferPos]=data;
        //Data has arrived so reset timeout counter
        msg->msgTimeoutTimerOvfCount=0;

        if (msg->bufferPos==USART_MSG_DATA_SIZE_POS)
        {
            msg->pendingDataCount=msg->buffer[USART_MSG_DATA_SIZE_POS];

            if (msg->pendingDataCount>USART_MSG_DATA_SIZE_MAX)
            {
                //Data size exceeded, response with error
                uint8_t dataSize=1;
                uint8_t resMsgBuf[USART_MSG_CMD_RES_HEADER_SIZE+dataSize];
                uint8_t resMsgBufSize=sizeof(resMsgBuf)/sizeof(resMsgBuf[0]);
                commandHandler_buildCmdResponseHeader(resMsgBuf,dataSize,ERROR_TYPE_DATA_SIZE_TOO_BIG);
                uint8_t dataIdx=USART_MSG_CMD_RES_HEADER_SIZE;
                //Send number of data sent in request
                resMsgBuf[dataIdx]=msg->pendingDataCount;
                usart_writeBuf(resMsgBuf,resMsgBufSize);
                commandHandler_resetMsg();
            }
        }

        if (msg->bufferPos>=USART_MSG_DATA_SIZE_POS)
        {
            if ((msg->bufferPos>USART_MSG_DATA_SIZE_POS) && (msg->pendingDataCount>0))
            {
                --msg->pendingDataCount;
            }

            if(0==msg->pendingDataCount)
            {
                msg->receieveInProgress=false;
                msg->ready=true;
            }
        }

        if (false==msg->ready)
        {
            ++msg->bufferPos;
        }
    }
    else
    {
        /* New data received while receive buffer has not been released.
        Send error message (system busy)
        */
        commandHandler_handleSystemBusy();
    }
}

void commandHandler_timerInit()
{
    //256 prescaler for 8-bit timer 0
    TCCR0B|=1<<CS02;
    //Enable interrupt on overflow
    TIMSK0|=1<<TOIE0;
}

ISR(TIMER0_OVF_vect)
{
    commandHandler_MsgS* msg=commandHandler_getMsg();

    if (true==msg->receieveInProgress)
    {
        ++msg->msgTimeoutTimerOvfCount;
        /*
        Number of times counter has overflowed that
        indicates a command timeout (around 0.5 seconds)
        */
        uint8_t commandTimeoutCounter=122;

        if (commandTimeoutCounter==msg->msgTimeoutTimerOvfCount)
        {
            commandHandler_onCommandTimeout();
        }
    }
}

static void commandHandler_onCommandTimeout()
{
    commandHandler_resetMsg();
    uint8_t msgBuffer[USART_MSG_CMD_RES_HEADER_SIZE];
    uint8_t msgBufferSize=sizeof(msgBuffer)/sizeof(msgBuffer[0]);
    uint8_t dataSize=msgBufferSize-USART_MSG_CMD_RES_HEADER_SIZE;
    commandHandler_buildCmdResponseHeader(msgBuffer,dataSize,ERROR_TYPE_MSG_TIMEOUT);
    usart_writeBuf(msgBuffer,msgBufferSize);
}

void commandHandler_init()
{
    commandHandler_timerInit();
    usart_registerDataRecvHandler(&commandHandler_onUsartDataReceived);
}

static void commandHandler_handleInvalidDataSize(uint8_t expectedDataSize,uint8_t actualDataSize)
{
    uint8_t msgBuf[USART_MSG_CMD_RES_HEADER_SIZE+2];
    uint8_t msgBufSize=sizeof(msgBuf)/sizeof(msgBuf[0]);
    uint8_t dataSize=msgBufSize-USART_MSG_CMD_RES_HEADER_SIZE;
    uint8_t dataIdx=USART_MSG_CMD_RES_HEADER_SIZE;
    commandHandler_buildCmdResponseHeader(msgBuf,dataSize,ERROR_TYPE_INVALID_DATA_SIZE);
    msgBuf[dataIdx++]=expectedDataSize;
    msgBuf[dataIdx]=actualDataSize;
    usart_writeBuf(msgBuf,msgBufSize);
}

void commandHandler_run()
{
    commandHandler_MsgS* msg=commandHandler_getMsg();

    if (true==msg->ready)
    {
        commandHandler_handleMsg();
    }
}

void commandHandler_sendCmdOkResNoData(enum CMD_TYPE requestedCmdType)
{
    uint8_t msgBuffer[USART_MSG_CMD_RES_OK_BUF_SIZE];
    uint8_t msgBufferSize=sizeof(msgBuffer)/sizeof(msgBuffer[0]);
    uint8_t dataSize=msgBufferSize-USART_MSG_CMD_RES_HEADER_SIZE;

    commandHandler_buildCmdResponseHeader(msgBuffer,dataSize,ERROR_TYPE_OK);

    uint8_t dataIdx=USART_MSG_CMD_RES_HEADER_SIZE;
    msgBuffer[dataIdx]=requestedCmdType;

    usart_writeBuf(msgBuffer,msgBufferSize);
};