//
// Created by Hello Peter on 2021/10/20.
//

#include "../TcpServer.h"

#ifdef IDLE_CONNECTIONS_MANAGER
uint32_t timeInProcess = 0; // counter for checking idle connections
#endif