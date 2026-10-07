#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include "MessageRAII.hpp"

namespace task
{
  ssize_t recv(ssize_t& err, int rd, char* b, ssize_t k);
  ssize_t recvSize(ssize_t& err, int rd, size_t& len);
}

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "child: incorrect args\n");
    return 1;
  }
  int err = 0;
  int rd = std::atoi(argv[1]);
  if (rd <= 0)
  {
    fprintf(stderr, "child: incorrect rd\n");
    return 1;
  }

  ssize_t recvErr = 0;
  size_t len = 0;
  task::recvSize(recvErr, rd, len);
  if (recvErr <= 0)
  {
    perror("child: recvSize error");
    return 1;
  }

  task::MessageRAII msgRAII{};
  msgRAII.line = static_cast< char* >(malloc((len + 1) * sizeof(char)));
  if (!msgRAII.line)
  {
    perror("child: memory allocation error");
    return 1;
  }

  task::recv(recvErr, rd, msgRAII.line, static_cast< ssize_t >(len));
  if (recvErr <= 0)
  {
    perror("child: recv error");
    return 1;
  }

  msgRAII.line[len] = '\0';

  err = close(rd);
  if (err)
  {
    perror("child: rd close error");
    return err;
  }
  err = printf("%s", msgRAII.line);
  if (err < 0)
  {
    perror("child: printf error");
    return 1;
  }
}

ssize_t task::recv(ssize_t& err, int rd, char* b, ssize_t k)
{
  ssize_t r = 0;
  while (r < k)
  {
    err = read(rd, b + r, static_cast< size_t >(k - r));
    if (err <= 0)
    {
      break;
    }
    r += err;
  }
  return r;
}

ssize_t task::recvSize(ssize_t& err, int rd, size_t& len)
{
  return recv(err, rd, reinterpret_cast< char* >(&len), sizeof(size_t));
}
