#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <minwindef.h>
#include <unistd.h>
#include <windows.h>
#include "MessageRAII.hpp"

namespace task
{
  DWORD send(DWORD& err, HANDLE wr, const char* b, size_t k);
  DWORD sendSize(DWORD& err, HANDLE wr, size_t sz);
  ssize_t myGetline(char** lineptr, size_t* n); // на винде нету сишного getline'а :D
}

int main(int argc, char** argv)
{
  task::MessageRAII msgRAII{};
  size_t cap = 0;

  ssize_t len = task::myGetline(&msgRAII.line, &cap);

  if (len == -1)
  {
    if (ferror(stdin))
    {
      perror("main: getline error");
      return 1;
    }
    return 0;
  }

  if (argc < 2)
  {
    std::cerr << "main: the argument is not specified" << '\n';
    return 1;
  }
  HANDLE read, write;
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = FALSE;
  if (!CreatePipe(&read, &write, &sa, 256))
  {
    std::cerr << "main: " << GetLastError() << '\n';
    return 1;
  }
  SetHandleInformation(read, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
  PROCESS_INFORMATION pi = {};
  STARTUPINFOA si{};
  si.cb = sizeof(si);
  std::string cmd = std::string(argv[1]) + ' ' + std::to_string(reinterpret_cast< DWORD_PTR >(read));
  if (!CreateProcessA(argv[1], &cmd[0], NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
  {
    CloseHandle(read);
    CloseHandle(write);
    std::cerr << "main: " << GetLastError() << '\n';
    return 1;
  }
  CloseHandle(read);
  DWORD err = 0;
  if (task::sendSize(err, write, static_cast< size_t >(len)) != sizeof(size_t))
  {
    std::cerr << "main: " << err << '\n';
    CloseHandle(write);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 1;
  }

  if (task::send(err, write, msgRAII.line, static_cast< size_t >(len)) != len)
  {
    std::cerr << "main: " << err << '\n';
    CloseHandle(write);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 1;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(write);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
}

DWORD task::send(DWORD& err, HANDLE wr, const char* b, size_t k)
{
  DWORD r = 0;
  BOOL st = true;
  DWORD h = 0;
  while (r < k)
  {
    st = WriteFile(wr, b + r, static_cast< DWORD >(k - r), &h, NULL);
    if (!st)
    {
      err = GetLastError();
      break;
    }
    r += h;
  }
  return r;
}

DWORD task::sendSize(DWORD& err, HANDLE wr, size_t sz)
{
  return send(err, wr, reinterpret_cast< const char* >(&sz), sizeof(sz));
}

ssize_t task::myGetline(char** lineptr, size_t* n)
{
  if (!lineptr || !n)
  {
    return -1;
  }

  if (!*lineptr || *n == 0)
  {
    *n = 128;
    *lineptr = static_cast< char* >(malloc(*n));
    if (!*lineptr)
    {
      return -1;
    }
  }

  size_t pos = 0;
  int c;

  while ((c = getchar()) != EOF)
  {
    if (pos + 1 >= *n)
    {
      size_t newN = *n * 2;
      char* newPtr = static_cast< char* >(realloc(*lineptr, newN));
      if (!newPtr)
      {
        return -1;
      }
      *lineptr = newPtr;
      *n = newN;
    }

    (*lineptr)[pos++] = static_cast< char >(c);
    if (c == '\n')
      break;
  }

  if (!pos && c == EOF)
  {
    return -1;
  }

  (*lineptr)[pos] = '\0';
  return static_cast< ssize_t >(pos);
}
