/**
 * Copyright 2019 Aleix Pol Gonzalez
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "clipboardlistenerwayland.h"
#include <KWayland/Client/registry.h>
#include <KWayland/Client/datacontroldevice.h>
#include <KWayland/Client/seat.h>
#include <KWayland/Client/connection_thread.h>
#include <QDebug>
#include <QMimeType>
#include <QMimeData>

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

using namespace KWayland::Client;

ClipboardListenerWayland::ClipboardListenerWayland()
{
    auto registry = new Registry(this);

    m_waylandConnection = ConnectionThread::fromApplication(this);
    if (!m_waylandConnection) {
        qWarning() << "Failed getting Wayland connection from QPA";
        return;
    }
    registry->create(m_waylandConnection);
    registry->setup();

    const auto ifaces = registry->interfaces(KWayland::Client::Registry::Interface::DataControlDeviceManager);
    connect(registry, &Registry::dataControlDeviceManagerAnnounced, this, [this, registry](quint32 name, quint32 version) {
        m_manager = registry->createDataControlDeviceManager(name, version, this);
        m_source = m_manager->createDataSource(this);
        connect(m_source, &DataControlSource::sendDataRequested, this, &ClipboardListenerWayland::sendData);
    });

    connect(registry, &Registry::interfacesAnnounced, this, [this] {
        Q_ASSERT(m_seat);
        m_dataControlDevice = m_manager->getDataDevice(m_seat, this);
        connect(m_dataControlDevice, &DataControlDevice::selectionOffered, this, &ClipboardListenerWayland::selectionReceived);
    });
}

const QString textMimetype = QStringLiteral("text/plain");

void ClipboardListenerWayland::setText(const QString& content)
{
    if (m_currentContent == content)
        return;

    m_currentContent = content;
    m_source->offer(textMimetype);
}

void ClipboardListenerWayland::sendData(const QString& mimeType, qint32 fd)
{
    Q_ASSERT(mimeType == textMimetype);

    QByteArray content = m_currentContent.toUtf8();
    if (!content.isEmpty()) {
        // Create a sigpipe handler that does nothing, or clients may be forced to terminate
        // if the pipe is closed in the other end.
        struct sigaction action, oldAction;
        action.sa_handler = SIG_IGN;
        sigemptyset (&action.sa_mask);
        action.sa_flags = 0;

        sigaction(SIGPIPE, &action, &oldAction);
        write(fd, content.constData(), content.size());
        sigaction(SIGPIPE, &oldAction, nullptr);
    }
    close(fd);
}

static inline int qt_safe_pipe(int pipefd[2], int flags = 0)
{
    Q_ASSERT((flags & ~O_NONBLOCK) == 0);

#ifdef QT_THREADSAFE_CLOEXEC
    // use pipe2
    flags |= O_CLOEXEC;
    return ::pipe2(pipefd, flags); // pipe2 is documented not to return EINTR
#else
    int ret = ::pipe(pipefd);
    if (ret == -1)
        return -1;

    ::fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
    ::fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);

    // set non-block too?
    if (flags & O_NONBLOCK) {
        ::fcntl(pipefd[0], F_SETFL, ::fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);
        ::fcntl(pipefd[1], F_SETFL, ::fcntl(pipefd[1], F_GETFL) | O_NONBLOCK);
    }

    return 0;
#endif
}

static int readData(int fd, QByteArray &data)
{
    char buf[4096];
    int retryCount = 0;
    int n;
    while (true) {
        n = read(fd, buf, sizeof buf);
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && ++retryCount < 1000)
            usleep(1000);
        else
            break;
    }
    if (retryCount >= 1000)
        qWarning("ClipboardListenerWayland: timeout reading from pipe");
    if (n > 0) {
        data.append(buf, n);
        n = readData(fd, data);
    }
    return n;
}

QString ClipboardListenerWayland::readString(const QMimeType& mimeType, KWayland::Client::DataControlOffer* offer)
{
    int pipefd[2];
    if (qt_safe_pipe(pipefd, O_NONBLOCK) == -1) {
        qWarning() << "unable to open offer";
        return {};
    }

    offer->receive(mimeType, pipefd[1]);
    m_waylandConnection->flush();

    close(pipefd[1]);

    QByteArray content;
    if (readData(pipefd[0], content) != 0) {
        qWarning("ClipboardListenerWayland: error reading data for mimeType %s", qPrintable(mimeType.name()));
        content = {};
    }

    close(pipefd[0]);
    return QString::fromUtf8(content);
}

void ClipboardListenerWayland::selectionReceived(KWayland::Client::DataControlOffer* offer)
{
    const auto mimetypes = offer->offeredMimeTypes();
    for (const auto &mimetype : mimetypes) {
        if (mimetype.inherits(textMimetype)) {
            m_currentContent = readString(mimetype, offer);
            break;
        }
    }
}
