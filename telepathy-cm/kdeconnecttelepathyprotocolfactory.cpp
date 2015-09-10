#include <TelepathyQt/Types>
#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Types>
#include <TelepathyQt/ConnectionManager>

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/Account>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingAccount>
#include <TelepathyQt/AccountSet>

#include <QtDBus/QDBusConnection>
#include "protocol.h"

#include "kdeconnecttelepathyprotocolfactory.h"

Tp::WeakPtr<KDEConnectTelepathyProtocol> KDEConnectTelepathyProtocolFactory::s_interface;

ConnectProtocolPtr KDEConnectTelepathyProtocolFactory::interface() {
    if (s_interface.isNull()) {
        Tp::registerTypes();
        Tp::enableDebug(true);
        Tp::enableWarnings(true);

        ConnectProtocolPtr protocol = Tp::BaseProtocol::create<KDEConnectTelepathyProtocol>(
                QDBusConnection::sessionBus(),
                QLatin1String("kdeconnect"));
        s_interface = protocol;

        static Tp::BaseConnectionManagerPtr cm = Tp::BaseConnectionManager::create(
                QDBusConnection::sessionBus(), QLatin1String("kdeconnect"));

        protocol->setConnectionManagerName(cm->name());
        protocol->setEnglishName(QLatin1String("KDE Connect"));
        protocol->setIconName(QLatin1String("kdeconnect"));
        protocol->setVCardField(QLatin1String("phone_number"));

        cm->addProtocol(protocol);
        cm->registerObject();
        
        //fake being a client and create an account to use this connection
        //maybe this should be per device.. with a device ID as a parameter, but lets keep it connect for now
        
        Tp::AccountManagerPtr am = Tp::AccountManager::create(QDBusConnection::sessionBus());
        Tp::PendingReady *pr = am->becomeReady();
        QObject::connect(pr, &Tp::PendingOperation::finished, [pr,am]() {
           if (pr->isError()) {
               return;
           }
           Tp::AccountSetPtr accounts = am->accountsByProtocol("kdeconnect");
           if (!accounts) {
               return;
           }
           if (accounts->accounts().isEmpty()) {
                Tp::PendingAccount* pa = am->createAccount("kdeconnect", "kdeconnect", "kdeconnect", QVariantMap(), QVariantMap());
                QObject::connect(pa, &Tp::PendingOperation::finished, pa, [pa](){
                    if (pa->isError() || !pa->account()) {
                        return;
                    }
                    pa->account()->setEnabled(true);
                    pa->account()->setRequestedPresence(Tp::Presence::available());
                });
           } else {
               Tp::AccountPtr account = accounts->accounts().first();
               account->setRequestedPresence(Tp::Presence::available());
           }
        });             
    }
    return s_interface.toStrongRef();
}
