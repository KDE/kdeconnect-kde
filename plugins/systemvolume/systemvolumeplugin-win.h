#ifndef SYSTEMVOLUMEPLUGINWIN_H
#define SYSTEMVOLUMEPLUGINWIN_H

#include <QObject>
#include <QMap>

#include <core/kdeconnectplugin.h>

#include <Windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

#define PACKET_TYPE_SYSTEMVOLUME QStringLiteral("kdeconnect.systemvolume")
#define PACKET_TYPE_SYSTEMVOLUME_REQUEST QStringLiteral("kdeconnect.systemvolume.request")

class Q_DECL_EXPORT SystemvolumePlugin : public KdeConnectPlugin
{
    Q_OBJECT

  public:
    explicit SystemvolumePlugin(QObject *parent, const QVariantList &args);
    ~SystemvolumePlugin();
    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;

  private:
    class CMMNotificationClient;
    class CAudioEndpointVolumeCallback;

    bool valid;
    IMMDeviceEnumerator* deviceEnumerator;
    CMMNotificationClient* deviceCallback;
    QMap<QString, QPair<IAudioEndpointVolume *, CAudioEndpointVolumeCallback *>> sinkList;

    bool sendSinkList();
};

#endif // SYSTEMVOLUMEPLUGINWIN_H
