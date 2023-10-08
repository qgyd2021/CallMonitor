//
// Created by tianx on 2023/3/7.
//

#include <chrono>
#include <iostream>
#include <sstream>

#include <glog/logging.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include "update_stream.h"
//#include "server/call_monitor_server/service/session_stream.h"
#include "../service/session_stream.h"


using json = nlohmann::json;

//请求体
class UpdateStreamRequest{
public:
  std::string language;
  std::string call_id;
  std::string scene_id;
  std::string signal;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(UpdateStreamRequest, language, call_id, scene_id, signal)
};

//响应体
class Task{
public:
  std::string task_name;
  std::string label;
  std::string status;
  std::string message;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Task, task_name, label, status, message)
};

class UpdateStreamResult{
public:
  std::string language;
  std::string call_id;
  std::string scene_id;
  std::string status;
  std::vector<Task> tasks;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(UpdateStreamResult, language, call_id, scene_id, status, tasks)
};

class UpdateStreamResponse{
public:
  int status_code = 0;
  std::string message;
  UpdateStreamResult result;
  float time_cost = 0;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(UpdateStreamResponse, status_code, message, result, time_cost)
};


std::function<void(const httplib::Request &, httplib::Response &)> control_update_stream(){
  return [](const httplib::Request &request, httplib::Response &response) {
    auto start_millisecond_clock = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    long long int start_time = start_millisecond_clock.count();

    UpdateStreamRequest request_object;
    UpdateStreamResponse response_object;

    //request body
    try
    {
      json requestJson = json::parse(request.body); //将字符串转为json对象
      request_object = requestJson.get<UpdateStreamRequest>(); //将json对象转为定义好的请求体
      LOG(INFO) << "request language: " << request_object.language \
          << ", request call_id: " << request_object.call_id \
          << ", request scene_id: " << request_object.scene_id \
          << ", request signal.length(): " << request_object.signal.length();
    }
    catch (json::exception &e)
    {
      LOG(ERROR) << "request body json::parse failed: " << e.what();
      response_object.status_code = 1;
      response_object.message = e.what();
      json response_json = response_object;
      response.set_content(response_json.dump(), "application/json");
      return;
    };

    //process
    SessionStream * session_stream = get_session_stream_instance();
    std::vector<TaskStatus> task_status_vector = session_stream->update_stream(
        request_object.language,
        request_object.call_id,
        request_object.scene_id,
        request_object.signal
    );

    //call_id status
    std::string status = "finished";
    for (std::vector<TaskStatus>::iterator it = task_status_vector.begin(); it != task_status_vector.end(); ++it) {
      if (it->status_.compare("acknowledged") == 0) {
        status = "acknowledged";
        break;
      }
    }

    //tasks status
    for (std::vector<TaskStatus>::iterator it = task_status_vector.begin();
         it != task_status_vector.end(); ++it) {
      Task task;
      task.task_name = it->task_name_;
      task.message = it->message_;
      task.label = it->label_;
      task.status = it->status_;
      response_object.result.tasks.push_back(std::move(task));
    };

    auto finish_millisecond_clock = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    long long int finish_time = finish_millisecond_clock.count();

    //response body
    response_object.status_code = 0;
    response_object.message = "success";
    response_object.time_cost = (float) (finish_time - start_time) / 1000;
    response_object.result.language = request_object.language;
    response_object.result.call_id = request_object.call_id;
    response_object.result.scene_id = request_object.scene_id;
    response_object.result.status = status;

    json response_json = response_object;
    std::string response_text = response_json.dump();

    LOG(INFO) << "request language: " << request_object.language \
          << ", request call_id: " << request_object.call_id \
          << ", request scene_id: " << request_object.scene_id \
          << ", request signal.length(): " << request_object.signal.length() \
          << ", response body: " << response_text;

    response.set_content(response_text, "application/json");
  };
}
