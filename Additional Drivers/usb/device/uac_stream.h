#ifndef _UAC_STREAM_H_
#define _UAC_STREAM_H_

#include "usb/device/uac_audio.h"

enum uac_event {
    USB_AUDIO_PLAY_OPEN = 0x0,
    USB_AUDIO_PLAY_CLOSE,
    USB_AUDIO_PLAY_RESTART,
    USB_AUDIO_MIC_OPEN,
    USB_AUDIO_MIC_CLOSE,
    USB_AUDIO_MIC_RESTART,
    // USB_AUDIO_MUTE,
    USB_AUDIO_SET_PLAY_VOL,
    USB_AUDIO_SET_MIC_VOL,
};

#endif
