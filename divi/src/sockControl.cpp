
// Copyright (c) 2017-2020 The DIVI Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sockControl.h>

bool CSocketsController::closeSocket(SOCKET& hSocket)
{
    if (hSocket == INVALID_SOCKET)
        return false;
    #ifdef WIN32
    int ret = closesocket(hSocket);
    #else
    int ret = close(hSocket);
    #endif
    hSocket = INVALID_SOCKET;
    return ret != SOCKET_ERROR;
}
