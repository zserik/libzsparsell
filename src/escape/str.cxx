// source: https://github.com/zserik/zxtw/src/escape.cxx
#include "escape.hpp"

using namespace std;

namespace zsparsell {
  auto escape(const string &s) -> string {
    string ret;
    ret.reserve(s.size());

    for(const auto i : s) {
      // NOTE: prevent creation of too many temporaries
      if(const char tmp = escape(i)) {
        ret += '\\';
        ret += tmp;
      } else {
        ret += i;
      }
    }

    return ret;
  }

  auto unescape(const string &s) -> string {
    string ret;
    ret.reserve(s.size());

    bool is_esc = false;
    for(const auto i : s) {
      bool sw_esc = true;
      const char tmp = ([=, &sw_esc] {
        if(is_esc)    return unescape(i);
        if(i == '\\') return '\0';
        sw_esc = false;
        return i;
      })();

      if(tmp) ret += tmp;
      if(sw_esc) is_esc = !is_esc;
    }

    return ret;
  }
}
