#ifndef __ASM_UART_H__
#define __ASM_UART_H__

#define UART_NUM 			5

/**
 * \name UART parity
 * \{
 */
typedef enum {
    UART_PARITY_DISABLE,    /*!< 关闭奇偶校验 */
    UART_PARITY_EVEN = 2,   /*!< 奇校验 */
    UART_PARITY_ODD = 3,    /*!< 偶校验 */
} uart_parity_t;
/* \} name */

/// \cond DO_NOT_DOCUMENT
#define UART0_PLATFORM_DATA_BEGIN(data) \
	static const struct uart_platform_data data = {

#define UART0_PLATFORM_DATA_END()  \
	.irq 					= IRQ_UART0_IDX, \
};

#define UART1_PLATFORM_DATA_BEGIN(data) \
	static const struct uart_platform_data data = {

#define UART1_PLATFORM_DATA_END()  \
	.irq 					= IRQ_UART1_IDX, \
};

#define UART2_PLATFORM_DATA_BEGIN(data) \
	static const struct uart_platform_data data = {

#define UART2_PLATFORM_DATA_END()  \
	.irq 					= IRQ_UART2_IDX, \
};

#define UART3_PLATFORM_DATA_BEGIN(data) \
	static const struct uart_platform_data data = {

#define UART3_PLATFORM_DATA_END()  \
	.irq 					= IRQ_UART3_IDX, \
};

#define UART4_PLATFORM_DATA_BEGIN(data) \
	static const struct uart_platform_data data = {

#define UART4_PLATFORM_DATA_END()  \
	.irq 					= IRQ_UART4_IDX, \
};
/// \endcond

#endif

