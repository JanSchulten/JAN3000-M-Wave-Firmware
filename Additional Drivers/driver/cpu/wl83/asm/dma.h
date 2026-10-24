#ifndef __ASM_DMA_H__
#define __ASM_DMA_H__

/// \cond DO_NOT_DOCUMENT
#define DEFAULT_INTERNAL_NODE_CNT 2       ///< 内部默认copy任务节点数量
#define DMA_DEFAULT_LEN_THRESHOLD  1024*8 ///< 默认len阈值大小。
/// \endcond


/**
 * \name dma copy task execute status return
 * \{
 */
typedef enum {
    DMA_EXEC_STATUS_NULL,       ///< NULL
    DMA_EXEC_STATUS_COMPLETE,   ///< 执行完成,未出错
    DMA_EXEC_STATUS_ERROR,      ///< 执行出错
} dma_execute_status_t;
/* \} name */


/**
 * \name dma copy task user config format
 * \{
 */
typedef struct {
    void *src_addr;             ///< 任务源地址
    void *dst_addr;             ///< 任务目标地址
    int len;                    ///< 待copy数据长度
} dma_user_task_t;
/* \} name */



/**@brief  打开dma copy模块
* @return  0:打开模块完成 -EFAULT:打开失败
*/
int dma_copy_open(void);

/**@brief  关闭dma copy模块
*/
void dma_copy_close(void);

/**@brief    单任务的同步dma copy
* @param dst 任务的目标地址
* @param src 任务的源地址
* @param len 数据长度
* @return    任务执行情况
*/
dma_execute_status_t dma_copy_sync(void *dst, void *src, int len);

/**@brief    单任务的异步dma copy函数
* @param dst 任务的目标地址。
* @param src 任务的源地址。
* @param len 数据长度
*/
void dma_copy_async(void *dst, void *src, int len);

/**@brief       单任务异步dma copy，可返回任务执行情况。
* @param dst    任务的目标地址
* @param src    任务的源地址
* @param len    数据长度
* @param irq_cb 中断回调函数。用于驱动返回dma当次task执行情况(如果在中断/临界区里调用，回调函数无效)
*/
void dma_copy_async_with_execute_status(void *dst, void *src, int len, void (*irq_cb)(dma_execute_status_t));

/**@brief   等待异步dma copy结束
*/
void dma_copy_async_wait(void);

/**@brief     多任务dma copy函数，内部调用异步dma copy。
* @param list 任务列表
* @param task_cnt 任务数量
* @return  0:copy任务配置完成  -EINVAL:输入参数错误
*/
int dma_task_list_copy(dma_user_task_t *list, int task_cnt);

/**@brief  获取len阈值。同步的dma_copy_sync()会对比任务的len和len阈值大小来判断是否在等待copy过程中释放cpu。
* @return  len阈值
*/
u32 dma_get_len_threshold(void);

/**@brief    设置len阈值。同步的dma_copy_sync()会对比任务的len和len阈值大小来判断是否在等待copy过程中释放cpu。
* @param len 要设置的len阈值
* @return  0:设置成功  -EINVAL:设置失败
*/
int dma_set_len_threshold(int len);

/**@brief  获取内部task节点数量
* @return  内部task节点数量
*/
u32 dma_get_internal_node_cnt(void);

/**@brief    设置内部task节点
* @param cnt 节点数量
* @return    0:设置成功  -EINVAL:设置失败
*/
int dma_set_internal_node_cnt(int cnt);

/**@brief     dma方式实现异步memset，将指定地址范围空间设置为同一个byte数据
* @param dst  目标空间地址
* @param byte 设置的数据
* @param len  目标空间长度
*/
void dma_memset_async(void *dst, u8 byte, int len);

/**@brief     dma方式实现同步memset，将指定地址范围空间设置为同一个byte数据
* @param dst  目标空间首地址
* @param byte 设置的数据
* @param len  目标空间长度
* @return     任务执行情况
*/
dma_execute_status_t dma_memset_sync(void *dst, u8 byte, int len);

#endif
