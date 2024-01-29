//
// Created by tianx on 2023/8/10.
//

#ifndef TASK_MUTE_DETECT_H
#define TASK_MUTE_DETECT_H
#include <mutex>
#include <set>
#include <vector>
#include <utility>

#include <nlohmann/json.hpp>

#include "task.h"
#include "toolbox/utils/wav.h"

using json = nlohmann::json;


class MuteDetectContextProcess: public TaskContextProcess {
public:
  std::map<std::string, std::vector<std::pair<double, double>>> language_to_thresholds_;
  std::set<std::string> scene_id_black_list_;

  //Context
  double energy_ = 0.;
  double duration_ = 0.;

  //Process
  double max_energy_threshold_ = 0.0;
  double max_duration_threshold_ = 0.0;

  void load_language_to_thresholds(json & language_to_thresholds);
  void load_scene_id_black_list(json & scene_id_black_list_json);
  void load_json_config(const std::string & config_json_file);

  MuteDetectContextProcess(
      const std::string & config_json_file,
      std::string language, std::string call_id, std::string scene_id
                           );

  bool update(std::string language, std::string call_id,
              std::string scene_id, const toolbox::WavFile & wav_file);
  bool decision();
  void process(std::string language, std::string call_id,
               std::string scene_id, const toolbox::WavFile & wav_file);

};


class MuteDetectManager: public TaskManagerAbstract {
public:
  //cache
  std::map<std::string, MuteDetectContextProcess * > context_process_cache_;
  std::int64_t cache_last_update_time_ = 0;
  std::mutex cache_update_lock_;

  std::string config_json_file_;

  MuteDetectManager(const std::string & config_json_file);

  MuteDetectContextProcess * get_context(std::string language, std::string call_id, std::string scene_id);
  void clear_context();
  TaskContextProcess * process(std::string language, std::string call_id, std::string scene_id, const toolbox::WavFile & wav_file);

};


#endif //TASK_MUTE_DETECT_H
