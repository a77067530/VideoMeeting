#ifndef WORLD_H
#define WORLD_H

#include<QAudioInput>
#include<QAudioOutput>
#include<QIODevice>
#include<QTimer>
#include<QMessageBox>

//webrtc vad  静音检测
#define USE_VAD (1)
#include "WebRtc_Vad/webrtc_vad.h"



enum ENUM_PLAY_STATE{stopped,playing,pausing};

#include "speex/include/speex.h"
#define SPEEX_QUALITY (8)
#define USE_SPEEX (1)

#endif // WORLD_H
