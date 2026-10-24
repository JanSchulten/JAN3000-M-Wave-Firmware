#ifndef __POWER_PORT_H__
#define __POWER_PORT_H__

typedef enum {
    PORTA = 0,
    PORTB,
    PORTC,
    PORTD,
    PORTE,
    PORTF,
    PORTFUSB,
    PORTHUSB,
} gpio_port_t;

//A B C D E F USBA USBB
#define PORT_TABLE(arg)         u32 gpio_confi##arg[8] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff}

void port_protect(u32 *gpio_config, u32 gpio);

#define PORT_PROTECT(gpio)      port_protect(gpio_config, gpio)

void *__port_init(u32 arg);

void gpio_close(u32 port, u32 val);

#endif
