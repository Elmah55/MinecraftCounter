/*
 * commandHandler.h
 *
 * Created: 31-May-23 10:14:22 PM
 *  Author: Elmah
 */ 


#ifndef COMMANDHANDLER_H_
#define COMMANDHANDLER_H_

#include <stdbool.h>

#define USART_MSG_BUFFER_SIZE 50
#define USART_MSG_DATA_SIZE_MAX (USART_MSG_BUFFER_SIZE-2)

#define USART_MSG_MSG_TYPE_POS 0
#define USART_MSG_DATA_SIZE_POS 2

#define USART_MSG_CMD_REQ_CMD_TYPE_POS 1

#define USART_MSG_CMD_RES_ERROR_TYPE_POS 1
#define USART_MSG_CMD_RES_HEADER_SIZE 3
#define USART_MSG_CMD_RES_OK_BUF_SIZE (USART_MSG_CMD_RES_HEADER_SIZE+1)

enum MSG_TYPE
{
    MSG_TYPE_CMD_REQUEST,
    MSG_TYPE_CMD_RESPOND
};

enum CMD_TYPE
{
    CMD_TYPE_SETTINGS_SET_TARGET_ADDR,
    CMD_TYPE_SETTINGS_GET_TARGET_ADDR,
    CMD_TYPE_SETTINGS_GET_QUERY_FREQ,
    CMD_TYPE_SETTINGS_SET_QUERY_FREQ,
    CMD_TYPE_SETTINGS_RESET,
    CMD_TYPE_SETTINGS_SAVE,
    CMD_TYPE_SET_SEGMENT_DISPLAY_NUM, //TEMP - FOR TEST ONLY
    CMD_TYPE_MAX_VALUE
};

enum ERROR_TYPE
{
    ERROR_TYPE_OK,
    ERROR_TYPE_INVALID_MSG_TYPE,
    ERROR_TYPE_DATA_SIZE_TOO_BIG,
    ERROR_TYPE_INVALID_DATA_SIZE,
    ERROR_TYPE_INVALID_CMD_TYPE,
    ERROR_TYPE_MSG_TIMEOUT,
    ERROR_TYPE_SYSTEM_BUSY,
    ERROR_TYPE_STORAGE_BUSY,
    ERROR_TYPE_MAX_VALUE
};

typedef struct commandHandler_MsgS
{
    /*
    Buffer used for storing incoming message
    */
    uint8_t buffer[USART_MSG_BUFFER_SIZE];
    /*
    Indicates position of last valid element in USART receive buffer
    */
    uint8_t bufferPos;
    /*
    Number of bytes of command data that is expected to arrive
    */
    uint8_t pendingDataCount;
    /*
    True while reception of this message has not been completed
    */
    bool receieveInProgress;
    /*
    Stores number of times command timeout timer overflowed
    */
    uint8_t msgTimeoutTimerOvfCount;
    /*
    True if message is ready to be handled
    */
    bool ready;
}commandHandler_MsgS;

void commandHandler_init();
/*
Command handler logic run every iteration of main loop
*/
void commandHandler_run();
void commandHandler_buildCmdResponseHeader(uint8_t* buf, uint8_t dataSize,enum ERROR_TYPE errType);
/*
Sends confirmation of successful execution of requested command
*/
void commandHandler_sendCmdOkResNoData(enum CMD_TYPE requestedCmdType);

#endif /* COMMANDHANDLER_H_ */