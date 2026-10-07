#include <cstdlib>

namespace task
{
  struct MessageRAII
  {
    char* line;

    MessageRAII():
      line(nullptr)
    {}

    MessageRAII(const MessageRAII&) = delete;
    MessageRAII& operator=(const MessageRAII&) = delete;
    MessageRAII(MessageRAII&&) = delete;
    MessageRAII& operator=(MessageRAII&&) = delete;

    ~MessageRAII()
    {
      free(line);
    }
  };
}
