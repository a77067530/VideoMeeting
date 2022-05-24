#include "audiowrite.h"

AudioWrite::AudioWrite(QObject *parent) : QObject(parent)
{
    //speex初始化
    speex_bits_init(&bits_dec);
    Dec_State = speex_decoder_init(speex_lib_get_mode(SPEEX_MODEID_NB));
    //声卡采样格式
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);
    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    if (!info.isFormatSupported(format)) {
        QMessageBox::information(NULL , "提示", "打开音频设备失败");
        format = info.nearestFormat(format);
    }

    m_audio_out = new QAudioOutput(format,this);
    //向buffer里面存数据，就会触发声音
    m_buffer_out = m_audio_out->start();

}

AudioWrite::~AudioWrite()
{
    if(m_audio_out)
    {
        m_audio_out->stop();
        delete m_audio_out;
        m_audio_out = NULL;
    }
}

void AudioWrite::slot_playAudio(QByteArray ba)
{
#ifdef USE_SPEEX
    char bytes[800] = {0};
    int i = 0;
    float output_frame1[320] = {0};
    char buf[800] = {0};
    memcpy(bytes,ba.data(),ba.length() / 2);
    //解压缩数据106 62
    //speex_bits_reset(&bits_dec);
    speex_bits_read_from(&bits_dec,bytes,ba.length() / 2);
    int error = speex_decode(Dec_State,&bits_dec,output_frame1);
    //将解压后数据转换为声卡识别格式
    //大端
    short num = 0;
    for (i = 0;i < 160;i++)
    {
        num = output_frame1[i];
        buf[2 * i] = num;
        buf[2 * i + 1] = num >> 8;
        //qDebug() << "float out" << num << output_frame1[i];
    }
    memcpy(bytes,ba.data() + ba.length() / 2,ba.length() / 2);
    //解压缩数据
    //speex_bits_reset(&bits_dec);
    speex_bits_read_from(&bits_dec,bytes,ba.length() / 2);
    error = speex_decode(Dec_State,&bits_dec,output_frame1);
    //将解压后数据转换为声卡识别格式
    //大端
    for (i = 0;i < 160;i++)
    {
        num = output_frame1[i];
        buf[2 * i + 320] = num;
        buf[2 * i + 1 + 320] = num >> 8;
    }
    m_buffer_out->write(buf,640);
    return;
#else
    if(ba.size() < 640)return;
    m_buffer_out->write( ba.data() , 640 );
#endif
}
