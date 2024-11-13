#include "seq_util.hpp"

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
  std::istringstream istr(INPUT1);
  auto seqPtr = Img::Sequence::readStream(istr);
  auto& seq = *seqPtr;

  if(!check_header(seq, "test_sequence", 0, 3, 2, 0, 1, 4, 0, 0))
    return 1;

  if(seq.imageFormat() != 'F')
    return 1;
  if(seq.layerCount() != 3)
    return 1;

  if(!check_image(seq.image(0), 0, true))
    return 1;
  if(!check_image(seq.image(1), 1, true))
    return 1;
  if(!check_image(seq.image(2), 3, false))
    return 1;

  if(!check_stats(seq.image(0).m_stats[0], 12144384, -1, -101, -102, -103, -104, -105, -106, -107, -108, 990, 3889, 65535, -1000))
    return 1;
  if(!check_stats(seq.image(0).m_stats[1], 12144384, -1, -999999, -999999, -999999, -999999, -999999, -999999, -999999, -999999, 1046, 2317, 65535, -999999))
    return 1;
  if(!check_stats(seq.image(0).m_stats[2], 12144384, -1, -999999, -999999, -999999, -999999, -999999, -999999, -999999, -999999, 1006, 1603, 65535, -999999))
    return 1;

  if(!check_registration(*seq.image(0).m_registration, 1, 0, 1, 2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 16, 17, 18))
    return 1;

  return 0;
}

