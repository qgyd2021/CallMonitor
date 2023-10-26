//
// Created by tianx on 2023/3/7.
//
#include <cstdint>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <vector>

#include <nlohmann/json.hpp>

#include "session_stream.h"
#include "toolbox/utils/base64.h"
#include "toolbox/utils/wav.h"
#include "../settings.h"

#include "task.h"
#include "task_beep_detect.h"
#include "task_cnn_voicemail.h"
#include "task_mute_detect.h"


using json = nlohmann::json;


TaskStatus::TaskStatus(std::string task_name, std::string message,
                       std::string label, std::string status) :
                       task_name_(task_name), message_(message),
                       label_(label), status_(status) {}


void SessionStream::load_scene_id_to_language_map(const std::string & filename){
  json scene_id_to_language_json;
  std::ifstream f(filename.c_str());
  f >> scene_id_to_language_json;

  for (
      json::iterator it = scene_id_to_language_json.begin();
      it != scene_id_to_language_json.end();
      ++it
      ) {
    scene_id_to_language_map_.insert(std::make_pair(it.key(), it.value()));
  }
}


void SessionStream::load_languages_to_skip_wav_set(const std::string & filename){
  json languages_to_skip_save_wav_json;
  std::ifstream f(filename.c_str());
  f >> languages_to_skip_save_wav_json;

  for (size_t i = 0; i < languages_to_skip_save_wav_json.size(); ++i) {
    this->languages_to_skip_save_wav_set_.insert(languages_to_skip_save_wav_json[i].get<std::string>());
  }
}


void SessionStream::init(){
  LOG(INFO) << "load scene id to language: " << FLAGS_scene_id_to_language_file;
  this->load_scene_id_to_language_map(FLAGS_scene_id_to_language_file);
  LOG(INFO) << "load languages to save wav: " << FLAGS_languages_to_skip_save_wav_file;
  this->load_languages_to_skip_wav_set(FLAGS_languages_to_skip_save_wav_file);
  LOG(INFO) << "new BeepDetectManager: ";
  beep_detect_manager_ = new BeepDetectManager();
  LOG(INFO) << "new CnnVoicemailManager: " << FLAGS_cnn_voicemail_language_to_model_file;
  cnn_voicemail_manager_ = new CnnVoicemailManager(FLAGS_cnn_voicemail_language_to_model_file);
  LOG(INFO) << "new MuteManager: " << FLAGS_mute_detect_json_config_file;
  mute_detect_manager_ = new MuteDetectManager(FLAGS_mute_detect_json_config_file);

  wav_save_dir_ = FLAGS_wav_save_dir;
}


bool SessionStream::save_signal_base64_string(
    const std::string & language,
    const std::string & call_id,
    const std::string & scene_id,
    const std::string & signal_base64_string
    ) {
  auto millisecond_clock = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());
  std::int64_t timestamp_now = millisecond_clock.count();
  std::string filename = wav_save_dir_ + "/" + call_id + "_" + language + "_" +
      scene_id + "_" + std::to_string(timestamp_now) + ".txt";

  //fopen_s raise a error.
  FILE* fp = fopen(filename.c_str(), "w");
  if (fp == NULL) {
    std::cout << "Error in open" << filename;
    return false;
  }

  fputs(signal_base64_string.c_str(), fp);
  fclose(fp);
  return true;
}


std::vector<TaskStatus> SessionStream::update_stream(
    const std::string & language,
    const std::string & call_id,
    const std::string & scene_id,
    const std::string & signal_base64_string
    ) {
  // language
  std::string valid_language = language;

  std::vector<TaskStatus> task_status_vector;

  // save wav
  auto languages_to_skip_save_wav_set_item = languages_to_skip_save_wav_set_.find(valid_language);
  if (languages_to_skip_save_wav_set_item == languages_to_skip_save_wav_set_.end()) {
    this->save_signal_base64_string(valid_language, call_id, scene_id, signal_base64_string);
  }

  // language map by scene_id
  auto scene_id_to_language_map_item = scene_id_to_language_map_.find(scene_id);
  if (scene_id_to_language_map_item != scene_id_to_language_map_.end()) {
    LOG(INFO) << "map language from " << valid_language \
              << " to " << scene_id_to_language_map_item->second \
              << " for scene id " << scene_id;

    valid_language = scene_id_to_language_map_item->second;

  }

  // base64string to wav buffer
  std::string decoded = toolbox::base64_decode(signal_base64_string);

  toolbox::WavFile wav_file = toolbox::WavFile();
  wav_file.fromBytes(reinterpret_cast<const unsigned char*>(decoded.c_str()), decoded.length());

  //task: beep detect
  LOG(INFO) << "beep detect start";
  TaskContextProcess * beep_detect_context = beep_detect_manager_->process(valid_language, call_id, scene_id, wav_file);

  TaskStatus task_status_beep_detect = TaskStatus(
      "beep detect",
      beep_detect_context->message_,
      beep_detect_context->label_,
      beep_detect_context->status_
      );
  task_status_vector.push_back(task_status_beep_detect);

  //task: cnn voicemail
  LOG(INFO) << "cnn voicemail start";
  TaskContextProcess * cnn_voicemail_context = cnn_voicemail_manager_->process(valid_language, call_id, scene_id, wav_file);

  TaskStatus task_status_cnn_voicemail = TaskStatus(
      "cnn voicemail",
      cnn_voicemail_context->message_,
      cnn_voicemail_context->label_,
      cnn_voicemail_context->status_
  );
  task_status_vector.push_back(task_status_cnn_voicemail);

  //task: mute detect
  LOG(INFO) << "mute detect start";
  TaskContextProcess * mute_detect_context = mute_detect_manager_->process(valid_language, call_id, scene_id, wav_file);
  TaskStatus task_status_mute_detect = TaskStatus(
      "mute detect",
      mute_detect_context->message_,
      mute_detect_context->label_,
      mute_detect_context->status_
  );
  task_status_vector.push_back(task_status_mute_detect);

  //return
  return std::move(task_status_vector);
}


//singleton
SessionStream * session_stream = nullptr;


SessionStream * get_session_stream_instance() {
  if (session_stream == nullptr) {
    session_stream = new SessionStream();
    session_stream->init();
  }
  return session_stream;
}

