//
// Created by tianx on 2023/3/7.
//

#ifndef CONTROL_UPDATE_STREAM_H
#define CONTROL_UPDATE_STREAM_H

#include <httplib.h>


std::function<void(const httplib::Request &, httplib::Response &)> control_update_stream();


#endif //CONTROL_UPDATE_STREAM_H
