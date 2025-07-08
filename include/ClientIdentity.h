// client_identity.h
#ifndef CLIENT_IDENTITY_H
#define CLIENT_IDENTITY_H

#include "client_config.h"

class ClientIdentity
{
private:
    ClientConfig *config;
    int clientId;

public:
    ClientIdentity(ClientConfig *cfg) : config(cfg), clientId(0) {}

    void begin()
    {
        config->begin();
        clientId = config->getClientId();
    }

    int get() const
    {
        return clientId;
    }

    void set(int id)
    {
        id = constrain(id, 0, 15);
        config->setClientId(id);
        clientId = id;
    }

    void refresh()
    {
        clientId = config->getClientId();
    }
};

#endif
