//
// Created by tianx on 2023/12/8.
//
#include <chrono>
#include <cstdint>
#include <fstream>
#include <list>
#include <map>
#include <set>
#include <string>

#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include <torch/script.h>
#include <torch/torch.h>

#include "report_asr_event.h"
#include "task.h"
#include "task_voicemail.h"
#include "toolbox/utils/wav.h"
#include "../settings.h"


using json = nlohmann::json;


VoicemailModel::VoicemailModel(const std::string & model_dir) {
  std::string model_file = model_dir + "/pth/cnn_voicemail.pth";
  std::string label_file = model_dir + "/pth/labels.json";

  //model
  try {
    LOG(INFO) << "load model file: " << model_file;
    model_ = torch::jit::load(model_file);
    //model_ = std::move(model);
    model_.eval();
  } catch (const c10::Error &e) {
    LOG(ERROR) << "error loading the model file: " << e.what();
  }

  //labels
  try {
    LOG(INFO) << "load label file: " << label_file;
    json label;
    std::ifstream f(label_file);
    f >> label;
    label_ = label.get<std::vector<std::string>>();
  } catch (const c10::Error &e) {
    LOG(ERROR) << "error loading the label file: " << e.what();
  }
}


c10::Dict<c10::IValue, c10::IValue> VoicemailModel::forward(
    std::vector<float> & signal) {

  torch::IValue outputs_i_value;

  try {
    torch::Tensor inputs1 = torch::from_blob(signal.data(), {(int32_t) signal.size()}, torch::kFloat);
    torch::Tensor inputs = torch::unsqueeze(inputs1, 0);
    outputs_i_value = this->model_({inputs});
  } catch (const c10::Error &e) {
    LOG(ERROR) << "VoicemailModel::forward error: " << e.what();
    throw e;
  }
  c10::Dict<c10::IValue, c10::IValue> outputs = outputs_i_value.toGenericDict();
  return std::move(outputs);
}


VoicemailContextProcess::VoicemailContextProcess(
    const std::map<std::string, VoicemailModel *> & language_to_model_map,
    const std::map<std::string, json> & language_to_model_settings_map):
    TaskContextProcess(),
    language_to_model_map_(language_to_model_map),
    language_to_model_settings_map_(language_to_model_settings_map) {
  //
}


bool VoicemailContextProcess::update(std::string language, std::string call_id,
                                        std::string scene_id, const toolbox::WavFile & wav_file) {
  if (status_.compare("finished") != 0) {
    //settings
    auto item1 = language_to_model_settings_map_.find(language);
    if (item1 == language_to_model_settings_map_.end()) {
      label_ = "language invalid";
      status_ = "finished";
      return false;
    }

    json js = item1->second;
    max_duration_ = js["max_duration"];
    max_hit_times_ = js["max_hit_times"];
    black_list_ = js["black_list"].get<std::set<std::string>>();

    //
    language_ = language;
    call_id_ = call_id;
    scene_id_ = scene_id;

    signal_.clear();
    float sample = 0.0;
    for (int i = 0; i < wav_file.num_samples(); ++i) {
      // make sample to [-1, 1]
      sample = (*(wav_file.data() + i)) / (1 << 15);
      signal_.push_back(sample);
    }
    signal_size_ = wav_file.num_samples();

    duration_ += (float) wav_file.num_samples() / (float) wav_file.sample_rate();

    //black list
    std::set<std::string>::iterator it = black_list_.find(scene_id);
    if (it != black_list_.end()) {
      label_ = "hit black list";
      status_ = "finished";
      return false;
    }

    //model
    auto item2 = language_to_model_map_.find(language);
    if (item2 == language_to_model_map_.end()) {
      label_ = "language invalid";
      status_ = "finished";
      return false;
    } else {
      VoicemailModel *model_ptr = item2->second;
      c10::Dict <c10::IValue, c10::IValue> outputs = model_ptr->forward(signal_);
      torch::Tensor probs_tensor = outputs.at(c10::IValue("probs")).toTensor();
      torch::Tensor indexes_tensor = torch::argmax(probs_tensor, -1);
      std::vector <std::int64_t> indexes_vector(
          indexes_tensor.data_ptr<std::int64_t>(),
          indexes_tensor.data_ptr<std::int64_t>() + indexes_tensor.numel()
      );
      torch::Tensor voicemail_probs_tensor = probs_tensor.index(
          {
              torch::indexing::Slice(torch::indexing::None),
              torch::indexing::Slice(1)
          }
      );
      std::vector<float> voicemail_probs_vector(
          voicemail_probs_tensor.data_ptr<float>(),
          voicemail_probs_tensor.data_ptr<float>() + voicemail_probs_tensor.numel()
      );
      std::string label = model_ptr->label_[indexes_vector[0]];
      float voicemail_prob_float = voicemail_probs_vector[0];
      score_queue_.push(voicemail_prob_float);
      return true;
    }
  } else {
    return false;
  }
}


bool VoicemailContextProcess::decision() {
  bool result = false;
  while (true) {
    if (score_queue_.empty()) {
      break;
    }
    this_score_ = score_queue_.front();
    score_queue_.pop();
    if (this_score_ > threshold1_) {
      message_ = "this_score: " + std::to_string(this_score_) +
                 ", threshold1_: " + std::to_string(threshold1_);
      result = true;
      break;
    } else if ((last_score_ + this_score_) / 2 > threshold2_) {
      message_ = "last_score: " + std::to_string(last_score_) +
                 ", this_score: " + std::to_string(this_score_) +
                 ", threshold2_: " + std::to_string(threshold2_);
      result = true;
      break;
    }
    last_score_ = this_score_;
  }
  return result;
}


void VoicemailContextProcess::process(std::string language, std::string call_id,
                                         std::string scene_id, const toolbox::WavFile & wav_file) {
  bool update_flag = this->update(language, call_id, scene_id, wav_file);
  if ( ! update_flag ) {
    return;
  }

  bool decision_flag = this->decision();

  if (decision_flag) {
    std::string product_id = "callbot";
    //voicemail correspond to eventType 1
    int event_type = 1;
    std::string text = "voicemail";

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

    label_ = "voicemail";

    hit_times_ += 1;
    if (max_hit_times_ > 0 && hit_times_ >= max_hit_times_) {
      status_ = "finished";
    }
  } else if (duration_ >= max_duration_) {
    label_ = "unknown";
    status_ = "finished";
  } else {
    //
  }
}


void VoicemailManager::load_language_to_model_map(
    const std::string & voicemail_json_file) {
  json language_to_model_json;
  std::ifstream f(voicemail_json_file);
  f >> language_to_model_json;

  std::string language;
  std::string model_dir;
  for (json::iterator it = language_to_model_json.begin();
       it != language_to_model_json.end(); ++it) {

    language = it.key();
    model_dir = it.value()["model_dir"];

    VoicemailModel * voicemail_model = new VoicemailModel(model_dir);
    language_to_model_map_[language] = voicemail_model;

  }
}


void VoicemailManager::load_language_to_model_settings_map(
    const std::string & voicemail_json_file) {
  json language_to_model_json;
  std::ifstream f(voicemail_json_file);
  f >> language_to_model_json;

  std::string language;
  json js;
  for (json::iterator it = language_to_model_json.begin();
       it != language_to_model_json.end(); ++it) {

    language = it.key();
    js = it.value();

    language_to_model_settings_map_[language] = js;

  }
}


VoicemailManager::VoicemailManager(const std::string & voicemail_json_file) {
  //load models
  LOG(INFO) << "load language to model map: " << voicemail_json_file;
  this->load_language_to_model_map(voicemail_json_file);
  LOG(INFO) << "load language to model settings map: " << voicemail_json_file;
  this->load_language_to_model_settings_map(voicemail_json_file);
}


VoicemailContextProcess * VoicemailManager::get_context(std::string call_id) {
  cache_update_lock_.lock();

  VoicemailContextProcess * context;

  std::map<std::string, VoicemailContextProcess * >::iterator item = context_process_cache_.find(call_id);
  if (item == context_process_cache_.end()) {
    context = new VoicemailContextProcess(
        language_to_model_map_,
        language_to_model_settings_map_
    );
    context_process_cache_[call_id] = context;
  } else {
    context = item->second;
    context->update_timestamp();
  }
  cache_update_lock_.unlock();
  return context;
}


void VoicemailManager::clear_context() {
  auto second_clock = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());
  std::int64_t timestamp_now = second_clock.count();

  LOG(INFO) << "task cnn voicemail, this->cache.size(): " << context_process_cache_.size();

  if (timestamp_now - cache_last_update_time_ > 6) {
    cache_last_update_time_ = timestamp_now;
    cache_update_lock_.lock();

    // search the context that timeout
    std::list<std::string> timeout_contexts;
    for (std::map<std::string, VoicemailContextProcess *>::iterator it = context_process_cache_.begin();
         it != context_process_cache_.end(); ++it) {
      std::int64_t duration = timestamp_now - it->second->last_update_time_;

      //超过6秒没有更新过的上下文,清除.
      if (duration > 6) {
        timeout_contexts.push_back(it->first);
      }
    }

    //erase the timeout context
    for (std::list<std::string>::iterator it=timeout_contexts.begin(); it != timeout_contexts.end(); ++it) {
      std::map<std::string, VoicemailContextProcess *>::iterator item = context_process_cache_.find(*it);
      if (item != context_process_cache_.end()) {
        delete item->second;
        context_process_cache_.erase(*it);
      }
    }
    cache_update_lock_.unlock();
  }
}


TaskContextProcess * VoicemailManager::process(std::string language, std::string call_id,
                                               std::string scene_id, const toolbox::WavFile & wav_file) {
  VoicemailContextProcess * context_ptr = this->get_context(call_id);
  context_ptr->process(language, call_id, scene_id, wav_file);
  this->clear_context();
  return context_ptr;
}
