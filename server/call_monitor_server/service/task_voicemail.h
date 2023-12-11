//
// Created by tianx on 2023/12/8.
//

#ifndef TASK_VOICEMAIL_H
#define TASK_VOICEMAIL_H

#include <map>
#include <mutex>
#include <set>
#include <string>

#include <nlohmann/json.hpp>
#include <torch/script.h>
#include <torch/torch.h>

#include "task.h"
#include "toolbox/utils/wav.h"


using json = nlohmann::json;


class VoicemailModel {
public:
  torch::jit::script::Module model_;
  std::vector<std::string> label_;

  VoicemailModel(const std::string & model_dir);

  c10::Dict<c10::IValue, c10::IValue> forward(std::vector<float> & signal);
};


class VoicemailContextProcess: public TaskContextProcess {
public:
  const std::map<std::string, VoicemailModel *> language_to_model_map_;
  const std::map<std::string, json> language_to_model_settings_map_;

  //Settings
  float max_duration_ = -1;
  int max_hit_times_ = 1;
  std::set<std::string> black_list_;

  float duration_ = 0.;
  int hit_times_ = 0;

  //Context
  std::vector<float> signal_;
  int signal_size_;

  float last_score_ = 0.0;
  float this_score_ = 0.0;
  std::queue<float> score_queue_;

  //Process
  //1次分数大于 0.85 或连续 2 次的平均分大于 0.65, 就判断为语音信箱
  double threshold1_ = 0.75;
  double threshold2_ = 0.5;
  int max_count_ = 4;

  VoicemailContextProcess(
      const std::map<std::string, VoicemailModel *> & language_to_model_map,
      const std::map<std::string, json> & language_to_model_settings_map
  );

  bool update(std::string language, std::string call_id,
              std::string scene_id, const toolbox::WavFile & wav_file);
  bool decision();
  void process(std::string language, std::string call_id,
               std::string scene_id, const toolbox::WavFile & wav_file);

};


class VoicemailManager: public TaskManagerAbstract {
public:

  //cache
  std::map<std::string, VoicemailContextProcess * > context_process_cache_;
  std::int64_t cache_last_update_time_ = 0;
  std::mutex cache_update_lock_;

  std::map<std::string, VoicemailModel *> language_to_model_map_;
  std::map<std::string, json> language_to_model_settings_map_;

  std::map<std::string, std::set<std::string>> language_to_scene_ids_gray_box_testing_map_;

  void load_language_to_model_map(const std::string & voicemail_json_file);
  void load_language_to_model_settings_map(const std::string & voicemail_json_file);
  VoicemailManager(const std::string & voicemail_json_file);

  VoicemailContextProcess * get_context(std::string call_id);
  void clear_context();
  TaskContextProcess * process(std::string language, std::string call_id, std::string scene_id, const toolbox::WavFile & wav_file);

};


#endif //TASK_VOICEMAIL_H
