#ifndef PLAYER_FFMPEG_VIDEO_DECODER_H_
#define PLAYER_FFMPEG_VIDEO_DECODER_H_

extern "C"
{
#define __STDC_CONSTANT_MACROS

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include<libavcodec/avcodec.h>
#include<libswresample/swresample.h>
}

#include <QContiguousCache>
#include <QThread>
#include <QObject>


class VideoDecoder : public QObject
{
    Q_OBJECT
public:
    void stopplay();

public slots:
    void init();
    void uninit();
    void load(const QString& file);
    
signals:
    void videoInfoReady(int width, int height, int format);
    void videoFrameDataReady(unsigned char* data, int width, int height);
    void videoFrameDataFinish();
protected:
    void demuxing();
    void decodeFrame();
    void readframe();
private:
    AVFormatContext* m_fmtCtx = nullptr;
    AVCodecContext* m_videoCodecCtx = nullptr;
    AVStream* m_videoStream = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket m_packet;
    int m_videoStreamIndex = 0;

    AVPixelFormat m_pixFmt;
    int m_width, m_height;
    bool isStop;
};

class VideoDecoderController : public QObject
{
    Q_OBJECT
public:
    VideoDecoderController(QObject* parent = nullptr);
    ~VideoDecoderController();

    void startThread();
    void stopThread();
    void stopplay();

signals:
    void init();
    void uninit();
    void pause(bool);
    void load(const QString& file);

    void videoInfoReady(int width, int height, int format);
    void videoFrameDataReady(unsigned char* data, int width, int height);
    void videoFrameDataFinish();

private:
    VideoDecoder* m_decoder = nullptr;
    QThread m_thread;
};

#endif // ! PLAYER_FFMPEG_VIDEO_DECODER_H_
