//
// Created by tianx on 2022/10/19.
//

#include <chrono>
#include <iostream>
#include <stdlib.h>
#include <sstream>

#include <glog/logging.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include "ping_pong.h"

using json = nlohmann::json;

//request schema
class PingPongRequest
{
public:
  int64_t pingMs = 0;
  std::string pingMsg;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(PingPongRequest, pingMs, pingMsg)
};

//response schema
class PingPongResponse
{
public:
  int64_t pingMs = 0;
  std::string pingMsg;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(PingPongResponse, pingMs, pingMsg)
};


std::default_random_engine unsigned_random_integer;
std::uniform_int_distribution<unsigned> zero_one_random(0, 999);


//curl -X POST http://127.0.0.1:4070/asrserver/pingpong -d '{"pingMsg": "tianxing", "pingMs": 20221008}'
std::function<void(const httplib::Request &, httplib::Response &)> control_ping_pong(){
  //[](){} 是标准的 lambda 表达式用法
  return [](const httplib::Request &request, httplib::Response &response) {
    //log
    bool do_log = false;
    if (zero_one_random(unsigned_random_integer) < 10) {
      do_log = true;
    }

    //time
    auto millisecond_clock = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    int64_t millisecond = millisecond_clock.count();

    PingPongRequest request_object;
    PingPongResponse response_object;

    //request body
    try
    {
      json request_json = json::parse(request.body); //将字符串转为json对象
      request_object = request_json.get<PingPongRequest>(); //将json对象转为定义好的请求体
    }
    catch (json::exception &e)
    {
      if (do_log) {
        LOG(INFO) << "request body: " << request.body << " request body json::parse failed: " << e.what();
      }
      response_object.pingMs = millisecond;
      response_object.pingMsg = e.what();
      json reqJson = response_object;
      response.set_content(reqJson.dump(), "application/json");
      return;
    };

    //process

    //response body
    response_object.pingMs = millisecond;
    response_object.pingMsg = request_object.pingMsg;

    json response_json = response_object;
    std::string response_text = response_json.dump();

    if (do_log) {
      LOG(INFO) << "request body: " << request.body << " response body: " << response_text;
    }
    response.set_content(response_text, "application/json");
  };
}
