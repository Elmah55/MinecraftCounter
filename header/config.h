/*
 * config.h
 *
 * Created: 31-May-23 10:44:42 PM
 *  Author: Elmah
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdbool.h>
#include <stdint.h>

#define CONFIG_CONN_ADDR_LENGTH 30

typedef struct config_CoreConfigS
{
    /* Connection address for minecraft server
     */
    char connAddr[CONFIG_CONN_ADDR_LENGTH];
    /*
     */
    uint8_t connAddrLen;
    /*
    Indicates how often (in seconds) target
    server should be queried
    */
    uint8_t queryFreq;
} config_CoreConfigS;

config_CoreConfigS *config_getConfig();
bool config_saveConfig();
void config_init();

#endif /* CONFIG_H_ */