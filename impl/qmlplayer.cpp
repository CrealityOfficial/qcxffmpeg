#include "cxffmpeg/qmlplayer.h"
#include <QPainter>
#include <QTimer>
#include <QAudioOutput>
#include "VideoDecoder.h"

QMLPlayer::QMLPlayer(QQuickItem *parent)
    :QQuickPaintedItem(parent)
{
    m_decoderController = new VideoDecoderController(this);
    connect(m_decoderController, &VideoDecoderController::videoFrameDataReady, this, &QMLPlayer::onVideoFrameDataReady);
    connect(m_decoderController, &VideoDecoderController::videoFrameDataFinish, this, &QMLPlayer::onVideoFrameDataFinish);
    m_linkState = false;
}

QMLPlayer::~QMLPlayer()
{
    if (m_decoderController)
    {
        delete m_decoderController;
    }
    
}

void QMLPlayer::paint(QPainter *painter)
{
    if (!image.isNull())
    {
        int imageH = image.height();
        int imageW = image.width();
        int screenH = this->height();
        int screenW = this->width();

        int scaledH = this->height();
        int scaledW = this->width();

        int offsetX = 0;
        int offsetY = 0;

        if (imageW > imageH)
        {
            scaledH = imageH * screenW / imageW;
            if (scaledH > screenH)
            {
                scaledH = screenH;
                scaledW = imageW * scaledH / imageH;

                offsetX = (screenW - scaledW) / 2;
                offsetY = 0;
            }
            else
            {
                offsetX = 0;
                offsetY = (screenH - scaledH) / 2;
            }
        }
        else
        {
            scaledW = imageW * scaledH / imageH;
            if (scaledW > screenW)
            {
                scaledW = screenW;
                scaledH = imageH * screenW / imageW;
                offsetX = 0;
                offsetY = (screenH - scaledH) / 2;
            }
            else
            {
                offsetX = (screenW - scaledW) / 2;
                offsetY = 0;
            }
        }


        QImage img = image.scaled(scaledW, scaledH);
        painter->drawImage(QPoint(offsetX, offsetY), img);
    }
    else
    {
        image = QImage(this->width(), this->height(), QImage::Format_RGB888);
        image.fill(QColor(0.0, 0.0, 0.0));
        QImage img = image.scaled(this->width(), this->height());
        painter->drawImage(QPoint(0, 0), img);
    }
}

void QMLPlayer::rowVideoData(unsigned char *data, int width, int height)
{
    image = QImage(data, width, height, QImage::Format_RGB888);
}

void QMLPlayer::rowAudioData(unsigned char *data, unsigned int size)
{
    audioOutputIO->write((const char *)data,size);
}

QString QMLPlayer::getUrl() const
{
    return url;
}

void QMLPlayer::setUrl(const QString &value)
{
    url = value;
}

void QMLPlayer::start(QString urlStr)
{
    QTimer* timer = new QTimer(this);
    timer->setInterval(40);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start();

    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    QAudioOutput *audio_output = new QAudioOutput(format,this);
    audioOutputIO = (audio_output)->start();
    int size = audio_output->periodSize();
    emit m_decoderController->load(urlStr);

}

void QMLPlayer::stop()
{
    qDebug() << "QMLPlayer::stop()";
    m_decoderController->stopplay();
}

void QMLPlayer::onVideoFrameDataReady(unsigned char* data, int width, int height)
{
    //qDebug() << "onVideoFrameDataReady data.width:"<< width << " data.height:"<< height;
    rowVideoData(data, width, height);
    m_linkState = true;

    emit sigVideoFrameDataReady();
}

void QMLPlayer::onVideoFrameDataFinish()
{
    for (int i = 0; i < 10; i++)
    {
        image.fill(QColor(0.0, 0.0, 0.0));
    }
}

bool QMLPlayer::getLinkState()
{
    return m_linkState;
}
