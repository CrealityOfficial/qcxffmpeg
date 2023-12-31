﻿#ifndef QMLPLAYER_H
#define QMLPLAYER_H

#include <QQuickPaintedItem>
#include <QImage>
#include "cxffmpeg/interface.h"

class VideoDecoderController;
class PLAYER_FFMPEG_API QMLPlayer : public QQuickPaintedItem
{
    Q_OBJECT
public:
    QMLPlayer(QQuickItem *parent = 0);
    ~QMLPlayer();

    QString getUrl() const;
    void setUrl(const QString &value);

    Q_INVOKABLE void start(QString urlStr);
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool getLinkState();

private:
    void paint(QPainter *painter) override;
    void rowVideoData(unsigned char *data, int width, int height);
    void rowAudioData(unsigned char *data, unsigned int size);

protected slots:
    void onVideoFrameDataReady(unsigned char* data, int width, int height);
    void onVideoFrameDataFinish();

signals:
    void sigVideoFrameDataReady();

private:
    QImage image;
    QString url;
    QIODevice *audioOutputIO;
    VideoDecoderController* m_decoderController;
    bool m_linkState;
};

#endif // QMLPLAYER_H
