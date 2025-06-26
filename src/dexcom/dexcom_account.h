#include <Arduino.h>


#ifndef __DEXCOM_ACCOUNT_H
#define __DEXCOM_ACCOUNT_H

typedef struct
{
    String username;
    String password;
} DexcomAccount;

#endif