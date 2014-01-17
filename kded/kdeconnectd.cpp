
#include <sys/socket.h>
#include <signal.h>

#include <QCoreApplication>
#include <QSocketNotifier>

#include "daemon.h"

static int sigtermfd[2];
const static char deadbeef = 1;
struct sigaction action;

void sighandler(int signum)
{
    if( signum == SIGTERM || signum == SIGINT)
    {
        ssize_t unused = ::write(sigtermfd[0], &deadbeef, sizeof(deadbeef));
        Q_UNUSED(unused);
    }
}

void initializeTermHandlers(QCoreApplication* app)
{
    ::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermfd);
    QSocketNotifier* snTerm = new QSocketNotifier(sigtermfd[1], QSocketNotifier::Read, app);
    QObject::connect(snTerm, SIGNAL(activated(int)), app, SLOT(quit()));    
    
    action.sa_handler = sighandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    initializeTermHandlers(&app);
    new Daemon(&app);
    return app.exec();
}












