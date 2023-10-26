//
// Created by tianx on 2023/2/3.
//

#ifndef CALL_MONITOR_SETTINGS_H
#define CALL_MONITOR_SETTINGS_H

#include <gflags/gflags.h>

DECLARE_string(call_monitor_log_dir);
DECLARE_uint32(call_monitor_stderrthreshold);

DECLARE_string(project_name);
DECLARE_string(http_host);
DECLARE_uint32(http_port);

DECLARE_string(cnn_voicemail_gray_box_testing_file);
DECLARE_string(cnn_voicemail_language_to_model_file);
DECLARE_string(scene_id_to_language_file);
DECLARE_string(languages_to_skip_save_wav_file);
DECLARE_string(mute_detect_json_config_file);

DECLARE_string(asr_event_http_host_port);
DECLARE_string(asr_event_uri);
DECLARE_string(secret_key);

DECLARE_string(wav_save_dir);


#endif //CALL_MONITOR_SETTINGS_H
