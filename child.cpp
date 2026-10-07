#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <windows.h>
#include "MessageRAII.hpp"

namespace task
{
  DWORD recv(DWORD& err, HANDLE rd, char* b, DWORD k);
  DWORD recvSize(DWORD& err, HANDLE rd, size_t& len);
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cerr << "child: the argument is not specified" << '\n';
  }

  HANDLE rd{reinterpret_cast< HANDLE >(std::stoull(argv[1]))};
  char msg[256] = {};
  DWORD err = 0, k = 255;
  if (task::recv(err, rd, msg, k) != k)
  {
    std::cerr << err << '\n';
    CloseHandle(rd);
    return 1;
  }
  CloseHandle(rd);
  printf("%s", msg);
}

DWORD recv(DWORD& err, HANDLE rd, char* b, DWORD k)
{
  DWORD r = 0;
  WINBOOL st = true;
  DWORD h = 0;
  while (r < k)
  {
    st = ReadFile(rd, b + r, k - r, &h, NULL);
    if (!st)
    {
      err = GetLastError();
      break;
    }
    r += h;
  }
  return r;
}

DWORD recvSize(DWORD& err, HANDLE rd, size_t& len)
{
  return recv(err, rd, reinterpret_cast< char* >(&len), sizeof(len));
}
