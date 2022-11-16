#include "VideoDecoder.h"
#include <QDebug>
#include <QImage>

void VideoDecoder::stopplay()
{
    isStop = true;
}

void VideoDecoder::startPlay(const QString& file)
{
	auto url = file.toStdString();
	unsigned int    i;
	int             ret;
	int             video_st_index = -1;
	int             audio_st_index = -1;
	AVFormatContext* ifmt_ctx = NULL;
	AVPacket        pkt;
	AVStream* st = NULL;
	char            errbuf[64];
	AVDictionary* optionsDict = NULL;
	av_register_all();                                                          // Register all codecs and formats so that they can be used.
	avformat_network_init();                                                    // Initialization of network components
	av_dict_set(&optionsDict, "rtsp_transport", "tcp", 0);                //����tcp����	,,��������������Щrtsp���ͻῨ��
	av_dict_set(&optionsDict, "stimeout", "2000000", 0);                  //���û������stimeout

	av_init_packet(&pkt);                                                       // initialize packet.
	pkt.data = NULL;
	pkt.size = 0;
	bool nRestart = false;
	AVStream* pVst;
	uint8_t* buffer_rgb = NULL;
	AVCodecContext* pVideoCodecCtx = NULL;
	AVCodecParameters* pVideoCodecPar = NULL;
	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pFrameRGB = av_frame_alloc();
	int got_picture;
	SwsContext* img_convert_ctx = NULL;
	AVCodec* pVideoCodec = NULL;

	if ((ret = avformat_open_input(&ifmt_ctx, url.c_str(), 0, &optionsDict)) < 0) {            // Open the input file for reading.
		goto EXIT;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {                // Get information on the input file (number of streams etc.).
		goto EXIT;
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                // dump information
		av_dump_format(ifmt_ctx, i, url.c_str(), 0);
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                // find video stream index
		st = ifmt_ctx->streams[i];
		switch (st->codecpar->codec_type) {
		case AVMEDIA_TYPE_AUDIO: audio_st_index = i; break;
		case AVMEDIA_TYPE_VIDEO: video_st_index = i; break;
		default: break;
		}
	}
	if (-1 == video_st_index) {
		goto EXIT;
	}

	while (!isStop)
	{
		do {
			ret = av_read_frame(ifmt_ctx, &pkt);                                // read frames
#if 1
			//decode stream
			if (!nRestart)
			{
				pVst = ifmt_ctx->streams[video_st_index];

				pVideoCodecPar = pVst->codecpar;

				//pVideoCodecCtx = pVst->codec;
				pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);
				avcodec_parameters_to_context(pVideoCodecCtx, pVideoCodecPar);

				pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
				if (pVideoCodec == NULL)
					return;
				//pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);

				if (avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0)
					return;
				nRestart = true;
			}

			if (pkt.stream_index == video_st_index)
			{
				int av_result = avcodec_send_packet(pVideoCodecCtx, &pkt);
				got_picture = avcodec_receive_frame(pVideoCodecCtx, pFrame);
				if (got_picture)
				{
				}

				if (av_result < 0)
				{
					return;
				}
#if 1
				if (got_picture == 0)
				{
					int bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pVideoCodecCtx->width, pVideoCodecCtx->height, 1);
					buffer_rgb = (uint8_t*)av_malloc(bytes);
					av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer_rgb, AV_PIX_FMT_RGB24, pVideoCodecCtx->width, pVideoCodecCtx->height, 1);

					img_convert_ctx = sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height, pVideoCodecCtx->pix_fmt,
						pVideoCodecCtx->width, pVideoCodecCtx->height, AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
					if (img_convert_ctx == NULL)
					{

						return;
					}
					sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pVideoCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

					QImage img(buffer_rgb, pVideoCodecCtx->width, pVideoCodecCtx->height, QImage::Format_RGB888, [](void* data) {
						av_free(data);
						}, buffer_rgb);
					emit videoFrameDataReady(img);

					sws_freeContext(img_convert_ctx);
					//av_free(buffer_rgb);
				}
#endif
			}

#endif
		} while (ret == AVERROR(EAGAIN));

		if (ret < 0) {
			goto EXIT;
		}

		if (pkt.stream_index == video_st_index) {                               // video frame
		}
		else if (pkt.stream_index == audio_st_index) {                         // audio frame
		}
		else {
		}

		av_packet_unref(&pkt);
	}

EXIT:

	if (NULL != ifmt_ctx) {
		avformat_close_input(&ifmt_ctx);
		ifmt_ctx = NULL;
	}

	return;
}

void VideoDecoder::readframe()
{
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
    while (true)
    {
        if (isStop)
        {
            break;
        }
        if (av_read_frame(m_fmtCtx, m_packet) < 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (m_packet->stream_index == m_videoStreamIndex)
        {
            if (avcodec_send_packet(m_videoCodecCtx, m_packet))
            {
                printf("error send packet!");
                continue;
            }
            if (!avcodec_receive_frame(m_videoCodecCtx, m_frame))
            {
                sws_scale(sws_conotext, m_frame->data, m_frame->linesize, 0, height, yuvFrame->data, yuvFrame->linesize);
            }
        }
        av_packet_unref(m_packet);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    av_frame_free(&yuvFrame);
    //av_packet_free(&m_packet);
}

//--------------------------------------------------------------------------------//
VideoDecoderController::VideoDecoderController(QObject* parent) : QObject(parent)
{
    m_decoder = new VideoDecoder;

    connect(m_decoder, &VideoDecoder::videoFrameDataReady, this, &VideoDecoderController::onVideoFrameDataReady);
    connect(m_decoder, &VideoDecoder::videoFrameDataFinish, this, &VideoDecoderController::videoFrameDataFinish);
}

VideoDecoderController::~VideoDecoderController()
{
    stopThread();
}

void VideoDecoderController::startThread(const QString& serverAddress)
{
    auto t = std::thread(&VideoDecoder::startPlay, m_decoder, serverAddress);
    t.detach();
}

void VideoDecoderController::stopThread()
{
    stopplay();
}

void VideoDecoderController::stopplay()
{
    m_decoder->stopplay();
}

void VideoDecoderController::onVideoFrameDataReady(QImage data)
{
    emit videoFrameDataReady(data);
}

void VideoDecoderController::videoFrameDataFinish()
{
    delete m_decoder;
}