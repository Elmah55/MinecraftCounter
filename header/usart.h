/*
 * usart.h
 *
 * Created: 29-May-23 10:03:15 PM
 *  Author: Elmah
 */

#ifndef USART_H_
#define USART_H_

#include <stdint.h>
#include <stddef.h>

void usart_init();

void usart_writeByte(const uint8_t byte);

void usart_writeBuf(const uint8_t *buf, size_t size);

void usart_writeString(const char *string);

void usart_registerDataRecvHandler(void (*onUsartDataRecv)(uint8_t));

#endif /* USART_H_ */