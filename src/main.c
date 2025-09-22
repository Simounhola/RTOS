#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

int init_leds(void);

K_CONDVAR_DEFINE(red_signal);
K_CONDVAR_DEFINE(green_signal);
K_CONDVAR_DEFINE(yellow_signal);

K_MUTEX_DEFINE(red_mutex);
K_MUTEX_DEFINE(green_mutex);
K_MUTEX_DEFINE(yellow_mutex);

static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

#define STACKSIZE 500
#define PRIORITY 5

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void dispatcher_task(void *, void *, void*);
void uart_task(void *, void *, void*);

K_SEM_DEFINE(release_sem, 0, 1);

K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);

int red_time = 1000;
int green_time = 1000;
int yellow_time = 1000;

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);


K_FIFO_DEFINE(dispatcher_fifo);

struct data_t {
	
	void *fifo_reserved;
	char msg[20];
};

int init_uart(void) {

	if (!device_is_ready(uart_dev)) {
		return 1;
	} 
	return 0;
}

int main(void)
{
	init_leds();
	int ret = init_uart();
	if (ret != 0) {
		printk("UART initialization failed!\n");
		return ret;
	}

	k_msleep(100);

	printk("Started serial read example\n");


	return 0;
}

int init_leds(void) {
	int ret;

	ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: red Led configure failed %d\n", ret);		
		return ret;
	}
	
	ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: green Led configure failed %d\n", ret);		
		return ret;
	}
	gpio_pin_set_dt(&red,0);
    gpio_pin_set_dt(&green,0);

	printk("Leds initialized ok\n");
	
	return 0;
}

void uart_task(void *, void *, void*) {
    char rc = 0;

    while (true) {
        if (uart_poll_in(uart_dev, &rc) == 0) {
            if (rc == 'R' || rc == 'G' || rc == 'Y') {
                struct data_t *buf = k_malloc(sizeof(struct data_t));
                if (buf == NULL) {
                    printk("Memory allocation failed!\n");
                    continue;
                }
                buf->msg[0] = rc;
                buf->msg[1] = '\0'; 
                k_fifo_put(&dispatcher_fifo, buf);

                printk("UART received: %c\n", rc);
            }
        }
        k_msleep(10);
    }
}

void dispatcher_task(void *, void *, void *) {
    while (true) {
        struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
        char color = rec_item->msg[0];
        k_free(rec_item);

        printk("Dispatcher received: %c\n", color);
        k_msleep(1000);
        if (color == 'R') {
            k_condvar_signal(&red_signal);
        } else if (color == 'G') {
            k_condvar_signal(&green_signal);
        } else if (color == 'Y') {
            k_condvar_signal(&yellow_signal);
        } else {
            printk("Unknown color code: %c\n", color);
            continue;
        }
        k_sem_take(&release_sem, K_FOREVER);
    }
}

void red_led_task(void *, void *, void*) {
    while (true) {

        k_condvar_wait(&red_signal, &red_mutex, K_FOREVER);
        gpio_pin_set_dt(&red, 1);
        k_msleep(red_time); 
        gpio_pin_set_dt(&red, 0);
        printk("Red thread runs\n");
        k_sem_give(&release_sem);
    }
}

void yellow_led_task(void *, void *, void*) {
    while (true) {

        k_condvar_wait(&yellow_signal, &yellow_mutex, K_FOREVER);
        gpio_pin_set_dt(&red, 1);
		gpio_pin_set_dt(&green, 1);
        k_msleep(yellow_time);
        gpio_pin_set_dt(&red, 0);
		gpio_pin_set_dt(&green, 0);
        printk("Yellow thread runs\n");
        k_sem_give(&release_sem);
    }
}

void green_led_task(void *, void *, void*) {
    while (true) {
    
        k_condvar_wait(&green_signal, &green_mutex, K_FOREVER);
        gpio_pin_set_dt(&green, 1);
        k_msleep(green_time); 
        gpio_pin_set_dt(&green, 0);
        printk("Green thread runs\n");
        k_sem_give(&release_sem);
    }
}
