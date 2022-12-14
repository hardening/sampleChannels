/**
 *  Echo channel
 *
 * Copyright 2022 David Fort <contact@hardening-consulting.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>

#include "wts.h"


/** @brief channel configuration */
typedef struct {
    BOOL dynamic;
    ULONG packetsLimit;
    DWORD sleepDelay;
} EchoConfig;

/**
 * @brief adjust the configuration from parameters
 *
 * @param argc number of arguments
 * @param argv arguments array
 * @param config target configuration
 * @return if the function succeeded
 */
static BOOL parseParameters(int argc, char **argv, EchoConfig *config) {
    int i;

    config->dynamic = TRUE;
    config->packetsLimit = ULONG_MAX;
    config->sleepDelay = 100;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-static") == 0) {
            config->dynamic = FALSE;
        }
        else if(strstr(argv[i], "--packets=") == argv[i]) {
            char *str = argv[i] + strlen("--packets=");
            config->packetsLimit = atol(str);
        }
        else if(strstr(argv[i], "--sleepDelay=") == argv[i]) {
            char *str = argv[i] + strlen("--sleepDelay=");
            config->sleepDelay = atol(str);
        }
        else
            return FALSE;
    }

    return TRUE;
}

static void printHelp(const char *argv0) {
    printf("usage: %s [-static] [-doReads] [--packets=<nb>] [--sleepDelay=<milli>]\n", argv0);
    printf("\t-static: opens the echo channel as static channel(default: no)\n");
    printf("\t--packets=<nb>: sends <nb> packets and then close the channel (default: infinite)\n");
    printf("\t--sleepDelay=<milli>: wait <milli> milliseconds between each packet sending (default: 100ms)\n");
}

typedef struct {
    HANDLE hChannel;
    HANDLE fileHandle;
    ULONG sentBytes;
} EchoState;

int main(int argc, char **argv) {
    ULONG i;
    BYTE payload = 0;
    WTSApi wtsapi;
    EchoConfig config;
    EchoState state = { 0 };
    DWORD flags;

    if (!parseParameters(argc, argv, &config)) {
        printHelp(argv[0]);
        return -2;
    }

    if (!loadWtsApi(&wtsapi)) {
        fprintf(stderr, "unable to load wtsapi32.dll\n");
        return -2;
    }

    if (config.dynamic)
        flags = WTS_CHANNEL_OPTION_DYNAMIC;

    state.hChannel = wtsapi.WTSVirtualChannelOpenEx(WTS_CURRENT_SESSION, "ECHO", flags);
    if (!state.hChannel) {
        fprintf(stderr, "unable to open the channel (GetLastError() = %ld)\n", GetLastError());
        return -3;
    }

    for (i = 0; i < config.packetsLimit; i++) {
        ULONG ulBytesWritten;

        BOOL bSuccess = wtsapi.WTSVirtualChannelWrite(state.hChannel, (PCHAR)&payload, 1, &ulBytesWritten);
        if (!bSuccess) {
            fprintf(stderr, "WTSVirtualChannelWrite failed (GetLastError() = %ld)\n",
                    GetLastError());
            break;
        }
        state.sentBytes += ulBytesWritten;
        payload++;

        if (config.sleepDelay)
            Sleep(config.sleepDelay);
    }

    wtsapi.WTSVirtualChannelClose(state.hChannel);
	return 0;
}
