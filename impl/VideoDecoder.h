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
#include <QImage>

class VideoDecoder : public QObject
{
    Q_OBJECT
public:
    void stopplay();

    void startPlay(const QString& file);
    
signals:
    void videoInfoReady(int width, int height, int format);
    void videoFrameDataReady(QImage data);
    void videoFrameDataFinish();
protected:
    void readframe();
private:
    AVFormatContext* m_fmtCtx = nullptr;
    AVCodecContext* m_videoCodecCtx = nullptr;
    AVStream* m_videoStream = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    int m_videoStreamIndex = 0;

    AVPixelFormat m_pixFmt;
    int m_width, m_height;
    bool isStop = false;
};

class VideoDecoderController : public QObject
{
    Q_OBJECT
public:
    VideoDecoderController(QObject* parent = nullptr);
    ~VideoDecoderController();

    void startThread(const QString& serverAddress);
    void stopThread();
    void stopplay();

signals:

    void videoInfoReady(int width, int height, int format);
    void videoFrameDataReady(QImage data);

public slots:
    void videoFrameDataFinish();
    void onVideoFrameDataReady(QImage data);

private:
    VideoDecoder* m_decoder = nullptr;
};

#endif // ! PLAYER_FFMPEG_VIDEO_DECODER_H_
