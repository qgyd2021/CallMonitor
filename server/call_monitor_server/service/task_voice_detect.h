//
// Created by tianx on 2023/12/8.
//

#ifndef TASK_VOICE_DETECT_H
#define TASK_VOICE_DETECT_H

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


class VoiceDetectModel {
public:
  torch::jit::script::Module model_;
  std::vector<std::string> label_;

  VoiceDetectModel(const std::string & model_dir);

  c10::Dict<c10::IValue, c10::IValue> forward(std::vector<float> & signal);
};


class VoiceDetectContextProcess: public TaskContextProcess {
public:
  const std::map<std::string, VoiceDetectModel *> language_to_model_map_;
  const std::map<std::string, json> language_to_model_settings_map_;

  //Settings
  float max_duration_ = -1;
  int max_hit_times_ = 1;
  std::string sound_event_ = "voice";
  float threshold_ = 1.0;
  int event_type_ = 4;

  float duration_ = 0.;
  int hit_times_ = 0;

  //Context
  std::vector<float> signal_;
  int signal_size_;

  std::string sound_label_;
  float sound_prob_ = 0.0;

  VoiceDetectContextProcess(
      const std::map<std::string, VoiceDetectModel *> & language_to_model_map,
      const std::map<std::string, json> & language_to_model_settings_map
  );

  bool update(std::string language, std::string call_id,
              std::string scene_id, const toolbox::WavFile & wav_file);
  bool decision();
  void process(std::string language, std::string call_id,
               std::string scene_id, const toolbox::WavFile & wav_file);

};


class VoiceDetectManager: public TaskManagerAbstract {
public:

  //cache
  std::map<std::string, VoiceDetectContextProcess * > context_process_cache_;
  std::int64_t cache_last_update_time_ = 0;
  std::mutex cache_update_lock_;

  std::map<std::string, VoiceDetectModel *> language_to_model_map_;
  std::map<std::string, json> language_to_model_settings_map_;

  void load_language_to_model_map(const std::string & voice_detect_json_file);
  void load_language_to_model_settings_map(const std::string & voice_detect_json_file);
  VoiceDetectManager(const std::string & voice_detect_json_file);

  VoiceDetectContextProcess * get_context(std::string call_id);
  void clear_context();
  TaskContextProcess * process(std::string language, std::string call_id, std::string scene_id, const toolbox::WavFile & wav_file);

};


#endif //TASK_VOICE_DETECT_H
