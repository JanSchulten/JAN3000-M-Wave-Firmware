#include "app_config.h"
#include "system/includes.h"
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"
#include "event/device_event.h"

#if TCFG_USB_SLAVE_AUDIO_ENABLE
#include "usb/device/uac_audio.h"
#include "uac_stream.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[UAC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define UAC_DEBUG_ECHO_MODE 0

static u8 speaker_stream_is_open[USB_MAX_HW_NUM];

struct uac_speaker_handle {
#if !TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    cbuffer_t cbuf;
    void *buffer;
#endif
    u32 samplerate;
    u32 bitwidth;
    u32 last_spk_l_vol;
    u32 last_spk_r_vol;
    volatile u8 need_resume;
    u8 channel;
    u8 alive;
};

static void (*uac_rx_handler[USB_MAX_HW_NUM])(void *, void *, int);
static void *uac_rx_priv[USB_MAX_HW_NUM];

#ifndef UAC_BUFFER_SIZE
#if (SOUNDCARD_ENABLE)
#define UAC_BUFFER_SIZE     (4 * 1024)
#else
#define UAC_BUFFER_SIZE     (2 * 1024)
#endif
#endif

#define UAC_BUFFER_MAX		(UAC_BUFFER_SIZE * 50 / 100)

static struct uac_speaker_handle *uac_speaker[USB_MAX_HW_NUM];

#if USB_MALLOC_ENABLE
#else
static struct uac_speaker_handle uac_speaker_handle[USB_MAX_HW_NUM] SEC(.uac_var);
static u8 uac_rx_buffer[USB_MAX_HW_NUM][UAC_BUFFER_SIZE] ALIGNED(4) SEC(.uac_rx);
#endif

void set_uac_speaker_rx_handler(const usb_dev usb_id, void *priv, void (*rx_handler)(void *, void *, int))
{
    uac_rx_priv[usb_id] = priv;
    uac_rx_handler[usb_id] = rx_handler;
}

void uac_speaker_stream_write(const usb_dev usb_id, const u8 *obuf, u32 len)
{
    if (speaker_stream_is_open[usb_id]) {
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        if (uac_rx_handler[usb_id]) {
            uac_rx_handler[usb_id](uac_rx_priv[usb_id], (void *)obuf, len);
        }
#else
        //write dac
        int wlen = cbuf_write(&uac_speaker[usb_id]->cbuf, (void *)obuf, len);
        if (wlen != len) {
            //putchar('W');
        }
        //if (uac_speaker[usb_id]->rx_handler) {
        if (uac_rx_handler[usb_id]) {
            /* if (uac_speaker[usb_id]->cbuf.data_len >= UAC_BUFFER_MAX) { */
            // 马上就要满了，赶紧取走
            uac_speaker[usb_id]->need_resume = 1; //2020-12-22注:无需唤醒
            /* } */
            if (uac_speaker[usb_id]->need_resume) {
                uac_speaker[usb_id]->need_resume = 0;
                uac_rx_handler[usb_id](uac_rx_priv[usb_id], (void *)obuf, len);
                //uac_speaker[usb_id]->rx_handler(0, (void *)obuf, len);
            }
        }
        uac_speaker[usb_id]->alive = 0;
#endif
    }
}

void uac_speaker_stream_get_volume(const usb_dev usb_id, u16 *l_vol, u16 *r_vol)
{
    uac_get_cur_vol(usb_id, l_vol, r_vol);
}

u8 uac_speaker_stream_status(const usb_dev usb_id)
{
    return speaker_stream_is_open[usb_id];
}

void uac_speaker_stream_open(const usb_dev usb_id, u32 samplerate, u32 ch, u32 bitwidth)
{
    log_info("%s", __func__);

#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    if (speaker_stream_is_open[usb_id]) {
        if (uac_speaker[usb_id]->samplerate != samplerate ||
            uac_speaker[usb_id]->channel != ch ||
            uac_speaker[usb_id]->bitwidth != bitwidth) {
            uac_speaker[usb_id]->samplerate = samplerate;
            uac_speaker[usb_id]->channel = ch;
            uac_speaker[usb_id]->bitwidth = bitwidth;
            /* pc_spk_set_fmt(usb_id, ch, bitwidth, samplerate); */
            struct device_event event = {0};
            event.event = USB_AUDIO_PLAY_RESTART;
            event.value = (int)((ch << 28) | (bitwidth << 20) | samplerate);
            event.arg = (void *)(int)usb_id;
            device_event_notify(DEVICE_EVENT_FROM_UAC, &event);
        }
        return;
    }
#else
    if (speaker_stream_is_open[usb_id]) {
        return;
    }
#endif

    if (!uac_speaker[usb_id]) {
#if USB_MALLOC_ENABLE
        uac_speaker[usb_id] = zalloc(sizeof(struct uac_speaker_handle));
        if (!uac_speaker[usb_id]) {
            return;
        }

#if !TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        uac_speaker[usb_id]->buffer = malloc(UAC_BUFFER_SIZE);
        if (!uac_speaker[usb_id]->buffer) {
            free(uac_speaker[usb_id]);
            uac_speaker[usb_id] = NULL;
            return;
        }
#endif
#else
        uac_speaker[usb_id] = &uac_speaker_handle[usb_id];
#if !TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        memset(uac_speaker[usb_id], 0, sizeof(struct uac_speaker_handle));
        uac_speaker[usb_id]->buffer = uac_rx_buffer[usb_id];
#endif
#endif
        uac_speaker[usb_id]->last_spk_l_vol = (u32) - 1;
        uac_speaker[usb_id]->last_spk_r_vol = (u32) - 1;
    }

    uac_speaker[usb_id]->channel = ch;
    uac_speaker[usb_id]->samplerate = samplerate;
    uac_speaker[usb_id]->bitwidth = bitwidth;
    //uac_speaker[usb_id]->rx_handler = NULL;

#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    /* pc_spk_set_fmt(usb_id, ch, bitwidth, samplerate); */
#else
    cbuf_init(&uac_speaker[usb_id]->cbuf, uac_speaker[usb_id]->buffer, UAC_BUFFER_SIZE);
#endif

    struct device_event event = {0};
    event.event = USB_AUDIO_PLAY_OPEN;
    event.value = (int)((ch << 28) | (bitwidth << 20) | samplerate);
    event.arg = (void *)(int)usb_id;

#if !UAC_DEBUG_ECHO_MODE
    device_event_notify(DEVICE_EVENT_FROM_UAC, &event);
#endif

    speaker_stream_is_open[usb_id] = 1;
}

void uac_speaker_stream_close(const usb_dev usb_id, int release)
{
    log_info("%s", __func__);

    if (speaker_stream_is_open[usb_id] == 0) {
        return;
    }

    speaker_stream_is_open[usb_id] = 0;

    if (uac_speaker[usb_id]) {
#if USB_MALLOC_ENABLE
#if !TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        if (uac_speaker[usb_id]->buffer) {
            free(uac_speaker[usb_id]->buffer);
        }
#endif
        free(uac_speaker[usb_id]);
#endif
        uac_speaker[usb_id] = NULL;
        uac_rx_handler[usb_id] = NULL;
        uac_rx_priv[usb_id] = NULL;
    }

    struct device_event event = {0};
    event.event = USB_AUDIO_PLAY_CLOSE;
    event.arg = (void *)(int)usb_id;
    device_event_notify(DEVICE_EVENT_FROM_UAC, &event);
}

__attribute__((weak))
int uac_get_spk_vol(void)
{
    return 99;
}

static u8 mic_stream_is_open[USB_MAX_HW_NUM];

struct uac_mic_handle {
    int (*mic_tx_handler)(void *, void *, int);
    void *mic_tx_priv;
    u32 last_mic_vol;
    u32 samplerate;
    u32 bitwidth;
    u8  channel;
};

static struct uac_mic_handle uac_mic[USB_MAX_HW_NUM];

void uac_mute_volume(const usb_dev usb_id, u32 type, u32 l_vol, u32 r_vol)
{
    struct device_event event = {0};

    switch (type) {
    case MIC_FEATURE_UNIT_ID: //MIC
        if (mic_stream_is_open[usb_id] == 0) {
            return;
        }
        if (l_vol == uac_mic[usb_id].last_mic_vol) {
            return;
        }
        uac_mic[usb_id].last_mic_vol = l_vol;
        event.event = USB_AUDIO_SET_MIC_VOL;
        break;
    case SPK_FEATURE_UNIT_ID: //SPK
        if (speaker_stream_is_open[usb_id] == 0) {
            return;
        }
        if (l_vol == uac_speaker[usb_id]->last_spk_l_vol && r_vol == uac_speaker[usb_id]->last_spk_r_vol) {
            return;
        }
        uac_speaker[usb_id]->last_spk_l_vol = l_vol;
        uac_speaker[usb_id]->last_spk_r_vol = r_vol;
        event.event = USB_AUDIO_SET_PLAY_VOL;
        break;
    default:
        break;
    }

    event.value = (int)(r_vol << 16 | l_vol);
    event.arg = (void *)(int)usb_id;
    device_event_notify(DEVICE_EVENT_FROM_UAC, &event);
}

int uac_mic_stream_read(const usb_dev usb_id, u8 *buf, u32 len)
{
    if (mic_stream_is_open[usb_id] == 0) {
        return 0;
    }
#if 0//48K 1ksin
    const s16 sin_48k[] = {
        0, 2139, 4240, 6270, 8192, 9974, 11585, 12998,
        14189, 15137, 15826, 16244, 16384, 16244, 15826, 15137,
        14189, 12998, 11585, 9974, 8192, 6270, 4240, 2139,
        0, -2139, -4240, -6270, -8192, -9974, -11585, -12998,
        -14189, -15137, -15826, -16244, -16384, -16244, -15826, -15137,
        -14189, -12998, -11585, -9974, -8192, -6270, -4240, -2139
    };
    u16 *l_ch = (u16 *)buf;
    u16 *r_ch = (u16 *)buf;
    r_ch++;
    for (int i = 0; i < len / 2; i++) {
        *l_ch = sin_48k[i];
        *r_ch = sin_48k[i];
        l_ch += 1;
        r_ch += 1;
    }
    return len;
#elif   UAC_DEBUG_ECHO_MODE
    uac_speaker_read(NULL, buf, len);
#if MIC_CHANNEL == 2
    const s16 sin_48k[] = {
        0, 2139, 4240, 6270, 8192, 9974, 11585, 12998,
        14189, 15137, 15826, 16244, 16384, 16244, 15826, 15137,
        14189, 12998, 11585, 9974, 8192, 6270, 4240, 2139,
        0, -2139, -4240, -6270, -8192, -9974, -11585, -12998,
        -14189, -15137, -15826, -16244, -16384, -16244, -15826, -15137,
        -14189, -12998, -11585, -9974, -8192, -6270, -4240, -2139
    };
    u16 *r_ch = (u16 *)buf;
    r_ch++;
    for (int i = 0; i < len / 4; i++) {
        *r_ch = sin_48k[i];
        r_ch += 2;
    }
#endif
    return len;
#else
    if (uac_mic[usb_id].mic_tx_handler) {
#if 1
        return uac_mic[usb_id].mic_tx_handler(uac_mic[usb_id].mic_tx_priv, buf, len);
#else
        //16bit ---> 24bit
        int rlen = uac_mic[usb_id].mic_tx_handler(uac_mic[usb_id].mic_tx_priv, tmp_buf, len / 3 * 2);
        rlen /= 2; //sampe point
        for (int i = 0 ; i < rlen ; i++) {
            buf[i * 3] = 0;
            buf[i * 3 + 1] = tmp_buf[i * 2];
            buf[i * 3 + 2] = tmp_buf[i * 2 + 1];
        }
#endif
    } else {
        putchar('N');
    }
    return 0;
#endif
    return 0;
}

void set_uac_mic_tx_handler(const usb_dev usb_id, void *priv, int (*tx_handler)(void *, void *, int))
{
    uac_mic[usb_id].mic_tx_handler = tx_handler;
    uac_mic[usb_id].mic_tx_priv = priv;
}

void uac_mic_stream_get_volume(const usb_dev usb_id, u16 *vol)
{
    *vol = uac_get_mic_vol(usb_id);
}

u8 uac_get_mic_stream_status(const usb_dev usb_id)
{
    return mic_stream_is_open[usb_id];
}

u32 uac_mic_stream_open(const usb_dev usb_id, u32 samplerate, u32 frame_len, u32 ch, u32 bitwidth)
{
    log_info("%s", __func__);

    if (mic_stream_is_open[usb_id]) {
#if !TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        if (uac_mic[usb_id].samplerate != samplerate ||
            uac_mic[usb_id].channel != ch ||
            uac_mic[usb_id].bitwidth != bitwidth) {
            uac_mic[usb_id].mic_tx_handler = NULL;	//添加测试
            uac_mic[usb_id].samplerate = samplerate;
            uac_mic[usb_id].bitwidth = bitwidth;
            uac_mic[usb_id].channel = ch;
            struct device_event event = {0};
            event.event = USB_AUDIO_MIC_RESTART;
            event.value = (int)((ch << 28) | (bitwidth << 20) | samplerate);
            event.arg = (void *)(int)usb_id;
            device_event_notify(DEVICE_EVENT_FROM_UAC, &event);
        }
#endif
        return 0;
    }

    memset(&uac_mic[usb_id], 0, sizeof(struct uac_mic_handle));
    uac_mic[usb_id].samplerate = samplerate;
    uac_mic[usb_id].bitwidth = bitwidth;
    uac_mic[usb_id].channel = ch;
    uac_mic[usb_id].last_mic_vol = (u32) - 1;

    struct device_event event = {0};
    event.event = USB_AUDIO_MIC_OPEN;
    event.value = (int)((ch << 28) | (bitwidth << 20) | samplerate);
    event.arg = (void *)(int)usb_id;

#if !UAC_DEBUG_ECHO_MODE
    device_event_notify(DEVICE_EVENT_FROM_UAC, &event);
#endif

    mic_stream_is_open[usb_id] = 1;

    return 0;
}

void uac_mic_stream_close(const usb_dev usb_id, int release)
{
    log_info("%s", __func__);

    if (mic_stream_is_open[usb_id] == 0) {
        return;
    }

    mic_stream_is_open[usb_id] = 0;

    struct device_event event = {0};
    event.event = USB_AUDIO_MIC_CLOSE;
    event.arg = (void *)(int)usb_id;
    device_event_notify(DEVICE_EVENT_FROM_UAC, &event);
}

#endif
