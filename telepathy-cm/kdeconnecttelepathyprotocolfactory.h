#include "protocol.h"

typedef Tp::SharedPtr<KDEConnectTelepathyProtocol> ConnectProtocolPtr;

/*
 * SingletonFactory for plugins to get access to the Telepathy connection manager
 * Whilst the main process also holds a reference.
 * 
 */
class KDEConnectTelepathyProtocolFactory
{
public:
    static ConnectProtocolPtr interface();
private:
    static Tp::WeakPtr<KDEConnectTelepathyProtocol> s_interface;
};
