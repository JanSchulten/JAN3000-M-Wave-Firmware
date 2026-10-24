#ifndef __ASM_CAN_H__
#define __ASM_CAN_H__

#include "generic/typedef.h"
#include "generic/ioctl.h"

/**
 * \name CAN dev_ioctl funciton selest
 * \{
 */
#define CAN_MAGIC								'C'
#define IOCTL_CAN_SET_DMA_FRAMES				_IOW(CAN_MAGIC,1,u8)				///<  设置can-dma缓存数据, 基础can不能设置大于6，增强型can不能设置大于4，can dma中断将根据该设置生效，没有接收到对应帧数无法起中断。
#define IOCTL_CAN_SET_INTERRUPT_ENABLE			_IOW(CAN_MAGIC,2,can_event_isr_t)	///<  中断使能
#define IOCTL_CAN_SET_INTERRUPT_DISABLE			_IOW(CAN_MAGIC,3,can_event_isr_t)	///<  中断失能
#define IOCTL_CAN_SET_MODE						_IOW(CAN_MAGIC,4,can_mode_t *)		///<  修改can参数
#define IOCTL_CAN_SET_FILTER					_IOW(CAN_MAGIC,5,can_rx_filter_t *)	///<  修改can参数
#define IOCTL_CAN_SET_RATE						_IOW(CAN_MAGIC,6,can_baudrate_t *)	///<  修改can参数
#define IOCTL_CAN_SET_RECV_WAIT_SEM  			_IO(CAN_MAGIC,7)                    ///<  阻塞式接收-使用信号量,占用中断
#define IOCTL_CAN_SET_RECV_WAIT_WHILE			_IO(CAN_MAGIC,8)                    ///<  阻塞式接收-使用查询中断标志位，内部会清除中断
#define IOCTL_CAN_SET_RECV_NON_BLOCK_ENABLE		_IOR(CAN_MAGIC,9,can_data_t *)		///<  非阻塞式接收-使用中断接收，需要配置IOCTL_CAN_SET_IRQ_CB
#define IOCTL_CAN_SET_RECV_NON_BLOCK_DISABLE	_IO(CAN_MAGIC,10)					///<  非阻塞式接收-使用中断接收，需要配置IOCTL_CAN_SET_IRQ_CB
#define IOCTL_CAN_SET_IRQ_CB					_IOW(CAN_MAGIC,11,can_cb_t *)		///<  非阻塞式接收会占用一个can接收中断，需要配置回调函数进行接收数据获取
#define IOCTL_CAN_SET_ERROR_ALARM_CNT			_IOW(CAN_MAGIC,12,u8)				///<  设置错误报警计数值，当达到预设值后会给STATUS_CALLBACKE的bit6置1
#define IOCTL_CAN_GET_INTERRUPT_SOURCE			_IOR(CAN_MAGIC,13,int *)			///<  获取中断源
#define IOCTL_CAN_GET_STATUS_CALLBACK			_IOR(CAN_MAGIC,14,int *)			///<  获取当前状态
#define IOCTL_CAN_GET_ARBITRATION_LOSS			_IOR(CAN_MAGIC,15,int *)			///<  获取仲裁丢失位
#define IOCTL_CAN_GET_ERROR_CAPTURE				_IOR(CAN_MAGIC,16,int *)			///<  错误捕获
#define IOCTL_CAN_GET_RX_ERROR_CNT				_IOR(CAN_MAGIC,17,u8 *)				///<  获取接收错误计数
#define IOCTL_CAN_GET_TX_ERROR_CNT				_IOR(CAN_MAGIC,18,u8 *)				///<  获取发送错误计数
#define IOCTL_CAN_GET_PUT_REGISTER_INFO			_IO(CAN_MAGIC,19)					///<  寄存器参数读取
#define IOCTL_CAN_SET_RESTART                   _IO(CAN_MAGIC,20)                   ///<  CAN软重启
/* \} name */


/**
 * \name CAN event interrupt
 * \{
 */
typedef enum {
    CAN_EVENT_CLEAR_INTERRUPT 				= BIT(0),	///<  清除中断标志位
    CAN_EVENT_RECEIVE_INTERRUPT 			= BIT(1),	///<  接收中断
    CAN_EVENT_TRANSMIT_INTERRUPT 			= BIT(2),	///<  发送中断
    CAN_EVENT_ERROR_INTERRUPT 				= BIT(3),	///<  错误中断
    CAN_EVENT_DATA_OVERFLOW_INTERRUPT	 	= BIT(4),	///<  数据溢出中断
    CAN_EVENT_ALL_INTERRUPT_DISABLE 		= BIT(5),	///<  关闭所有中断
    CAN_EVENT_ERROR_PASSIVE_INTERRUPT 		= BIT(6),	///<  被动错误中断
    CAN_EVENT_ARBITRATION_LOST_INTERRUPT 	= BIT(7),	///<  仲裁丢失中断
    CAN_EVENT_BUS_ERROR_INTERRUPT 			= BIT(8),	///<  总线错误中断
} can_event_isr_t;
/* \} name */

/**
 * \name CAN event status
 * \{
 */
typedef enum {
    CAN_EVENT_RXFIFO_HAVE_DATA_STATUS 				= BIT(0),	///<  接收缓冲器中有数据
    CAN_EVENT_RECEIVE_DATA_OVERFLOW_STATUS			= BIT(1),	///<  接收数据已溢出
    CAN_EVENT_TRANSMISSION_BUFFER_RELEASE_STATUS	= BIT(2),	///<  发送缓冲器已释放
    CAN_EVENT_TRANSMIT_END_STATUS					= BIT(3),	///<  发送请求已处理完毕
    CAN_EVENT_RECEIVING_DATA_STATUS					= BIT(4),	///<  正在接收数据
    CAN_EVENT_SENDING_DATA_STATUS					= BIT(5),	///<  正在发送数据
    CAN_EVENT_ERROR_OVERFLOW_STATUS					= BIT(6),	///<  错误计数器达到设置的报警限制值
    CAN_EVENT_BUS_IS_IDLE_STATUS					= BIT(7),	///<  总线空闲状态
} can_event_status_t;
/* \} name */

/**
 * \name CAN mode
 * \{
 */
typedef enum {
    CAN_MODE_BASICCAN = 0,			///<  标准模式
    CAN_MODE_ENHANCED,				///<  增强模式
    CAN_MODE_MAX
} can_mode_t;
/* \} name */

/**
 * \name CAN filter mode
 * \{
 */
typedef enum {
    CAN_FILTER_DOUBLE_STAND = 0, 	///<  2 x 16bit filters
    CAN_FILTER_DOUBLE_EXTEND,    	///<  2 x 16bit filters
    CAN_FILTER_SINGLE_STAND,     	///<  1 x 32bit filter
    CAN_FILTER_SINGLE_EXTEND,    	///<  1 x 32bit filter
    CAN_FILTER_STAND_AND_EXTEND,    ///<  2 x 16bit filters 1-STAND(id1) 1-EXTEND(id0)
    CAN_FILTER_MAX
} can_filter_t;
/* \} name */

/**
 * \name CAN data format
 * \{
 */
typedef enum {
    CAN_REQUEST_DATA_TYPE = 0,		///<  数据帧
    CAN_REQUEST_REMOTE_TYPE,		///<  远程帧
    CAN_REQUEST_MAX
} can_request_type_t;
/* \} name */

/**
 * \name CAN frame format
 * \{
 */
typedef enum {
    CAN_FRAME_STANDARD_FORMAT = 0,	///<  标准帧
    CAN_FRAME_EXTENDED_FORMAT,		///<  扩展帧
    CAN_FRAME_MAX
} can_frame_format_t;
/* \} name */

/**
 * \name CAN baudrate
 * \{
 */
typedef struct {
    u16 brp			: 9;				///<  分频系数
    u16 sjw			: 2;				///<  再同步补偿最大宽度，节点与总线再同步每一bit最多允许的补偿偏差
    u16 aligned0	: 5;				///<  仅对齐作用
    u8 sam			: 1;				///<  采样次数:0-采样1次;1-采样3次
    u8 tseg2		: 3;				///<  PBS2
    u8 tseg1		: 4;				///<  PBS1+PTS
} can_baudrate_t;
/* \} name */

/**
 * \name CAN filter id
 * \{
 */
typedef struct {
    can_filter_t  	filter_mode;	///<  ecan 双接收滤波器模/
    u8 accept_rtr0	: 1; 			///<  接收的rtr0
    u8 accept_rtr1	: 1; 			///<  接收的rtr1
    u8 mask_rtr0	: 1; 			///<  接收rtr0的屏蔽位
    u8 mask_rtr1	: 1; 			///<  接收rtr1的屏蔽位
    u8 aligned		: 4; 			///<  仅对齐作用
    u32 accept_id0;					///<  接收的id0;扩展帧在双滤波器开启的状态下仅支持ID的高16位，即bit 28-bit 13
    u32 accept_id1;					///<  接收的id1;扩展帧在双滤波器开启的状态下仅支持ID的高16位，即bit 28-bit 13
    u32 mask_id0;  					///<  接收id0的屏蔽位
    u32 mask_id1;  					///<  接收id1的屏蔽位
} can_rx_filter_t;
/* \} name */

/**
 * \name CAN data
 * \{
 */
typedef struct {
    u32 id;
    u8 dlc			: 4;
    u8 data_format	: 1; 			///<  标准帧/扩展帧标记位
    u8 rtr			: 1;
    u8 crc_en       : 1;
    u8 aligned		: 1;
    u8 data[8];
    u16 crc;
} can_data_t;
/* \} name */

/**
 * \name CAN platform data
 * \{
 */
struct can_platform_data {
    int rx_pin;
    int tx_pin;
    u8	stm;						///<  增强型can自检测模式使能
    u8	lom;						///<  增强型can监听模式使能
    u8 priority;					///<  can中断优先级-用于阻塞式接收-信号量/非阻塞式接收
    u8 cpu_id; 						///<  can中断注册cpu-用于阻塞式接收-信号量/非阻塞式接收
    can_mode_t 		can_mode;		///<  can 模式
    can_baudrate_t 	baudrate;		///<  波特率设置
    can_rx_filter_t filter;			///<  滤波器ID设置
};
/* \} name */

/**
 * \name CAN irq callback
 * \{
 */
typedef struct {
    int (*cb_func)(void *priv, void *hdl, can_event_isr_t event_isr);
    void *cb_priv;
} can_cb_t;
/* \} name */

/**
 * \name CAN inside addr
 * \{
 */
typedef struct {
    u32 *rxdata_addr;
    int *rxcnt_addr;
} can_addr_t;
/* \} name */

//*********************************************************************************************//
//                                      波特率计算公式                                         //
//                              波特率默认值1MHZ，默认lsb频率48M                               //
///                         rate = (lsb_clk)/((TSEG1+TSEG2+3)*(BRP+1)*2)                      ///
//    请根据实际项目中lsb时钟频率修改对应的采样点系数以及分频系数。TSEG1=PTS_PBS1,TSEG2=PBS2   //
//*********************************************************************************************//

/********************************************************************/
/*							错误信息查询表格						*/
//0b00011			0x03			帧起始
//0b00010			0x02			ID BIT 28-21
//0b00110			0x06			ID BIT 20-18
//0b00100			0x04			SRTR位
//0b00101			0x05			IDE位
//0b00111			0x07			ID BIT 17-13
//0b01111			0x0F			ID BIT 12-5
//0b01110			0x0E			ID BIT 4-0
//0b01100			0x0C			RTR位
//0b01101			0x0D			保留位1
//0b01001			0x09			保留位0
//0b01011			0x0B			数据长度代码
//0b01010			0x0A			数据区
//0b01000			0x08			CRC序列
//0b11000			0x18			CRC界定符
//0b11001			0x19			应答
//0b11011			0x1B			应答界定
//0b11010			0x1A			帧结束
//0b10010			0x12			中止
//0b10001			0x11			活动错误标志
//0b10110			0x16			消极错误标志
//0b10011			0x13			控制位误差
//0b10111			0x17			错误定义符
//0b11100			0x1C			溢出标志
/********************************************************************/

/*************************************************************************************/


/// \cond DO_NOT_DOCUMENT
#define CAN_PLATFORM_DATA_BEGIN(can_data) \
    static const struct can_platform_data can_data = { \

#define CAN_PLATFORM_DATA_END() 	};
/// \endcond

extern const struct device_operations can_dev_ops;

#endif
