#include <iostream>

#define CPPHTTPLIB_THREAD_POOL_COUNT 8u

#include "server/call_monitor_server/call_monitor_server.h"


int main(int argc, char *argv[])
{
  std::cout << "Hello world!" << std::endl;
  callMonitorServer(argc, argv);
  return 0;
}
