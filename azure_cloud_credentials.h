/*
 * Google Cloud Certificates
 * Copyright (c) 2019-2020, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef AZURE_CLOUD_CREDENTIALS_H
#define AZURE_CLOUD_CREDENTIALS_H

namespace azure_cloud {
    namespace credentials {
        /*
        * Primary Connecion String
        */

        // Use https://dpsgen.z8.web.core.windows.net/ to obtain YOUR connection string
        // This one will not work, but I guess you knew that ;)
        const char iothub_connection_string[] = "HostName=iotc-ac5c0d05-4474-48a2-957c-fe71b84b1590.azure-devices.net;DeviceId=zm5secb5eu;SharedAccessKey=6QMLEuHXHH1+iUHN77QSR5FRXd0nZm1G7LKbBoUToPY=";
    }
}
#endif
