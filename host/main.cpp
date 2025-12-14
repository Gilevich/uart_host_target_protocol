#include <iostream>
#include "host.hpp"

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: host.exe COMx" << std::endl;
    return 0;
  }

  Host host(argv[1]);
  host.connect();
}