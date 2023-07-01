//
// Created by tianx on 2022/10/8.
//
#include <chrono>
#include <iostream>
#include <sstream>

#include <glog/logging.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include "heart_beat.h"


using json = nlohmann::json;

//request schema
class HeartBeatRequest
{
public:
  std::string valStr;
  int valInt = 0;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(HeartBeatRequest, valStr, valInt)
};

//response schema
class HeartBeatResponse
{
public:
  int retCode = 0;
  std::string retMsg;
  std::string valStr;
  int valInt = 0;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(HeartBeatResponse, retCode, retMsg, valStr, valInt)
};


//curl -X POST http://127.0.0.1:4070/HeartBeat -d '{"valStr": "tianxing", "valInt": 20221008}'
std::function<void(const httplib::Request &, httplib::Response &)> control_heart_beat(){
  //[](){} 是标准的 lambda 表达式用法
  return [](const httplib::Request &request, httplib::Response &response) {
    HeartBeatRequest request_object;
    HeartBeatResponse response_object;

    //request body
    try
    {
      json request_json = json::parse(request.body);
      request_object = request_json.get<HeartBeatRequest>();
    }
    catch (json::exception &e)
    {
      LOG(ERROR) << "request body json::parse failed: " << e.what();
      response_object.retCode = 1;
      response_object.retMsg = e.what();
      json req_json = response_object;
      response.set_content(req_json.dump(), "application/json");
      return;
    };

    //response body
    response_object.retCode = 0;
    response_object.retMsg = "success";
    response_object.valStr = request_object.valStr;
    response_object.valInt = request_object.valInt;

    json response_json = response_object;
    std::string response_text = response_json.dump();

    LOG(INFO) << "request body: " << request.body << " response body: " << response_text;
    response.set_content(response_text, "application/json");
  };
}
