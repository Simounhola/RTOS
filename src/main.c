#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
int init_leds(void);
int led_state = 0; 

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec yellow = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);


// Red led thread initialization
#define STACKSIZE 500
#define PRIORITY 5

void red_led_task1(void *, void *, void*);
void yellow_led_task2(void *, void *, void*);
void green_led_task3(void *, void *, void*);

K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task1,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task2,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task3,NULL,NULL,NULL,PRIORITY,0,0);

// Main program
int main(void)
{
	init_leds();

	return 0;
}

// Initialize leds
int init_leds(void) {
	int ret;

	// Led pin initialization

	ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: red Led configure failed %d\n", ret);		
		return ret;
	}
	ret = gpio_pin_configure_dt(&yellow, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: yellow Led configure failed %d\n", ret);		
		return ret;
	}
	ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: green Led configure failed %d\n", ret);		
		return ret;
	}
	// set led off
	
	gpio_pin_set_dt(&red,0);
	gpio_pin_set_dt(&yellow,0);
    gpio_pin_set_dt(&green,0);

	printk("Leds initialized ok\n");
	
	return 0;
}

void red_led_task1(void *, void *, void*) {
	
	printk("Red led thread started\n");
	while (true) {
		if (led_state == 0) {
		gpio_pin_set_dt(&red,1);
		k_msleep(1000);
		printk("Red on\n");
		//k_msleep(1000);
		gpio_pin_set_dt(&red,0);
		printk("Red off\n");
		led_state = 1;
		}
        k_msleep(1000);	
		}
}

void yellow_led_task2(void *, void *, void*) {

	printk("yellow led thread started\n");
	while (true) {
		if (led_state == 1) {
		k_msleep(1000);
		gpio_pin_set_dt(&green,1);
		gpio_pin_set_dt(&red,1);
		printk("yellow on\n");
		k_msleep(1000);
		gpio_pin_set_dt(&green,0);
		gpio_pin_set_dt(&red,0);
		printk("Yellow off\n");
		led_state = 2;
		}
	    k_msleep(1000);
	}
}

void green_led_task3(void *, void *, void*) {
	
	printk("green led thread started\n");
	while (true) {
		if (led_state == 2) {
		gpio_pin_set_dt(&green,1);
		printk("Green on\n");
		k_msleep(1000);
		gpio_pin_set_dt(&green,0);
		printk("Green off\n");
		k_msleep(1000);
		led_state = 0;
		}
	    k_msleep(1000);
	}
}

