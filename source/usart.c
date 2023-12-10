/*
 * usart.c
 *
 * Created: 29-May-23 10:43:13 PM
 *  Author: Elmah
 */

#include "usart.h"
#include <avr/io.h>
#include <string.h>
#include <avr/interrupt.h>

// Value of baud rate register
#define UBRR_VAL ((F_CPU / (16UL * USART_BAUDRATE)) - 1)
#define USART_BAUDRATE 9600

// Func ptr to data received handler
void (*onUsartDataRecv)(uint8_t data) = NULL;

void usart_init()
{
    // Set baud rate register
    uint16_t baudRateRegValue = UBRR_VAL;
    UBRR0H = (uint8_t)(baudRateRegValue >> 8);
    UBRR0L = (uint8_t)(baudRateRegValue);

    // 8 bit data, no parity bit, 1 stop bit
    UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);

    // Enable TX and RX, interrupt on RX complete
    UCSR0B |= (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
}

void usart_writeBuf(const uint8_t *buf, size_t bufSize)
{
    for (uint16_t i = 0; i < bufSize; i++)
    {
        // Busy waiting until transmit buffer is free
        while (!(UCSR0A & (1 << UDRE0)))
            ;

        UDR0 = buf[i];
    }
}

void usart_writeString(const char *string)
{
    size_t stringSize = strlen(string);
    usart_writeBuf((uint8_t *)string, stringSize);
}

void usart_registerDataRecvHandler(void (*recvHandler)(uint8_t))
{
    onUsartDataRecv = recvHandler;
}

void usart_writeByte(const uint8_t byte)
{
    usart_writeBuf(&byte, sizeof(byte));
}

ISR(USART_RX_vect)
{
    uint8_t recvData = UDR0;

    if (NULL != onUsartDataRecv)
    {
        onUsartDataRecv(recvData);
    }
}