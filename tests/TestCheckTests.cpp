#include "TestCheck.h"
#include <string_view>

#ifndef NDEBUG
#error This test must exercise checks with NDEBUG defined.
#endif

int main(int argc, char** argv) {
  int evaluations=0;
  TEST_CHECK(++evaluations==1);
  TEST_CHECK(evaluations==1);
  if(argc>1 && std::string_view{argv[1]}=="--fail")TEST_CHECK(false);
  return 0;
}
