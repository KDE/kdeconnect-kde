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

#ifndef CONNECTCM_TEXTCHANNEL_H
#define CONNECTCM_TEXTCHANNEL_H

#include "connectcm_export.h"

#include <TelepathyQt/BaseChannel>

class ConnectTextChannel;

typedef Tp::SharedPtr<ConnectTextChannel> ConnectTextChannelPtr;

class CONNECTCM_EXPORT ConnectTextChannel : public Tp::BaseChannelTextType
{
    Q_OBJECT
public:
    static ConnectTextChannelPtr create(QObject *connection, Tp::BaseChannel *baseChannel, uint targetHandle, const QString &identifier);
    virtual ~ConnectTextChannel();

    QString sendMessageCallback(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error);

private:
    ConnectTextChannel(QObject *connection, Tp::BaseChannel *baseChannel, uint targetHandle, const QString &identifier);

    QObject *m_connection;

    QString m_identifier;

    Tp::BaseChannelTextTypePtr m_channelTextType;
    Tp::BaseChannelMessagesInterfacePtr m_messagesIface;

};

#endif // CONNECTCM_TEXTCHANNEL_H
