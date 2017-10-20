#include <cstdio>
#include <cstdint>

uint64_t libc_id = 0;
inline uint64_t GenID(uint32_t FTID) {
  return ((libc_id++ << 16) | FTID);
}

inline uint64_t CheckID(uint64_t id) {
  return (id >> 16);
}
inline uint64_t CheckType(uint64_t id) {
  return (id & 0xFFFF);
}

int main() {
  uint64_t id0 = GenID(4);
  uint64_t id1 = GenID(16);
  uint64_t id2 = GenID(31);
  uint64_t id3 = GenID(93);
  uint64_t id4 = GenID(16);
  uint64_t id5 = GenID(16);
  uint64_t id6 = GenID(31);
  uint64_t id7 = GenID(93);
  uint64_t id8 = GenID(4);

  printf("0 %lu %lu %lu\n", id0, CheckID(id0), CheckType(id0));
  printf("1 %lu %lu %lu\n", id1, CheckID(id1), CheckType(id1));
  printf("2 %lu %lu %lu\n", id2, CheckID(id2), CheckType(id2));
  printf("3 %lu %lu %lu\n", id3, CheckID(id3), CheckType(id3));
  printf("4 %lu %lu %lu\n", id4, CheckID(id4), CheckType(id4));
  printf("5 %lu %lu %lu\n", id5, CheckID(id5), CheckType(id5));
  printf("6 %lu %lu %lu\n", id6, CheckID(id6), CheckType(id6));
  printf("7 %lu %lu %lu\n", id7, CheckID(id7), CheckType(id7));
  printf("8 %lu %lu %lu\n", id8, CheckID(id8), CheckType(id8));

  return 0;
}
