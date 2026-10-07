#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <windows.h>
#include "MessageRAII.hpp"

namespace task
{
  DWORD send(DWORD& err, HANDLE wr, const char* b, DWORD k);
  DWORD sendSize(DWORD& err, HANDLE wr, size_t sz);
  ssize_t myGetline(char** lineptr, size_t* n); // на винде нету сишного getline'а :D
}

int main()
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

  int pps[2] = {}, err = pipe(pps);
  if (err)
  {
    perror("main: pipe error");
    return err;
  }

  int rd = pps[0], wr = pps[1];

  pid_t pid = fork();

  if (pid == -1)
  {
    perror("main: fork error");
    return 1;
  }
  if (!pid)
  {
    err = close(wr);
    if (err)
    {
      perror("fork: wr close error");
      return err;
    }

    char p[100] = {};
    err = sprintf(p, "%d", rd);
    if (err <= 0)
    {
      perror("fork: sprintf error");
      return 1;
    }

    execl("child", "child", p, NULL);
    perror("fork: execl error");
    return 1;
  }

  err = close(rd);
  if (err)
  {
    perror("main: rd close error");
    return err;
  }

  ssize_t sendErr = 0;
  task::sendSize(sendErr, wr, static_cast< size_t >(len));
  if (sendErr <= 0)
  {
    perror("main: sendSize error");
    return 1;
  }

  task::send(sendErr, wr, msgRAII.line, len);
  if (sendErr <= 0)
  {
    perror("main: send error");
    return 1;
  }

  err = close(wr);
  if (err)
  {
    perror("main: wr close error");
    return err;
  }

  err = waitpid(pid, 0, 0);
  if (err != pid)
  {
    perror("main: waitpid error");
    return err;
  }
}

DWORD send(DWORD& err, HANDLE wr, const char* b, DWORD k)
{
  DWORD r = 0;
  WINBOOL st = true;
  DWORD h = 0;
  while (r < k)
  {
    st = WriteFile(wr, b + r, k - r, &h, NULL);
    if (!st)
    {
      err = GetLastError();
      break;
    }
    r += h;
  }
  return r;
}

DWORD sendSize(DWORD& err, HANDLE wr, size_t sz)
{
  return send(err, wr, reinterpret_cast< const char* >(&sz), sizeof(sz));
}

ssize_t myGetline(char** lineptr, size_t* n)
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
