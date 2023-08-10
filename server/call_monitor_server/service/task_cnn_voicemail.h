//
// Created by tianx on 2023/4/23.
//

#ifndef TASK_CNN_VOICEMAIL_H
#define TASK_CNN_VOICEMAIL_H

#include <map>
#include <mutex>
#include <set>
#include <string>

#include <torch/script.h>
#include <torch/torch.h>

#include "task.h"
#include "toolbox/utils/wav.h"


class CnnVoicemailModel {
public:
  torch::jit::script::Module model_;
  std::vector<std::string> label_;

  CnnVoicemailModel(const std::string & model_dir);

  c10::Dict<c10::IValue, c10::IValue> forward(std::vector<float> & signal);
};


class CnnVoicemailContextProcess: public TaskContextProcess {
public:
  const std::map<std::string, CnnVoicemailModel *> language_to_model_map_;
  const std::map<std::string, std::set<std::string>> language_to_scene_ids_gray_box_testing_map_;
  std::vector<float> voicemail_prob_vector_;

  //Context
  std::vector<float> signal_;
  int signal_size_;
  float duration_ = 0.;

  float last_score_ = 0.0;
  float this_score_ = 0.0;
  std::queue<float> score_queue_;

  //Process
  //1次分数大于 0.85 或连续 2 次的平均分大于 0.65, 就判断为语音信箱
  double threshold1_ = 0.75;
  double threshold2_ = 0.5;
  int max_count_ = 4;

  CnnVoicemailContextProcess(
    const std::map<std::string, CnnVoicemailModel *> & language_to_model_map,
    const std::map<std::string, std::set<std::string>> & language_to_scene_ids_gray_box_testing_map
  );

  bool update(std::string language, std::string call_id,
              std::string scene_id, const toolbox::WavFile & wav_file);
  bool decision();
  void process(std::string language, std::string call_id,
               std::string scene_id, const toolbox::WavFile & wav_file);

};


class CnnVoicemailManager: public TaskManagerAbstract {
public:

  //cache
  std::map<std::string, CnnVoicemailContextProcess * > context_process_cache_;
  std::int64_t cache_last_update_time_ = 0;
  std::mutex cache_update_lock_;

  std::map<std::string, CnnVoicemailModel *> language_to_model_map_;
  std::map<std::string, std::set<std::string>> language_to_scene_ids_gray_box_testing_map_;

  void load_language_to_model_map(const std::string & language_to_model_json_file);
  void load_language_to_scene_ids_gray_box_testing_map(const std::string & cnn_voicemail_gray_box_testing_json_file);
  CnnVoicemailManager(const std::string & language_to_model_json_file);

  CnnVoicemailContextProcess * get_context(std::string call_id);
  void clear_context();
  TaskContextProcess * process(std::string language, std::string call_id, std::string scene_id, const toolbox::WavFile & wav_file);

};


#endif //TASK_CNN_VOICEMAIL_H
