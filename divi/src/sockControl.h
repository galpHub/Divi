
// Copyright (c) 2017-2020 The DIVI Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SOCKCONTROL_H
#define _SOCKCONTROL_H
#include <compat.h>
class CSocketsController {
    public:
        CSocketsController(){};
        bool CloseSocket(SOCKET& hSocket);
};

#endif