#pragma once

#include <cstdlib>
#include <iostream>

namespace foam::test {
[[noreturn]] inline void fail(const char* expression, const char* file, int line) {
  std::cerr << file << ':' << line << ": check failed: " << expression << std::endl;
  // Exit works in Qt event callbacks too, without throwing across the event loop.
  std::exit(EXIT_FAILURE);
}
}

// Unlike assert, checks and their expressions run even when NDEBUG is defined.
#define TEST_CHECK(...) do { \
  if (!(__VA_ARGS__)) ::foam::test::fail(#__VA_ARGS__, __FILE__, __LINE__); \
} while (false)
