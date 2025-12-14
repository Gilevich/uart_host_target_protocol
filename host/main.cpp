#include <iostream>
#include "host.hpp"

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: host_target_uart_connection COMx" << std::endl;
    return 0;
  }

  Host host(argv[1]);

  host.connect();
}