/*
 * buzzer_dev.h
 *
 *  Created on: Oct 27, 2017
 *      Author: thinhnguyen
 */

#ifndef BUZZER_DEV_H_
#define BUZZER_DEV_H_

struct buzzer
{
	unsigned int gpio_pin;
	unsigned int freq;
	unsigned int repeat;
};

void buzzerdev_init(struct buzzer *buzz);
void buzzerdev_deinit(struct buzzer *buzz);
void buzzerdev_set_freq(struct buzzer *buzz, unsigned int freq);
void buzzerdev_on(struct buzzer *buzz);
void buzzerdev_off(struct buzzer *buzz);

#endif /* BUZZER_DEV_H_ */
