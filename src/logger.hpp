#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <source_location>

#define FOR_LOG_LEVEL(f) f(Trace) f(Debug) f(Info) f(Warn) f(Error)

enum class LogLevel : std::uint8_t {
#define _FUNC(name) name,
  FOR_LOG_LEVEL(_FUNC)
#undef _FUNC
};

inline std::string getlogLevalName(LogLevel lv) {
#define _FUNC(name) \
  if (lv == LogLevel::name) return #name;
  FOR_LOG_LEVEL(_FUNC)
#undef _FUNC
  return "unkown";
}

inline LogLevel getLogLevel(std::string lv) {
#define _FUNC(name) \
  if (lv == #name) return LogLevel::name;
  FOR_LOG_LEVEL(_FUNC)
#undef _FUNC
  return LogLevel::Debug;
}

#if defined(__linux__) || defined(__APPLE__)
#define ANSI_AVAILABLE
// 各日志级别对应的颜色控制码
inline constexpr std::string_view k_ansi_colors[static_cast<std::size_t>(LogLevel::Error) + 1] = {
    "\E[37m",   // Trace-白色
    "\E[32m",   // Debug-绿色
    "\E[34m",   // Info-蓝色
    "\E[33m",   // Warn-黄色
    "\E[31;1m", // Error-红色加粗
};
// 用于清除控制
inline constexpr std::string_view k_ansi_reset = "\E[m";
#endif

inline LogLevel g_mute_log_level = []() -> LogLevel {
  if (auto name = std::getenv("MUTE_LOG_LEVEL")) {
    return getLogLevel(name);
  }
  return LogLevel::Debug;
}();

inline void set_mute_log_level(LogLevel lv) { g_mute_log_level = lv; };

inline std::ofstream g_log_file = []() -> std::ofstream {
  if (auto path = std::getenv("LOG_FILE")) {
    return std::ofstream(path, std::ios::app);
  }
  return std::ofstream();
}();

inline void set_log_file(std::string path) { g_log_file = std::ofstream(path, std::ios::app); }

// 介入「格式字符串->std::format_string」的隐式构造，在构造过程中以默认参数的形式顺带构造
// source_location，以获取调用处的位置信息
template <class T>
struct with_source_location {
private:
  T inner;
  std::source_location loc;

public:
  template <class U>
  consteval with_source_location(U &&inner, std::source_location loc = std::source_location::current())
      : inner(std::forward<U>(inner)), loc(std::move(loc)) {}
  constexpr const T &format() const { return inner; }
  constexpr const std::source_location &location() const { return loc; }
};

inline void output_log(LogLevel lv, const std::string &msg, const std::source_location &loc) {
  std::chrono::zoned_time now(std::chrono::current_zone(), std::chrono::high_resolution_clock::now());
  std::string final_output =
      std::format("{:%F %X} [{}] {}:{} {}\n", now, getlogLevalName(lv), loc.file_name(), loc.line(), msg);
  // 如果指定了日志文件，输出一份日志至文件中
  if (g_log_file) {
    g_log_file << final_output;
  }
  if (lv <= g_mute_log_level) {
    return;
  }

#ifdef ANSI_AVAILABLE
  std::ostringstream oss;
  oss << k_ansi_colors[static_cast<std::size_t>(lv)] << final_output << k_ansi_reset;
  std::cout << oss.str();
#else
  std::cout << final_output;
#endif
}

template <class... Args>
void generic_log(LogLevel lv, with_source_location<std::format_string<Args...>> fmt, Args &&...args) {
  auto const &loc = fmt.location();
  auto msg = std::vformat(fmt.format().get(), std::make_format_args(args...));
  output_log(lv, msg, loc);
}

#define _FUNC(name)                                                                       \
  template <class... Args>                                                                \
  void log##name(with_source_location<std::format_string<Args...>> fmt, Args &&...args) { \
    generic_log(LogLevel::name, std::move(fmt), std::forward<Args>(args)...);             \
  }
FOR_LOG_LEVEL(_FUNC)
#undef _FUNC

#define LOG_P(x) logDebug(#x " = {}", (x));

// inline void foo() {
//   int num = 114514;
//   // 空大括号占位，顺序对应参数顺序
//   std::cout << std::format("hello {}\n", num);
//   // 亦可指定填充顺序
//   std::cout << std::format("hello {1}, {0}\n", num, num + 1);
//
//   // 在冒后指定格式，冒号前指定顺序
//   // :4f 表示浮点表示且保留四位小数
//   std::cout << std::format("hello [{:.4f}]\n", float(num));
//   // :>10d 表示右边对齐十进制显示且填充至10位
//   std::cout << std::format("hello [{:10d}]\n", num);
//   // :_<#10x 表示左对齐十六进制显示且填充至10位（#表示显示进制标识，_<表示左对齐使用_填充）
//   std::cout << std::format("hello [{:_<#10x}]\n", num);
//   // :^10.3s 表示居中显示字符串的前3位，且填充至10位显示
//   std::cout << std::format("hello [{:^10.3s}]\n", "114514");
//
//   // 执行位置信息
//   auto loc = std::source_location::current();
//   std::cout << std::format("line={} column={} file={} func={} \n", loc.line(), loc.column(), loc.file_name(),
//                            loc.function_name());
// }
