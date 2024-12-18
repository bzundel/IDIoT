#include <stdio.h>

#include "clk.h"
#include "board.h"
#include "periph_conf.h"
#include "timex.h"
#include "ztimer.h"
#include "dht_params.h"
#include "dht.h"

static void delay(int seconds)
{
    ztimer_sleep(ZTIMER_USEC, seconds * US_PER_SEC);
}

int main(void)
{
    dht_params_t sensor_params;
    sensor_params.pin = GPIO_PIN(0, 21);
    sensor_params.type = DHT11;
    sensor_params.in_mode = DHT_PARAM_PULL;

    dht_t dev;
    if (dht_init(&dev, &sensor_params) != DHT_OK) {
        puts("Failed to connect to sensor.");
    }
    puts("Connected to sensor. Waiting for initial delay...");

    delay(3); //delay required, otherwise dht_read/3 returns ENODEV

    int16_t temp, hum;
    while (1) {
        int result = dht_read(&dev, &temp, &hum);

        if (result != DHT_OK) {
            puts("An error occured trying to read from sensor.");
        }

        printf("Temp.: %d.%d°C, Hum.: %d.%d%%\n", temp / 10, temp % 10, hum / 10, hum % 10);

        delay(5);
    }

    return 0;
}
