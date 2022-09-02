#include "AbstractSerialChannel.h"
#include <string.h>
#include <sstream>
namespace common {
namespace serial {
int AbstractSerialChannel::readLine(std::string &buffer, const std::string delimiter, int timeo_ms) {
  std::stringstream ss;
  char              buf;
  int               ret = -1;
  buffer                = "";
  std::size_t pos;
  while ((ret = read(&buf, 1, timeo_ms)) == 1) {
    ss << buf;
    if ((pos=ss.str().find(delimiter)) != std::string::npos) {
      buffer = ss.str().substr(0,pos);
      return ss.str().size();
    } else {
      //  ACTDBG << "receive buffer \"" << ss.str()<<"\" ret:"<<ret;
    }
  }
  return ret;
}

int AbstractSerialChannel::read(void *buffer, int maxnb, const char *isanyof, int ms_timeo, int *timeout_arised) {
  int   t_arised = 0;
  int   cnt      = 0;
  char *bufp     = (char *)buffer;
  if (timeout_arised) {
    *timeout_arised = 0;
  }
  while (cnt < maxnb) {
    char buf;
    if (read(&buf, 1, ms_timeo, &t_arised) > 0) {
      bufp[cnt++] = buf;
    }
    if (t_arised) {
      if (timeout_arised) {
        *timeout_arised = t_arised;
        return cnt;
      }
    }
    if (isanyof) {
      for (int i = 0; i < strlen(isanyof); i++) {
        if (isanyof[i] == buf) {
          return cnt;
        }
      }
    }
  }
  return cnt;
}

}  // namespace serial
};  // namespace common
