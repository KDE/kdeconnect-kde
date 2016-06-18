/*
    Copyright (C) 2014 Alexandr Akulich <akulichalexander@gmail.com>

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "textchannel.h"

#include <TelepathyQt/Constants>
#include <TelepathyQt/RequestableChannelClassSpec>
#include <TelepathyQt/RequestableChannelClassSpecList>
#include <TelepathyQt/Types>

#include <QLatin1String>
#include <QVariantMap>

#include <QDebug>

ConnectTextChannel::ConnectTextChannel(QObject *connection, Tp::BaseChannel *baseChannel, uint targetHandle, const QString &identifier)
    : Tp::BaseChannelTextType(baseChannel),
      m_connection(connection),
      m_identifier(identifier)
{
    QStringList supportedContentTypes = QStringList() << "text/plain";
    Tp::UIntList messageTypes = Tp::UIntList() << Tp::ChannelTextMessageTypeNormal;

    uint messagePartSupportFlags = 0;
    uint deliveryReportingSupport = 0;

    m_messagesIface = Tp::BaseChannelMessagesInterface::create(this,
                                                               supportedContentTypes,
                                                               messageTypes,
                                                               messagePartSupportFlags,
                                                               deliveryReportingSupport);

    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(m_messagesIface));

    m_messagesIface->setSendMessageCallback(Tp::memFun(this, &ConnectTextChannel::sendMessageCallback));
}

ConnectTextChannelPtr ConnectTextChannel::create(QObject *connection, Tp::BaseChannel *baseChannel, uint targetHandle, const QString &identifier)
{
    return ConnectTextChannelPtr(new ConnectTextChannel(connection, baseChannel, targetHandle, identifier));
}

ConnectTextChannel::~ConnectTextChannel()
{
}

QString ConnectTextChannel::sendMessageCallback(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error)
{
    QString content;
    for (Tp::MessagePartList::const_iterator i = messageParts.begin()+1; i != messageParts.end(); ++i) {
        if(i->count(QLatin1String("content-type"))
            && i->value(QLatin1String("content-type")).variant().toString() == QLatin1String("text/plain")
            && i->count(QLatin1String("content")))
        {
            content = i->value(QLatin1String("content")).variant().toString();
            break;
        }
    }

    QMetaObject::invokeMethod(m_connection, "messageReceived", Q_ARG(QString, m_identifier), Q_ARG(QString, content));

    return QString();
}
