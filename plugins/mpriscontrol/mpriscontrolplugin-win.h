#ifndef MPRISCONTROLPLUGINWIN_H
#define MPRISCONTROLPLUGINWIN_H

#include <core/kdeconnectplugin.h>

#include <QString>
#include <QLoggingCategory>

#define PLAYERNAME QStringLiteral("Media Player")

#define PACKET_TYPE_MPRIS QStringLiteral("kdeconnect.mpris")

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_PLUGIN_MPRIS)

class MprisControlPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

  public:
    explicit MprisControlPlugin(QObject *parent, const QVariantList &args);

    bool receivePacket(const NetworkPacket &np) override;
    void connected() override {}

  private:
    const QString playername = "Media Player";
};
#endif //MPRISCONTROLPLUGINWIN_H