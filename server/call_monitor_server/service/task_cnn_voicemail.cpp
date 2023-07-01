//
// Created by tianx on 2023/4/23.
//
#include <chrono>
#include <cstdint>
#include <map>
#include <set>
#include <string>

#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include <torch/script.h>
#include <torch/torch.h>

#include "report_asr_event.h"
#include "task.h"
#include "task_cnn_voicemail.h"
#include "toolbox/utils/wav.h"
#include "../settings.h"


using json = nlohmann::json;


CnnVoicemailModel::CnnVoicemailModel(const std::string & model_dir) {
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


c10::Dict<c10::IValue, c10::IValue> CnnVoicemailModel::forward(
    std::vector<float> & signal) {

  torch::IValue outputs_i_value;

  try {
    torch::Tensor inputs1 = torch::from_blob(signal.data(), {(int32_t) signal.size()}, torch::kFloat);
    torch::Tensor inputs = torch::unsqueeze(inputs1, 0);
    outputs_i_value = this->model_({inputs});
  } catch (const c10::Error &e) {
    LOG(ERROR) << "CnnVoicemailModel::forward error: " << e.what();
    throw e;
  }
  c10::Dict<c10::IValue, c10::IValue> outputs = outputs_i_value.toGenericDict();
  return std::move(outputs);
}


CnnVoicemailContextProcess::CnnVoicemailContextProcess(
    const std::map<std::string, CnnVoicemailModel *> & language_to_model_map,
    const std::map<std::string, std::set<std::string>> & language_to_scene_ids_gray_box_testing_map):
    TaskContextProcess(),
    language_to_model_map_(language_to_model_map),
    language_to_scene_ids_gray_box_testing_map_(language_to_scene_ids_gray_box_testing_map) {
  //
}


bool CnnVoicemailContextProcess::update(std::string language, std::string call_id,
                                        std::string scene_id, const toolbox::WavFile & wav_file) {
  if (status_.compare("finished") != 0) {

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

    //language to scene_ids gray box testing
    //std::map<std::string, std::set<std::string>>::iterator item1 = language_to_scene_ids_gray_box_testing_map_.find(language);
    auto item1 = language_to_scene_ids_gray_box_testing_map_.find(language);
    if (item1 != language_to_scene_ids_gray_box_testing_map_.end()) {
      std::set <std::string> scene_ids = item1->second;
      std::set<std::string>::iterator it = scene_ids.find(scene_id);
      if (it == scene_ids.end()) {
        label_ = "scene id not allowed in test language";
        status_ = "finished";
        return false;
      }
    }

    //model
    //std::map<std::string, CnnVoicemailModel * >::iterator item2 = language_to_model_map_.find(language);
    auto item2 = language_to_model_map_.find(language);
    if (item2 == language_to_model_map_.end()) {
      label_ = "language invalid";
      status_ = "finished";
      return false;
    } else {
      CnnVoicemailModel *model_ptr = item2->second;
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


bool CnnVoicemailContextProcess::decision() {
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


void CnnVoicemailContextProcess::process(std::string language, std::string call_id,
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
    status_ = "finished";
  } else if (duration_ >= 10.) {
    label_ = "unknown";
    status_ = "finished";
  } else {
    //
  }
}


void CnnVoicemailManager::load_language_to_model_map(
    const std::string &language_to_model_json_file) {
  json language_to_model_json;
  std::ifstream f(language_to_model_json_file);
  f >> language_to_model_json;

  std::string language;
  std::string model_dir;
  std::string model_path;
  std::string label_path;
  for (json::iterator it = language_to_model_json.begin();
       it != language_to_model_json.end(); ++it) {

    //language = static_cast<std::string>(it.keys());
    //model_dir = static_cast<std::string>(it.value());

    language = it.key();
    model_dir = it.value();

    CnnVoicemailModel * cnn_voicemail_model = new CnnVoicemailModel(model_dir);
    language_to_model_map_[language] = cnn_voicemail_model;

  }
}


void CnnVoicemailManager::load_language_to_scene_ids_gray_box_testing_map(
    const std::string &cnn_voicemail_gray_box_testing_file) {
  json language_to_scene_ids_json;
  std::ifstream f(cnn_voicemail_gray_box_testing_file);
  f >> language_to_scene_ids_json;

  for (
      json::iterator it = language_to_scene_ids_json.begin();
      it != language_to_scene_ids_json.end();
      ++it
      ) {
    std::set<std::string> value = it.value();
    language_to_scene_ids_gray_box_testing_map_.insert(std::make_pair(it.key(), value));
  }
}


CnnVoicemailManager::CnnVoicemailManager(const std::string & language_to_model_json_file) {
  //load models
  LOG(INFO) << "load language to model map: " << language_to_model_json_file;
  this->load_language_to_model_map(language_to_model_json_file);
}


CnnVoicemailContextProcess * CnnVoicemailManager::get_context(std::string call_id) {
  cache_update_lock_.lock();

  CnnVoicemailContextProcess * context;

  std::map<std::string, CnnVoicemailContextProcess * >::iterator item = context_process_cache_.find(call_id);
  if (item == context_process_cache_.end()) {
    context = new CnnVoicemailContextProcess(
      language_to_model_map_,
      language_to_scene_ids_gray_box_testing_map_
    );
    context_process_cache_[call_id] = context;
  } else {
    context = item->second;
    context->update_timestamp();
  }
  cache_update_lock_.unlock();
  return context;
}


void CnnVoicemailManager::clear_context() {
  auto second_clock = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());
  std::int64_t timestamp_now = second_clock.count();

  LOG(INFO) << "task cnn voicemail, this->cache.size(): " << context_process_cache_.size();

  if (timestamp_now - cache_last_update_time_ > 6) {
    cache_last_update_time_ = timestamp_now;
    cache_update_lock_.lock();

    // search the context that timeout
    std::list<std::string> timeout_contexts;
    for (std::map<std::string, CnnVoicemailContextProcess *>::iterator it = context_process_cache_.begin();
         it != context_process_cache_.end(); ++it) {
      std::int64_t duration = timestamp_now - it->second->last_update_time_;

      //超过6秒没有更新过的上下文,清除.
      if (duration > 6) {
        timeout_contexts.push_back(it->first);
      }
    }

    //erase the timeout context
    for (std::list<std::string>::iterator it=timeout_contexts.begin(); it != timeout_contexts.end(); ++it) {
      std::map<std::string, CnnVoicemailContextProcess *>::iterator item = context_process_cache_.find(*it);
      if (item != context_process_cache_.end()) {
        delete item->second;
        context_process_cache_.erase(*it);
      }
    }
    cache_update_lock_.unlock();
  }
}


TaskContextProcess * CnnVoicemailManager::process(std::string language, std::string call_id,
                             std::string scene_id, const toolbox::WavFile & wav_file) {
  CnnVoicemailContextProcess * context_ptr = this->get_context(call_id);
  context_ptr->process(language, call_id, scene_id, wav_file);
  this->clear_context();
  return context_ptr;
}
