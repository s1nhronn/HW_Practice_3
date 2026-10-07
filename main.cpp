#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include "MessageRAII.hpp"

namespace task
{
  ssize_t send(ssize_t& err, int wr, const char* b, ssize_t k);
  ssize_t sendSize(ssize_t& err, int wr, size_t sz);
}

int main()
{
  task::MessageRAII msgRAII{};
  size_t cap = 0;

  ssize_t len = getline(&msgRAII.line, &cap, stdin);

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

ssize_t task::send(ssize_t& err, int wr, const char* b, ssize_t k)
{
  ssize_t r = 0;
  while (r < k)
  {
    err = write(wr, b + r, static_cast< size_t >(k - r));
    if (err <= 0)
    {
      break;
    }
    r += err;
  }
  return r;
}

ssize_t task::sendSize(ssize_t& err, int wr, size_t sz)
{
  return send(err, wr, reinterpret_cast< const char* >(&sz), sizeof(size_t));
}
