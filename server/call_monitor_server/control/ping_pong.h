//
// Created by tianx on 2022/10/19.
//
#ifndef CONTROL_PING_PONG_H
#define CONTROL_PING_PONG_H

#include <httplib.h>


//curl -X POST http://127.0.0.1:4070/asrserver/pingpong -d '{"pingMsg": "tianxing", "pingMs": 20221008}'
std::function<void(const httplib::Request &, httplib::Response &)> control_ping_pong();

#endif //CONTROL_PING_PONG_H
