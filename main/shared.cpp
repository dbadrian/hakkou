#include "shared.h"

#include "cstdio"

namespace hakkou {
TimeString time_to_string_representation(u32 time) {
  TimeString buf = {};
  u8 hours = time / 3600;
  u8 minutes = (time / 60) % 60;
  sprintf(buf.data(), "%03uh%02um", hours, minutes);
  return buf;
}

}  // namespace hakkou