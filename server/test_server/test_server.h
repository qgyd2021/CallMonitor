//
// Created by tianx on 2022/10/8.
//
#ifndef TEST_SERVER_H
#define TEST_SERVER_H


#include <httplib.h>


int testServer(int argc, char *argv[])
{
    // HTTP
    httplib::Server svr;

    svr.Get("/hi", [](const httplib::Request &, httplib::Response &res) {
        res.set_content("Hello World!", "text/plain");
    });

    // curl http://127.0.0.1:8080/hi
    svr.listen("0.0.0.0", 8080);
    return 0;
}

#endif //TEST_SERVER_H
