#include "io/sequence.hpp"

#include <glibmm/init.h>
#include <sstream>

static const char *INPUT1 =
"S 'test_sequence' 0 3 2 0 1 4 0 0\n"
"TF\n"
"L 3\n"
"I 0 1\n"
"I 1 1\n"
"I 3 0\n"
"M0-0 12144384 -1 -101 -102 -103 -104 -105 -106 -107 -108 990 3889 65535 -1000\n"
"R1 0 1 2 3 4 5 H 10 11 12 13 14 15 16 17 18\n"
"M1-0 12144384 -1 -999999 -999999 -999999 -999999 -999999 -999999 -999999 -999999 1046 2317 65535 -999999\n"
"M2-0 12144384 -1 -999999 -999999 -999999 -999999 -999999 -999999 -999999 -999999 1006 1603 65535 -999999\n";

int main() {
  Glib::init();

  std::istringstream istr(INPUT1);
  auto seqPtr = IO::Sequence::readStream(istr);

  std::ostringstream ostr;
  seqPtr->writeStream(ostr);

  std::string result = ostr.str();
  return result != INPUT1 ? 1 : 0;
}

