
//
// Created by tianx on 2023/4/19.
//
#ifndef TASK_H
#define TASK_H

#include "toolbox/utils/wav.h"

class TaskContextProcess {
public:
  std::string language_;
  std::string call_id_;
  std::string scene_id_;

  std::string label_ = "unknown";
  std::string status_ = "acknowledged";
  std::string message_ = "blank";

  //最近一次更新时间,用于判断缓存是否过期
  long long int last_update_time_;

  TaskContextProcess();

  void update_timestamp();

  virtual void process(std::string language, std::string call_id, std::string scene_id, const toolbox::WavFile & wav_file) = 0;

};


class TaskManagerAbstract {
public:
  virtual TaskContextProcess * process(std::string  language, std::string call_id, std::string scene_id, const toolbox::WavFile & wav_file) = 0;
};


#endif //TASK_H
