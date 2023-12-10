/*
 * main.c
 *
 * Created: 29-May-23 09:48:58 PM
 *  Author: Elmah
 */

#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "usart.h"
#include "commandHandler.h"
#include "config.h"

#define SOFT_VERSION "1.0"

static void init()
{
    usart_init();
    commandHandler_init();
    config_init();

    // Turn off built-in LED
    DDRB |= 1 << DDB5;

    sei();

    char initMsg[] = {"Minecraft Counter version " SOFT_VERSION " initialized\n"};
    usart_writeString(initMsg);
}

int main()
{
    init();

    while (true)
    {
        commandHandler_run();
    }

    return EXIT_SUCCESS;
}