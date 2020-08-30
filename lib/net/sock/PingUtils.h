// /**************************************************************************
//  *
//  * Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
//  * All rights reserved.
//  *
//  * Distributed under the terms of the GNU General Public License v2.
//  *
//  * This software is provided 'as is' with no explicit or implied warranties
//  * in respect of its properties, including, but not limited to, correctness
//  * and/or fitness for purpose.
//  *
//  **************************************************************************/

#include <string>

namespace MSF {

int Ping(const char *host, int ping_timeout);
std::string PingResult(int result);

}  // namespace MSF