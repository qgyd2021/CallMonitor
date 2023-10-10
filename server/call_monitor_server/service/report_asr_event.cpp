//
// Created by tianx on 2023/4/20.
//
#include <string>

#include <glog/logging.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include "report_asr_event.h"
#include "toolbox/hashlib/md5.h"
#include "../settings.h"


//请求体
class AsrEventRequest
{
public:
  std::string productID;
  std::string callID;
  int eventType;
  std::string language;
  int64_t eventTime;
  std::string text;
  int64_t createTs;
  std::string sign;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(AsrEventRequest,
      productID, callID, eventType, language, eventTime, text, createTs, sign)
};


bool report_asr_event(
    std::string & product_id, std::string & call_id,
    std::string & language, int event_type, std::string & text,
    std::string & asr_event_http_host_port,
    std::string & asr_event_uri,
    std::string & secret_key
)
{
  //https://cplusplus.com/reference/chrono/
  auto millisecond_clock = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
  int64_t millisecond = millisecond_clock.count();

  //nx_auth
  std::string auth_raw = "callID=" + call_id +
                         "&createTs=" + std::to_string(millisecond) +
                         "&eventTime=" + std::to_string(millisecond) +
                         "&eventType=" + std::to_string(event_type) +
                         "&language=" + language +
                         "&productID=" + product_id +
                         "&text=" + text;
  //std::cout << auth_raw << std::endl;

  std::string raw_sign = secret_key + "&" + auth_raw;
  //std::cout << raw_sign << std::endl;
  MD5 md5(raw_sign);
  std::string sign = md5.toStr();
  //std::cout << sign << std::endl;

  //request body
  AsrEventRequest request;

  request.productID = product_id;
  request.callID = call_id;
  request.eventType = event_type;
  request.language = language;
  request.eventTime = millisecond;
  request.text = text;
  request.createTs = millisecond;
  request.sign = sign;

  httplib::Headers headers = {
      { "Content-Type", "application/json" }
  };

  nlohmann::json request_json = request;
  auto request_body = request_json.dump();

  httplib::Client http_requests(asr_event_http_host_port);
  http_requests.set_connection_timeout(0, 300000); // 300 milliseconds
  http_requests.set_read_timeout(2, 0); // seconds
  http_requests.set_write_timeout(2, 0); // seconds

  // http://10.52.66.97:8002/svrapi/callbot/voip/asrevent
  auto response_data = http_requests.Post(asr_event_uri, request_body, "application/json");
  //std::cout << response_data << std::endl;

  LOG(INFO) << "report voicemail"
            << " asr_event_http_host_port: (" << asr_event_http_host_port << ");" \
            << " asr_event_uri: (" << asr_event_uri << ");" \
            << " response_data: " << response_data << ", (tips: 1 when success);" \
            << " status: (" << std::to_string(response_data->status) << ");" \
            << " body: (" << response_data->body << ");";

  bool result = false;
  //失败时 responseData 为 0
  if (response_data) {
    //状态码
    int status = response_data->status;
    //resp.text
    std::string body = response_data->body;

    //std::cout << status << std::endl;
    //std::cout << body << std::endl;


    if (status == 200) {
      result = true;
    }
  }
  return result;
}

