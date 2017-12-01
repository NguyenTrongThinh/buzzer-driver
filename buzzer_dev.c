/*
 * buzzer_dev.c
 *
 *  Created on: Oct 27, 2017
 *      Author: thinhnguyen
 */


#include "buzzer_dev.h"
#include <linux/gpio.h>
#include <linux/stddef.h>

void buzzerdev_init(struct buzzer *buzz)
{
	gpio_request(buzz->gpio_pin, "BUZZER");
	gpio_direction_output(buzz->gpio_pin, 1);
}
void buzzerdev_deinit(struct buzzer *buzz)
{
	gpio_free(buzz->gpio_pin);
}
void buzzerdev_set_freq(struct buzzer *buzz, unsigned int freq)
{
	buzz->freq = freq;
}
void buzzerdev_set_repeat(struct buzzer *buzz, unsigned int times)
{
	buzz->repeat = times;
}
void buzzerdev_on(struct buzzer *buzz)
{
	gpio_set_value(buzz->gpio_pin, buzz->activestate);
}
void buzzerdev_off(struct buzzer *buzz)
{
	gpio_set_value(buzz->gpio_pin, (buzz->activestate ^ 1));
}

void buzzerdev_set_active_state(struct buzzer *buzz, unsigned int state)
{
	buzz->activestate = state;
}
