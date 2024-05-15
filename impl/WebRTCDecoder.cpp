#include "WebRTCDecoder.h"
#include <QDebug>
#include <QImage>
#include <QPixmap>
#include <thread>
#include <QtConcurrent>
#include <yangstream/YangSynBuffer.h>

WebRTCDecoder::WebRTCDecoder()
{
    m_context = new YangContext();
    m_context->init();

    m_context->synMgr.session->playBuffer = (YangSynBuffer*)yang_calloc(sizeof(YangSynBuffer), 1);//new YangSynBuffer();
    yang_create_synBuffer(m_context->synMgr.session->playBuffer);

    m_context->avinfo.sys.mediaServer = Yang_Server_P2p;//Yang_Server_Srs/Yang_Server_Zlm
    m_context->avinfo.rtc.rtcSocketProtocol = Yang_Socket_Protocol_Udp;//

    m_context->avinfo.rtc.rtcLocalPort = 10000 + yang_random() % 15000;
    memset(m_context->avinfo.rtc.localIp, 0, sizeof(m_context->avinfo.rtc.localIp));
    yang_getLocalInfo(m_context->avinfo.sys.familyType, m_context->avinfo.rtc.localIp);
    m_context->avinfo.rtc.enableDatachannel = yangfalse;
    m_context->avinfo.rtc.iceCandidateType = YangIceHost;
    m_context->avinfo.rtc.turnSocketProtocol = Yang_Socket_Protocol_Udp;

    m_context->avinfo.rtc.enableAudioBuffer = yangtrue; //use audio buffer
    m_context->avinfo.audio.enableAudioFec = yangfalse; //srs not use audio fec
    m_player = YangPlayerHandle::createPlayerHandle(m_context, this);
}
void WebRTCDecoder::stopplay()
{
    m_isStop = true;
    if (m_player) m_player->stopPlay();
}

void WebRTCDecoder::startPlay(const QString& strUrl)
{
    m_url = strUrl;
    //QString url = "http://172.23.208.238:8000/call/demo";
    m_context->synMgr.session->playBuffer->resetVideoClock(m_context->synMgr.session->playBuffer->session);
    int32_t err = m_player->playRtc(0, m_url.toLatin1().data());
    if (!err)
    {
        QtConcurrent::run([this]() {
            //QThread::msleep(1000);
            while (!this->isStop())
            {
                this->getRenderData();
                QThread::msleep(2);
            }
            
            });
    }
}
unsigned int convertYUVtoRGB(int y, int u, int v) {
    int r, g, b;

    r = y + (int)(1.402f * v);
    g = y - (int)(0.344f * u + 0.714f * v);
    b = y + (int)(1.772f * u);

    r = r > 255 ? 255 : r < 0 ? 0 : r;
    g = g > 255 ? 255 : g < 0 ? 0 : g;
    b = b > 255 ? 255 : b < 0 ? 0 : b;
    return 0xff000000 | (b << 16) | (g << 8) | r;
}
unsigned int* convertYUV420_NV21toRGB8888(unsigned char data[78080], int width, int height) {
    int size = width * height;
    int offset = size;
    unsigned int* pixels = new unsigned int[size];
    int u, v, y1, y2, y3, y4;

    // i percorre os Y and the final pixels
    // k percorre os pixles U e V
    for (int i = 0, k = 0; i < size; i += 2, k += 2) {
        y1 = data[i] & 0xff;
        y2 = data[i + 1] & 0xff;
        y3 = data[width + i] & 0xff;
        y4 = data[width + i + 1] & 0xff;

        u = data[offset + k] & 0xff;
        v = data[offset + k + 1] & 0xff;
        u = u - 128;
        v = v - 128;

        pixels[i] = convertYUVtoRGB(y1, u, v);
        pixels[i + 1] = convertYUVtoRGB(y2, u, v);
        pixels[width + i] = convertYUVtoRGB(y3, u, v);
        pixels[width + i + 1] = convertYUVtoRGB(y4, u, v);

        if (i != 0 && (i + 2) % width == 0)
            i += width;
    }

    return pixels;
}
void WebRTCDecoder::getRenderData()
{
    uint8_t* t_vb = m_context->synMgr.session->playBuffer->getVideoRef(m_context->synMgr.session->playBuffer->session, &m_frame);
    if (t_vb)
    {
        
        YangSynBuffer* sync_buffer = m_context->synMgr.session->playBuffer;
        int width = sync_buffer->width(sync_buffer->session);
        int height = sync_buffer->height(sync_buffer->session);
        //unsigned int* imageData = convertYUV420_NV21toRGB8888(t_vb, width , height);
        //QByteArray byteImage((const char*)image);
        //qDebug() << "receive data"<< byteImage.size();
        int ulndex = width * height;
        int vlndex = ulndex + ((width * height) >> 2);
        QImage image(width,height, QImage::Format_RGB888);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                //Y分量
                double Y = (double)t_vb[y * width + x];
                //U分量
                double U = (double)t_vb[ulndex + (y / 2) * (width / 2) + (x / 2)] - 128;
                //V分量
                double V = (double)t_vb[vlndex + (y / 2) * (width / 2) + (x / 2)] - 128;
                //转换公式
                int R = (int)(Y + 1.13983 * V);
                int G = (int)(Y - 0.39466 * U - 0.58060 * V);
                int B = (int)(Y + 2.03211 * U);
                R = qBound(0, R, 255);
                G = qBound(0, G, 255);
                B = qBound(0, B, 255);
                QRgb rgbValue = qRgb(R, G, B);
                image.setPixel(x, y, rgbValue);
            }
        }
        emit videoFrameDataReady(m_url, image);
        qDebug() << image.width();
        //delete t_vb;
        
    }
}
void WebRTCDecoder::success()
{
   
}
void WebRTCDecoder::failure(int32_t errcode)
{
    emit RtcConnectFailure(errcode);
}
void WebRTCDecoder::connectFailure(int errcode) {

}