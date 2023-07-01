//
// Created by tianx on 2023/4/19.
//
#include <chrono>
#include <cstdint>
#include <iostream>

#include <glog/logging.h>

#include "task.h"


TaskContextProcess::TaskContextProcess() {
  update_timestamp();
}

void TaskContextProcess::update_timestamp()
{
  auto milliSecondClock = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()
      );
  std::size_t timestampNow = milliSecondClock.count();
  this->last_update_time_ = timestampNow;
}
