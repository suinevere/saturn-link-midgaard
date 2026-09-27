#ifndef SATURN_UART16550_H
#define SATURN_UART16550_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t base;
    uint32_t stride;
} saturn_uart16550_t;

#define SATURN_SMPC_COMREG  (*(volatile uint8_t*)0x2010001F)
#define SATURN_SMPC_SF      (*(volatile uint8_t*)0x20100063)

#define SATURN_SMPC_CMD_NEON   0x0A
#define SATURN_SMPC_CMD_NEOFF  0x0B

static inline void saturn_smpc_command(uint8_t cmd) {
    while (SATURN_SMPC_SF & 0x01);
    SATURN_SMPC_SF = 0x01;
    SATURN_SMPC_COMREG = cmd;
    while (SATURN_SMPC_SF & 0x01);
}

static inline void saturn_netlink_smpc_enable(void) {
    saturn_smpc_command(SATURN_SMPC_CMD_NEOFF);
    for (volatile uint32_t i = 0; i < 100000; i++);

    saturn_smpc_command(SATURN_SMPC_CMD_NEON);

    for (volatile uint32_t i = 0; i < 2000000; i++);
}

#define SATURN_NETLINK_QUIRK_ADDR  (*(volatile uint8_t*)0x2582503D)

#define SATURN_NETLINK_POST_ACCESS() \
    do { SATURN_NETLINK_QUIRK_ADDR = 0xFF; } while (0)

#define SATURN_UART_REG_RAW(u, n) \
    (*(volatile uint8_t*)((u)->base + (uint32_t)(n) * (u)->stride))

static inline uint8_t saturn_uart_reg_read(const saturn_uart16550_t* uart,
                                            int reg) {
    uint8_t val = SATURN_UART_REG_RAW(uart, reg);
    SATURN_NETLINK_POST_ACCESS();
    return val;
}

static inline void saturn_uart_reg_write(const saturn_uart16550_t* uart,
                                          int reg, uint8_t val) {
    SATURN_UART_REG_RAW(uart, reg) = val;
    SATURN_NETLINK_POST_ACCESS();
}

#define SATURN_UART_RBR  0
#define SATURN_UART_THR  0
#define SATURN_UART_DLL  0
#define SATURN_UART_IER  1
#define SATURN_UART_DLM  1
#define SATURN_UART_IIR  2
#define SATURN_UART_FCR  2
#define SATURN_UART_LCR  3
#define SATURN_UART_MCR  4
#define SATURN_UART_LSR  5
#define SATURN_UART_MSR  6
#define SATURN_UART_SCR  7

#define SATURN_UART_LSR_DR    0x01
#define SATURN_UART_LSR_THRE  0x20

#define SATURN_UART_LCR_WLS0  0x01
#define SATURN_UART_LCR_WLS1  0x02
#define SATURN_UART_LCR_DLAB  0x80

#define SATURN_UART_LCR_8N1   (SATURN_UART_LCR_WLS0 | SATURN_UART_LCR_WLS1)

#define SATURN_UART_MCR_DTR   0x01
#define SATURN_UART_MCR_RTS   0x02
#define SATURN_UART_MCR_OUT2  0x08

#define SATURN_UART_FCR_ENABLE   0x01
#define SATURN_UART_FCR_RXRESET  0x02
#define SATURN_UART_FCR_TXRESET  0x04

#define SATURN_UART_FCR_INIT \
    (SATURN_UART_FCR_ENABLE | SATURN_UART_FCR_RXRESET | SATURN_UART_FCR_TXRESET)

static inline bool saturn_uart_detect(const saturn_uart16550_t* uart) {
    uint8_t lsr, scr_read;

    lsr = SATURN_UART_REG_RAW(uart, SATURN_UART_LSR);
    if (lsr == 0xFF) return false;

    SATURN_UART_REG_RAW(uart, SATURN_UART_SCR) = 0xA5;
    for (volatile int i = 0; i < 100; i++);
    scr_read = SATURN_UART_REG_RAW(uart, SATURN_UART_SCR);
    if (scr_read != 0xA5) return false;

    SATURN_UART_REG_RAW(uart, SATURN_UART_SCR) = 0x5A;
    for (volatile int i = 0; i < 100; i++);
    scr_read = SATURN_UART_REG_RAW(uart, SATURN_UART_SCR);
    if (scr_read != 0x5A) return false;

    return true;
}

typedef struct {
    uint8_t lsr;
    uint8_t msr;
    uint8_t iir;
    uint8_t scr_a5;
    uint8_t scr_5a;
    bool    detected;
} saturn_uart_detect_result_t;

static inline saturn_uart_detect_result_t
saturn_uart_detect_verbose(const saturn_uart16550_t* uart) {
    saturn_uart_detect_result_t r;

    r.lsr = SATURN_UART_REG_RAW(uart, SATURN_UART_LSR);
    r.msr = SATURN_UART_REG_RAW(uart, SATURN_UART_MSR);
    r.iir = SATURN_UART_REG_RAW(uart, SATURN_UART_IIR);

    SATURN_UART_REG_RAW(uart, SATURN_UART_SCR) = 0xA5;
    for (volatile int i = 0; i < 100; i++);
    r.scr_a5 = SATURN_UART_REG_RAW(uart, SATURN_UART_SCR);

    SATURN_UART_REG_RAW(uart, SATURN_UART_SCR) = 0x5A;
    for (volatile int i = 0; i < 100; i++);
    r.scr_5a = SATURN_UART_REG_RAW(uart, SATURN_UART_SCR);

    r.detected = (r.lsr != 0xFF) && (r.scr_a5 == 0xA5) && (r.scr_5a == 0x5A);
    return r;
}

static inline void saturn_uart_set_baud(const saturn_uart16550_t* uart,
                                         uint16_t divisor) {
    uint8_t lcr = saturn_uart_reg_read(uart, SATURN_UART_LCR);

    saturn_uart_reg_write(uart, SATURN_UART_LCR, lcr | SATURN_UART_LCR_DLAB);

    saturn_uart_reg_write(uart, SATURN_UART_DLL, (uint8_t)(divisor & 0xFF));
    saturn_uart_reg_write(uart, SATURN_UART_DLM, (uint8_t)((divisor >> 8) & 0xFF));

    saturn_uart_reg_write(uart, SATURN_UART_LCR, lcr & ~SATURN_UART_LCR_DLAB);
}

static inline void saturn_uart_init(const saturn_uart16550_t* uart,
                                     uint16_t divisor) {
    saturn_uart_reg_write(uart, SATURN_UART_IER, 0x00);

    saturn_uart_reg_write(uart, SATURN_UART_LCR, SATURN_UART_LCR_8N1);

    if (divisor > 0) {
        saturn_uart_set_baud(uart, divisor);
    }

    saturn_uart_reg_write(uart, SATURN_UART_FCR, SATURN_UART_FCR_INIT);

    saturn_uart_reg_write(uart, SATURN_UART_MCR,
        SATURN_UART_MCR_DTR | SATURN_UART_MCR_RTS | SATURN_UART_MCR_OUT2);

    for (volatile int i = 0; i < 50000; i++);
}

static inline bool saturn_uart_tx_ready(const saturn_uart16550_t* uart) {
    return (saturn_uart_reg_read(uart, SATURN_UART_LSR) &
            SATURN_UART_LSR_THRE) != 0;
}

static inline bool saturn_uart_rx_ready(const saturn_uart16550_t* uart) {
    return (saturn_uart_reg_read(uart, SATURN_UART_LSR) &
            SATURN_UART_LSR_DR) != 0;
}

static inline uint8_t saturn_uart_read_msr(const saturn_uart16550_t* uart) {
    return saturn_uart_reg_read(uart, SATURN_UART_MSR);
}

static inline bool saturn_uart_putc(const saturn_uart16550_t* uart,
                                     uint8_t c) {
    uint32_t timeout = 200000;
    while (!saturn_uart_tx_ready(uart)) {
        if (--timeout == 0) return false;
    }
    saturn_uart_reg_write(uart, SATURN_UART_THR, c);
    return true;
}

static inline uint8_t saturn_uart_getc(const saturn_uart16550_t* uart) {
    while (!saturn_uart_rx_ready(uart));
    return saturn_uart_reg_read(uart, SATURN_UART_RBR);
}

static inline int saturn_uart_getc_timeout(const saturn_uart16550_t* uart,
                                            uint32_t timeout) {
    while (timeout--) {
        if (saturn_uart_rx_ready(uart)) {
            return saturn_uart_reg_read(uart, SATURN_UART_RBR);
        }
    }
    return -1;
}

static inline bool saturn_uart_puts(const saturn_uart16550_t* uart,
                                     const char* str) {
    while (*str) {
        if (!saturn_uart_putc(uart, *str++)) return false;
    }
    return true;
}

static inline void saturn_uart_flush_rx(const saturn_uart16550_t* uart) {
    while (saturn_uart_rx_ready(uart)) {
        (void)saturn_uart_reg_read(uart, SATURN_UART_RBR);
    }
}

#endif
