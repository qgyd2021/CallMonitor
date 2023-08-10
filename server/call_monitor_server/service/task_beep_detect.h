//
// Created by tianx on 2022/12/9.
//
#ifndef TASK_BEEP_DETECT_H
#define TASK_BEEP_DETECT_H

#include <cstdint>
#include <iostream>
#include <map>
#include <queue>
#include <vector>
#include <mutex>

#include <torch/script.h>
#include <torch/torch.h>

#include "task.h"


class BeepDetectContextProcess: public TaskContextProcess {
public:
  //Context
  int sample_rate_;
  //float * signal_;    //在 beep detect 中, signal_ 直接使用 wav 的指针, 只在当前会话中有效.
  std::vector<float> signal_;
  int signal_size_;
  float duration_ = 0.;

  std::queue<float> score_queue_;

  //Process
  float win_len_;
  float win_step_;
  std::int64_t n_fft_;

  int max_length_ = 25;
  int freq_begin_ = 36;
  int freq_end_ = 54;
  int freq_win_size_ = 8;
  float score_threshold_ = 0.8;

  BeepDetectContextProcess(float win_len, float win_step, std::int64_t n_fft);

  bool update(std::string language, std::string call_id, std::string scene_id, const toolbox::WavFile & wav_file);
  bool decision();
  void process(std::string language, std::string call_id, std::string scene_id, const toolbox::WavFile & wav_file);

  torch::Tensor calc_mag_spec(const torch::Tensor & signal, std::int64_t n_fft,
                              std::int64_t hop_length, std::int64_t win_length);

};


class BeepDetectManager: public TaskManagerAbstract {
public:

  //cache
  std::map<std::string, BeepDetectContextProcess * > context_process_cache_;
  std::int64_t cache_last_update_time_ = 0;
  std::mutex cache_update_lock_;

  //构造函数
  BeepDetectManager() = default;

  BeepDetectContextProcess * get_context(std::string call_id);
  void clear_context();
  TaskContextProcess * process(std::string language, std::string call_id, std::string scene_id, const toolbox::WavFile & wav_file);
};


#endif //TASK_BEEP_DETECT_H
