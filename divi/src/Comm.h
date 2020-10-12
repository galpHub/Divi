
// Copyright (c) 2017-2020 The DIVI Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _COMM_H
#define _COMM_H

#include <compat.h>
#include <protocol.h>

 class CComm 
 {
     private:
        SOCKET socket; 
        CAddress address;
     public:
        CComm(SOCKET hSocketIn, CAddress addrIn);
        SOCKET getSocket(); 

 };

#endif