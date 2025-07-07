// ==================== client_config.h ====================
#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <Preferences.h>

class ClientConfig
{
private:
    Preferences preferences;
    const char *namespaceName = "ClientPrefs";

public:
    void begin()
    {
        preferences.begin(namespaceName, false);
    }

    void end()
    {
        preferences.end();
    }

    int getClientId()
    {
        return preferences.getInt("clientId", 0);
    }

    void setClientId(int id)
    {
        id = constrain(id, 0, 15);
        preferences.putInt("clientId", id);
    }
};

#endif // CLIENT_CONFIG_H