// TAREN COLLYER

#include "NTPClient.h"
#include "azure_c_shared_utility/xlogging.h"

extern NetworkInterface *_defaultSystemNetwork;

// Class that deals with connecting to Azure and acquiring time
class Azure {
    public:
        bool connect()
        {
            LogInfo("Connecting to the network");

            _defaultSystemNetwork = NetworkInterface::get_default_instance();
            if (_defaultSystemNetwork == nullptr) {
                LogError("No network interface found");
                return false;
            }

            int ret = _defaultSystemNetwork->connect();
            if (ret != 0) {
                LogError("Connection error: %d", ret);
                return false;
            }
            LogInfo("Connection success, MAC: %s", _defaultSystemNetwork->get_mac_address());
            return true;
        }

        bool setTime()
        {
            LogInfo("Getting time from the NTP server");

            NTPClient ntp(_defaultSystemNetwork);
            ntp.set_server("time.google.com", 123);
            time_t timestamp = ntp.get_timestamp();
            if (timestamp < 0) {
                LogError("Failed to get the current time, error: %ud", timestamp);
                return false;
            }
            LogInfo("Time: %s", ctime(&timestamp));
            set_time(timestamp);
            return true;
        }
};