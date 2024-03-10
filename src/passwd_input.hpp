#pragma once

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

#include <termios.h>
#include <unistd.h>

namespace play {

struct PasswdInput {
  virtual std::string request(std::string_view prompt) = 0;
  virtual ~PasswdInput() = default;
};

class UnixPasswdInput : PasswdInput {
private:
  struct PasswdGuard {
    termios oldtc;
    bool guarded = false;

    PasswdGuard() {
      if (isatty(STDIN_FILENO)) {
        struct termios tc;
        tcgetattr(STDIN_FILENO, &tc);
        memcpy(&oldtc, &tc, sizeof(tc));
        guarded = true;
        // 一些 io 控制 flag：
        // ICANON（默认开启），缓冲用户的输入，允许退格、方向等编辑功能，直至回车才会真正提交输入。关闭后禁用缓冲编辑，可逐字符读取各种字符（包括退格等）
        // ECHO（默认开启），回显用户输入的内容
        tc.c_lflag &= ~ICANON;
        tc.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &tc);
      }
    }
    ~PasswdGuard() {
      if (guarded) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldtc);
      }
    }
  };

public:
  std::string request(std::string_view prompt = "") override {
    if (!prompt.empty()) fputs(prompt.data(), stderr);
    PasswdGuard guard;
    std::string res;
    while (true) {
      int c = getchar();
      if (c == EOF) break;
      if (strchr("\n\r", c)) {
        fputc('\n', stderr);
        break;
      }
      if (c == '\b' || c == '\x7f') {
        if (!res.empty()) {
          res.pop_back();
          fputs("\b \b", stderr);
        }
      } else {
        res.push_back(c);
        fputc('*', stderr);
      }
    }
    return res;
  };
};

} // namespace play
