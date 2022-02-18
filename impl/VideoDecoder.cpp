#include "VideoDecoder.h"
#include <QDebug>
/* Enable or disable frame reference counting. You are not supposed to support
 * both paths in your application but pick the one most appropriate to your
 * needs. Look for the use of refcount in this example to see what are the
 * differences of API usage between them. */
static int gRefCount = 0;
static void pgm_save(unsigned char* buf, int wrap, int xsize, int ysize,
    char* filename)
{
    FILE* f;
    int i;

    f = fopen(filename, "w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}
static void outError(int num)
{
    char errorStr[1024];
    av_strerror(num, errorStr, sizeof errorStr);
    qDebug() << "FFMPEG ERROR:" << QString(errorStr);
}
static int open_codec_context(int& streamIndex
    , AVCodecContext** decCtx
    , AVFormatContext* fmtCtx
    , enum AVMediaType type)
{
    int ret;
    int index;
    AVStream* st;
    AVCodec* codec = nullptr;
    AVDictionary* opts = nullptr;
    ret = av_find_best_stream(fmtCtx, type, -1, -1, nullptr, 0);
    if (ret < 0)
    {
        qWarning() << "Could not find stream " << av_get_media_type_string(type);
        return ret;
    }
    index = ret;
    st = fmtCtx->streams[index];
    codec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!codec)
    {
        qWarning() << "Cound not find codec " << av_get_media_type_string(type);
        return AVERROR(EINVAL);
    }
    *decCtx = avcodec_alloc_context3(codec);
    if (!*decCtx)
    {
        qWarning() << "Failed to allocate codec context " << av_get_media_type_string(type);
        return AVERROR(ENOMEM);
    }
    ret = avcodec_parameters_to_context(*decCtx, st->codecpar);
    if (ret < 0)
    {
        qWarning() << "Failed to copy codec parameters to decoder context" << av_get_media_type_string(type);
        return ret;
    }
    av_dict_set(&opts, "refcounted_frames", gRefCount ? "1" : "0", 0);

    ret = avcodec_open2(*decCtx, codec, &opts);
    if (ret < 0)
    {
        qWarning() << "Failed to open codec " << av_get_media_type_string(type);
        return ret;
    }
    streamIndex = index;
    return 0;
}
void VideoDecoder::init()
{
    avformat_network_init();
}

void VideoDecoder::uninit()
{
    avformat_network_deinit();
}

void VideoDecoder::stopplay()
{
    isStop = true;
}

void VideoDecoder::load(const QString& file)
{
    isStop = false;
    int ret = 0;
    AVDictionary* options = NULL;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    ret = avformat_open_input(&m_fmtCtx, file.toStdString().data(), nullptr, &options);
    if (0 > ret)
    {
        qWarning() << "open url error";
        outError(ret);
        return;
    }
    ret = avformat_find_stream_info(m_fmtCtx, nullptr);
    if (0 > ret)
    {
        qWarning() << "find stream failed";
        outError(ret);
        return;
    }
    ret = open_codec_context(m_videoStreamIndex, &m_videoCodecCtx, m_fmtCtx, AVMEDIA_TYPE_VIDEO);
    if (ret < 0)
    {
        qWarning() << "open_codec_context failed";
        return;
    }
    m_videoStream = m_fmtCtx->streams[m_videoStreamIndex];
    m_width = m_videoCodecCtx->width;
    m_height = m_videoCodecCtx->height;
    m_pixFmt = m_videoCodecCtx->pix_fmt;

    emit videoInfoReady(m_width, m_height, m_pixFmt);

    av_dump_format(m_fmtCtx, 0, file.toStdString().data(), 0);
    do {
        if (!m_videoStream)
        {
            qWarning() << "Could not find audio or video stream in the input, aborting";
            break;
        }
        m_frame = av_frame_alloc();
        if (!m_frame)
        {
            qWarning() << "Could not allocate frame";
            break;
        }
        readframe();
    } while (0);
    avcodec_free_context(&m_videoCodecCtx);
    avformat_close_input(&m_fmtCtx);
    av_frame_free(&m_frame);
    emit videoFrameDataFinish();

}



void VideoDecoder::readframe()
{
    AVPacket* pPacket = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();

    int width = m_width;
    int height = m_height;

    SwsContext* sws_conotext = sws_getContext(m_width, m_height, m_pixFmt,
        width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
    AVFrame* yuvFrame = av_frame_alloc();

    int size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    uint8_t* buff = (uint8_t*)av_malloc(size);
    av_image_fill_arrays(yuvFrame->data, yuvFrame->linesize, buff, AV_PIX_FMT_RGB24, width, height, 1);
    yuvFrame->width = width;
    yuvFrame->height = height;
    int16_t* outputBuffer = NULL;
    while (av_read_frame(m_fmtCtx, pPacket) == 0 && !isStop)
    {
        if (pPacket->stream_index == m_videoStreamIndex)
        {
            if (avcodec_send_packet(m_videoCodecCtx, pPacket)) 
            {
                printf("error send packet!");
                continue;
            }
            if (!avcodec_receive_frame(m_videoCodecCtx, pFrame)) 
            {
                sws_scale(sws_conotext, pFrame->data, pFrame->linesize, 0, height, yuvFrame->data, yuvFrame->linesize);
                emit videoFrameDataReady(yuvFrame->data[0], yuvFrame->width, yuvFrame->height);
            }
        }
        av_packet_unref(pPacket);
    }
}

//--------------------------------------------------------------------------------//
VideoDecoderController::VideoDecoderController(QObject* parent) : QObject(parent)
{
    m_decoder = new VideoDecoder;
    m_decoder->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_decoder, &VideoDecoder::deleteLater);
    connect(this, &VideoDecoderController::init, m_decoder, &VideoDecoder::init);
    connect(this, &VideoDecoderController::uninit, m_decoder, &VideoDecoder::uninit);
    connect(this, &VideoDecoderController::load, m_decoder, &VideoDecoder::load);

    connect(m_decoder, &VideoDecoder::videoFrameDataReady, this, &VideoDecoderController::videoFrameDataReady);
    connect(m_decoder, &VideoDecoder::videoFrameDataFinish, this, &VideoDecoderController::videoFrameDataFinish);

    m_thread.start();
    // emit init();
}

VideoDecoderController::~VideoDecoderController()
{
    if (m_thread.isRunning())
    {
        emit uninit();
        m_thread.quit();
        m_thread.wait();
    }
}

void VideoDecoderController::startThread()
{
    //m_thread.start();
    emit init();
}

void VideoDecoderController::stopThread()
{
    //emit uninit();
}

void VideoDecoderController::stopplay()
{
    m_decoder->stopplay();
}