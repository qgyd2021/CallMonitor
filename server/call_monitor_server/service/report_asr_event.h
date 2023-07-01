//
// Created by tianx on 2023/4/20.
//

#ifndef SERVICE_REPORT_ASR_EVENT_H
#define SERVICE_REPORT_ASR_EVENT_H

#include <string>

bool report_asr_event(
    std::string & product_id, std::string & call_id,
    std::string & language, int event_type, std::string & text,
    std::string & asr_event_http_host_port,
    std::string & asr_event_uri,
    std::string & secret_key
);



#endif //SERVICE_REPORT_ASR_EVENT_H
