//
// Created by tianx on 2023/3/6.
//
// 第三方头文件
// 设置宏控制 torch 使用 glog 来打日志; #define C10_USE_GLOG
//

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <httplib.h>

#include "call_monitor_server.h"
#include "control/heart_beat.h"
#include "control/ping_pong.h"
#include "control/update_stream.h"
#include "service/session_stream.h"
#include "settings.h"


int callMonitorServer(int argc, char *argv[]) {
  //初始化日志和启动参数组件
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  FLAGS_log_dir = FLAGS_call_monitor_log_dir;
  FLAGS_stderrthreshold = FLAGS_call_monitor_stderrthreshold;

  //init
  get_session_stream_instance();

  //server
  httplib::Server svr;

  //url http://127.0.0.1:8080/hi
  svr.Post("/HeartBeat", control_heart_beat());
  svr.Post("/asrserver/pingpong", control_ping_pong());
  //svr.Post("/UpdateStream", control_update_stream());
  svr.Post("/asrserver/update_stream", control_update_stream());

  LOG(INFO) << "server " << FLAGS_project_name << " start at " << FLAGS_http_host << ":" << FLAGS_http_port;
  svr.listen(FLAGS_http_host, FLAGS_http_port);
  return 0;
}
