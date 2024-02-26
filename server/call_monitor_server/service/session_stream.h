//
// Created by tianx on 2023/3/7.
//

#ifndef SERVICE_SESSION_STREAM_H
#define SERVICE_SESSION_STREAM_H

#include <map>
#include <set>
#include <vector>

#include "task_beep_detect.h"
#include "task_voicemail.h"
#include "task_mute_detect.h"
#include "task_voice_detect.h"


class TaskStatus
{
public:
  std::string task_name_;
  std::string message_ = "blank";
  std::string label_ = "unknown";
  std::string status_ = "acknowledged";

  TaskStatus(std::string task_name, std::string message, std::string label, std::string status);
};


class SessionStream
{
public:
  std::map<std::string, std::string> scene_id_to_language_map_;
  std::set<std::string> languages_to_skip_save_wav_set_;

  BeepDetectManager * beep_detect_manager_;
  VoicemailManager * voicemail_manager_;
  MuteDetectManager * mute_detect_manager_;
  VoiceDetectManager * voice_detect_manager_;

  std::string wav_save_dir_;

  SessionStream()=default;
  ~SessionStream()=default;

  void init();

  std::vector<TaskStatus> update_stream(
      const std::string & language,
      const std::string & callId,
      const std::string & sceneId,
      const std::string & signal_base64_string
      );

  void load_scene_id_to_language_map(const std::string & filename);
  void load_languages_to_skip_wav_set(const std::string & filename);
  bool save_signal_base64_string(
      const std::string & language,
      const std::string & call_id,
      const std::string & scene_id,
      const std::string & signal_base64_string
      );

};

SessionStream * get_session_stream_instance();


#endif //SERVICE_SESSION_STREAM_H
