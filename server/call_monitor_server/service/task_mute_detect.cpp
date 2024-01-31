//
// Created by tianx on 2023/8/10.
//
#include <cstdint>
#include <fstream>
#include <list>
#include <map>
#include <math.h>
#include <set>
#include <string>

#include <glog/logging.h>
#include <nlohmann/json.hpp>

#include "report_asr_event.h"
#include "task.h"
#include "task_mute_detect.h"
#include "toolbox/utils/wav.h"
#include "../settings.h"

using json = nlohmann::json;


MuteDetectContextProcess::MuteDetectContextProcess(
    const std::string & config_json_file,
    std::string language, std::string call_id, std::string scene_id
    ): TaskContextProcess()
    {

  language_ = language;
  call_id_ = call_id;
  scene_id_ = scene_id;

  //language_to_thresholds, max duration threshold.
  LOG(INFO) << "load json config: " << config_json_file;
  this->load_json_config(config_json_file);

  double energy;
  double duration;
  const std::map<std::string, std::vector<std::pair<double, double>>>::iterator item2 = language_to_thresholds_.find(language);
  if (item2 != language_to_thresholds_.end()) {
    for (
        std::vector<std::pair<double, double>>::iterator it = item2->second.begin();
        it != item2->second.end();
        ++it
        ) {
      duration = (*it).first;
      if (duration > max_duration_threshold_) {
        max_duration_threshold_ = duration;
      }
      energy = (*it).second;
      if (duration > max_energy_threshold_) {
        max_energy_threshold_ = energy;
      }
    }
  }
}


void MuteDetectContextProcess::load_language_to_thresholds(json & language_to_thresholds) {
  language_to_thresholds_ = language_to_thresholds;
}


void MuteDetectContextProcess::load_scene_id_black_list(json & scene_id_black_list_json) {

  scene_id_black_list_ = scene_id_black_list_json.get<std::set<std::string>>();

}


void MuteDetectContextProcess::load_json_config(const std::string & config_json_file) {
  json config_json;
  std::ifstream f(config_json_file);
  f >> config_json;

  for (
      json::iterator it = config_json.begin();
      it != config_json.end();
      ++it
      ) {
    if (it.key() == "language_to_thresholds") {
      this->load_language_to_thresholds(it.value());

    } else if (it.key() == "scene_id_black_list") {
      this->load_scene_id_black_list(it.value());

    } else {
      //
    }
  }
}


bool MuteDetectContextProcess::update(std::string language, std::string call_id,
                                      std::string scene_id, const toolbox::WavFile & wav_file) {
  if (status_.compare("finished") != 0) {
    std::set<std::string>::iterator black_item =  scene_id_black_list_.find(scene_id);
    if (black_item != scene_id_black_list_.end()) {
      message_ = "scene_id in black list. ";
      label_ = "unknown";
      status_ = "finished";
      return false;
    }
    std::map<std::string, std::vector<std::pair<double, double>>>::iterator item = language_to_thresholds_.find(scene_id);
    if (item == language_to_thresholds_.end()) {
      item = language_to_thresholds_.find(language);
    }
    if (item == language_to_thresholds_.end()) {
      message_ = "language invalid. ";
      label_ = "unknown";
      status_ = "finished";
      return false;
    }

    double sample = 0.0;
    for (int i = 5; i < wav_file.num_samples() - 5; ++i) {
      // make sample to [-1, 1]
      sample = (* (wav_file.data() + i)) / (1 << 15);
      energy_ += pow(sample, 2);
      //energy_ += abs(sample);

    }
    duration_ += ((float) wav_file.num_samples() / (float) wav_file.sample_rate());

    return true;
  } else {
    return false;
  }
}


bool MuteDetectContextProcess::decision() {
  bool result = false;

  message_ = "duration: " + std::to_string(duration_) + \
      " max_duration_threshold:  " + std::to_string(max_duration_threshold_) + \
      " energy: " + std::to_string(energy_) + \
      " max_energy_threshold:  " + std::to_string(max_energy_threshold_);

  std::map<std::string, std::vector<std::pair<double, double>>>::iterator item = language_to_thresholds_.find(call_id_);
  if (item == language_to_thresholds_.end()) {
    item = language_to_thresholds_.find(language_);
  }

  if (item != language_to_thresholds_.end()) {
    for (
        std::vector<std::pair<double, double>>::iterator it = item->second.begin();
        it != item->second.end();
    ++it
    ) {
      if (duration_ >= (*it).first && energy_ < (*it).second) {
        result = true;
      }
    }
  }

  return result;
}


void MuteDetectContextProcess::process(std::string language, std::string call_id,
                                       std::string scene_id, const toolbox::WavFile & wav_file) {
  bool update_flag = this->update(language, call_id, scene_id, wav_file);
  if ( ! update_flag ) {
    return;
  }

  bool decision_flag = this->decision();

  if (decision_flag) {
    std::string product_id = "callbot";
    //voicemail correspond to eventType 1; mute_detect correspond to eventType 3;
    int event_type = 3;
    std::string text = "mute_detect";

    LOG(INFO) << "report voicemail; duration: " << duration_ \
              << ", language: " << language \
              << ", call_id: " << call_id \
              << ", scene_id: " << scene_id;

    bool report_flag = report_asr_event(
        product_id,
        call_id_,
        language_,
        event_type,
        text,
        FLAGS_asr_event_http_host_port,
        FLAGS_asr_event_uri,
        FLAGS_secret_key
    );
    label_ = "mute_detect";
    status_ = "finished";
  } else if (duration_ >= max_duration_threshold_) {
    label_ = "unknown";
    status_ = "finished";
  } else if (energy_ >= max_energy_threshold_) {
    label_ = "unknown";
    status_ = "finished";
  } else {
    //
  }
}


MuteDetectManager::MuteDetectManager(const std::string & config_json_file) {
  config_json_file_ = config_json_file;
}


MuteDetectContextProcess * MuteDetectManager::get_context(std::string language, std::string call_id, std::string scene_id) {
  cache_update_lock_.lock();

  MuteDetectContextProcess * context;

  std::map<std::string, MuteDetectContextProcess * >::iterator item = context_process_cache_.find(call_id);
  if (item == context_process_cache_.end()) {
    context = new MuteDetectContextProcess(
        config_json_file_,
        language, call_id, scene_id
        );
    context_process_cache_[call_id] = context;
  } else {
    context = item->second;
    context->update_timestamp();
  }
  cache_update_lock_.unlock();
  return context;
}


void MuteDetectManager::clear_context() {
  auto second_clock = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());
  std::int64_t timestamp_now = second_clock.count();

  LOG(INFO) << "task mute detect, this->cache.size(): " << context_process_cache_.size();

  if (timestamp_now - cache_last_update_time_ > 6) {
    cache_last_update_time_ = timestamp_now;
    cache_update_lock_.lock();

    // search the context that timeout
    std::list<std::string> timeout_contexts;
    for (std::map<std::string, MuteDetectContextProcess *>::iterator it = context_process_cache_.begin();
         it != context_process_cache_.end(); ++it) {
      std::int64_t duration = timestamp_now - it->second->last_update_time_;

      //超过6秒没有更新过的上下文,清除.
      if (duration > 6) {
        timeout_contexts.push_back(it->first);
      }
    }

    //erase the timeout context
    for (std::list<std::string>::iterator it=timeout_contexts.begin(); it != timeout_contexts.end(); ++it) {
      std::map<std::string, MuteDetectContextProcess *>::iterator item = context_process_cache_.find(*it);
      if (item != context_process_cache_.end()) {
        delete item->second;
        context_process_cache_.erase(*it);
      }
    }
    cache_update_lock_.unlock();
  }
}


TaskContextProcess * MuteDetectManager::process(std::string language, std::string call_id,
                                                std::string scene_id, const toolbox::WavFile & wav_file) {
  MuteDetectContextProcess * context_ptr = this->get_context(language, call_id, scene_id);
  context_ptr->process(language, call_id, scene_id, wav_file);
  this->clear_context();
  return context_ptr;
}
