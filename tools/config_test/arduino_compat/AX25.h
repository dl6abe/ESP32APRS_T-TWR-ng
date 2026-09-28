#pragma once
struct AX25Msg {
  char len;
  unsigned char info[256];
};
