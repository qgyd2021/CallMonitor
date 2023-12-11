//
// Created by tianx on 2023/2/3.
//

#include <gflags/gflags.h>

DEFINE_string(call_monitor_log_dir, "./logs", "call monitor log dir");
//日志级别 INFO, WARNING, ERROR, FATAL 的值分别为0, 1, 2, 3.
DEFINE_uint32(call_monitor_stderrthreshold, 0, "call monitor stderr threshold");

DEFINE_string(project_name, "CallMonitor", "a string to indicate the name of the server");
DEFINE_string(http_host, "0.0.0.0", "server host");
DEFINE_uint32(http_port, 4070, "server port");

DEFINE_string(cnn_voicemail_gray_box_testing_file, "server/call_monitor_server/json_config/cnn_voicemail_language_to_model.json", "cnn_voicemail_gray_box_testing_file");
DEFINE_string(cnn_voicemail_language_to_model_file, "server/call_monitor_server/json_config/cnn_voicemail_language_to_model.json", "cnn_voicemail_language_to_model_file");
DEFINE_string(voicemail_json_file, "server/call_monitor_server/json_config/voicemail.json", "voicemail_json_file");
DEFINE_string(scene_id_to_language_file, "server/call_monitor_server/json_config/scene_id_to_language.json", "scene_id_to_language_file");
DEFINE_string(languages_to_skip_save_wav_file, "server/call_monitor_server/json_config/languages_to_skip_save_wav.json", "languages_to_skip_save_wav_file");
DEFINE_string(mute_detect_json_config_file, "server/call_monitor_server/json_config/mute_detect_json_config.json", "mute_detect_json_config_file");

//voicemail callback
//HK callbot_asr_event_url=http://10.52.66.97:8002/svrapi/callbot/voip/asrevent
//HK secret_key=9672be65ed3031752b385b1c930aeca6
//DEV callbot_asr_event_url=http://10.20.251.7:8002/svrapi/callbot/voip/asrevent
//DEV secret_key=9672be65ed3031752b385b1c930aeca6
DEFINE_string(asr_event_http_host_port, "http://10.52.66.97:8002", "asr_event_http_host_port");
DEFINE_string(asr_event_uri, "/svrapi/callbot/voip/asrevent", "asr_event_uri");
DEFINE_string(secret_key, "9672be65ed3031752b385b1c930aeca6", "secret_key");

DEFINE_string(wav_save_dir, "/data/tianxing/update_stream_wav", "wav_save_dir");
//DEFINE_string(wav_save_dir, "D:/Users/tianx/CLionProjects/CallMonitor/update_stream_wav", "wav_save_dir");
