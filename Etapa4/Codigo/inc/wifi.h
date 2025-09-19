#ifndef WIFI_H
#define WIFI_H

#include "pico/cyw43_arch.h"
#include <stdio.h>

#define FINATECH_SSID "AP-ACCESS BLH"
#define FINATECH_PSWD "Fin@ApointBlH"
#define SSID "Boboy_2.4GHz"
#define PSWD "13zb0276"

int connect_wifi(const char *ssid, const char *password);

#endif