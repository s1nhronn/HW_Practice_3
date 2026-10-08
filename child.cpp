#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <windows.h>
#include "MessageRAII.hpp"

namespace task
{
  DWORD recv(DWORD& err, HANDLE rd, char* b, size_t k);
  DWORD recvSize(DWORD& err, HANDLE rd, size_t& len);
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cerr << "child: the argument is not specified" << '\n';
    return 1;
  }

  HANDLE rd{reinterpret_cast< HANDLE >(std::stoull(argv[1]))};
  DWORD err = 0;

  size_t len = 0;
  if (task::recvSize(err, rd, len) != sizeof(size_t))
  {
    std::cerr << "child: " << err << '\n';
    CloseHandle(rd);
    return 1;
  }

  task::MessageRAII msgRAII{};
  msgRAII.line = static_cast< char* >(malloc((len + 1) * sizeof(char)));
  if (!msgRAII.line)
  {
    perror("child: memory allocation error");
    CloseHandle(rd);
    return 1;
  }

  if (task::recv(err, rd, msgRAII.line, len) != len)
  {
    std::cerr << "child: " << err << '\n';
    CloseHandle(rd);
    return 1;
  }

  msgRAII.line[len] = '\0';
  CloseHandle(rd);
  std::cout << msgRAII.line << '\n';
}

DWORD task::recv(DWORD& err, HANDLE rd, char* b, size_t k)
{
  DWORD r = 0;
  BOOL st = true;
  DWORD h = 0;
  while (r < k)
  {
    st = ReadFile(rd, b + r, static_cast< DWORD >(k - r), &h, NULL);
    if (!st)
    {
      err = GetLastError();
      break;
    }
    r += h;
  }
  return r;
}

DWORD task::recvSize(DWORD& err, HANDLE rd, size_t& len)
{
  return recv(err, rd, reinterpret_cast< char* >(&len), sizeof(len));
}
