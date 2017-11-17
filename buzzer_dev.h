/*
 * buzzer_dev.h
 *
 *  Created on: Oct 27, 2017
 *      Author: thinhnguyen
 */

#ifndef BUZZER_DEV_H_
#define BUZZER_DEV_H_

#define IOCTL_SET_REPEAT		0
#define IOCTL_SET_FREQ			1
#define IOCTL_START				2

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
void buzzerdev_set_repeat(struct buzzer *buzz, unsigned int times);

#endif /* BUZZER_DEV_H_ */
