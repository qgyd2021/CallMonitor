//
// Created by tianx on 2023/8/10.
//

#ifndef TASK_MUTE_DETECT_H
#define TASK_MUTE_DETECT_H
#include <mutex>
#include <set>

#include <nlohmann/json.hpp>

#include "task.h"
#include "toolbox/utils/wav.h"

using json = nlohmann::json;


class MuteDetectContextProcess: public TaskContextProcess {
public:
  std::map<std::string, std::map<std::string, double >> enabled_languages_;
  std::set<std::string> scene_id_black_list_;

  //Context
  double energy_ = 0.;
  double duration_ = 0.;

  //Process
  double max_energy_threshold_ = 0.0;
  double max_duration_threshold_ = 20.0;

  MuteDetectContextProcess(const std::map<std::string, std::map<std::string, double >> & enabled_languages_,
                           const std::set<std::string> & scene_id_black_list_,
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

  std::map<std::string, std::map<std::string, double >> enabled_languages_;
  std::set<std::string> scene_id_black_list_;

  void load_enabled_languages(json & enabled_languages_json);
  void load_scene_id_black_list(json & scene_id_black_list_json);
  void load_json_config(const std::string & config_json_file);

  MuteDetectManager(const std::string & config_json_file);

  MuteDetectContextProcess * get_context(std::string language, std::string call_id, std::string scene_id);
  void clear_context();
  TaskContextProcess * process(std::string language, std::string call_id, std::string scene_id, const toolbox::WavFile & wav_file);

};


#endif //TASK_MUTE_DETECT_H
