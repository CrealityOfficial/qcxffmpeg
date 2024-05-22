#ifndef PLAYER_FFMPEG_WEBRTC_DECODER_H_
#define PLAYER_FFMPEG_WEBRTC_DECODER_H_
#include <QContiguousCache>
#include <QThread>
#include <QObject>
#include <QImage>
#include <QFuture> 
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif
extern "C"
{
#define __STDC_CONSTANT_MACROS
//#include "video/yangrecordthread.h"
#include "yangplayer/YangPlayerHandle.h"
#include "yangstream/YangStreamType.h"
//#include "yangplayer/YangPlayWidget.h"
#include <yangutil/yangavinfotype.h>
#include <yangutil/sys/YangSysMessageI.h>
#include <yangutil/sys/YangSocket.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangMath.h>

}
class WebRTCDecoder : public QObject, public YangSysMessageI
{
    Q_OBJECT
public:
    void stopplay();
    void startPlay(const QString& strUrl);
    WebRTCDecoder();
    ~WebRTCDecoder();
    void success();
    void failure(int32_t errcode);
    bool isStop() { return m_isStop; }
signals:
    void videoFrameDataReady(QString url, QImage data);
    void videoFrameDataFinish(QString url);
    void RtcConnectFailure(int errcode);
private slots:
    void connectFailure(int errcode);
private:
    bool m_isStop = false;
    YangPlayerHandle* m_player;
    YangFrame m_frame;
    void getRenderData();
protected:
    YangContext* m_context;
    QString m_url;
    QFuture<void> m_playFutrue;
};

#endif // ! PLAYER_FFMPEG_WEBRTC_DECODER_H_