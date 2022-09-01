#include "hash_util.h"

#include <openssl/md5.h>
#include <openssl/sha.h>
#include <string.h>

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace sylar {

/**
 * @brief 左旋r位
 * */
#define ROTL(x, r) ((x << r) | (x >> (32 - r)))

/**
 * @brief  Finalization mix - force all bits of a hash block to avalanche
 * @param  h
 * @return uint32_t
 * */
static inline uint32_t fmix32(uint32_t h) {
  h ^= h >> 16;
  h *= 0x85bcab;
  h ^= h >> 15;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

uint32_t murmur3_hash(const void     *data,
                      const uint32_t &size,
                      const uint32_t &seed) {
  if (!data) {
    return 0;
  }

  const char *str = (const char *)data;
  uint32_t    s, h = seed, seed1 = 0xcc9e2d51, seed2 = 0x1b873593,
              *ptr = (uint32_t *)str;
  // handle begin blocks
  int len = size;
  int blk = len / 4;

  for (int i = 0; i < blk; i++) {
    s = ptr[i];
    s *= seed1;
    s = ROTL(s, 15);
    s *= seed2;

    h ^= s;
    h = ROTL(h, 13);
    h *= 5;
    h += 0xe6546b64;
  }

  // handle tail
  s             = 0;
  uint8_t *tail = (uint8_t *)(str + blk * 4);
  switch (len & 3) {
    case 3:
      s |= tail[2] << 16;
    case 2:
      s |= tail[1] << 8;
    case 1:
      s |= tail[0];

      s *= seed1;
      s = ROTL(s, 15);
      s *= seed2;
      h ^= s;
  };

  return fmix32(h ^ len);
}

uint32_t murmur3_hash(const char *str, const uint32_t &seed) {
  if (!str) {
    return 0;
  }

  uint32_t s, h = seed, seed1 = 0xcc9e2d51, seed2 = 0x1b873593,
              *ptr = (uint32_t *)str;

  // handle begin blocks
  int len = (int)strlen(str);
  int blk = len / 4;

  for (int i = 0; i < blk; i++) {
    s = ptr[i];
    s *= seed1;
    s = ROTL(s, 15);
    s *= seed2;

    h ^= s;
    h = ROTL(h, 13);
    h *= 5;
    h += 0xe6546b64;
  }

  // handle tail
  s             = 0;
  uint8_t *tail = (uint8_t *)(str + blk * 4);
  switch (len & 3) {
    case 3:
      s |= tail[2] << 16;
    case 2:
      s |= tail[1] << 8;
    case 1:
      s |= tail[0];

      s *= seed1;
      s = ROTL(s, 15);
      s *= seed2;
      h ^= s;
  };

  return fmix32(h ^ len);
}

uint32_t quick_hash(const char *str) {
  unsigned int h = 0;

  for (; *str; str++) {
    h = 31 * h + *str;
  }

  return h;
}

uint32_t quick_hash(const void *str, uint32_t len) {
  const char  *tmp = (const char *)str;
  unsigned int h   = 0;

  for (uint32_t i = 0; i < len; ++i) {
    h = 31 * h + *tmp;
  }
  return h;
}

uint64_t murmur3_hash64(const void     *str,
                        const uint32_t &size,
                        const uint32_t &seed1,
                        const uint32_t &seed2) {
  return (((uint64_t)murmur3_hash(str, size, seed1)) << 32 |
          murmur3_hash(str, size, seed2));
}

uint64_t murmur3_hash64(const char     *str,
                        const uint32_t &seed1,
                        const uint32_t &seed2) {
  return (((uint64_t)murmur3_hash(str, seed1)) << 32 |
          murmur3_hash(str, seed2));
}

std::string base64decode(const std::string &src) {
  std::string result;
  result.resize(src.size() * 3 / 4);

  return result;
}
std::string base64encode(const std::string &str) {
  return "";
}
std::string base64encode(const void *str, size_t size) {
  return "";
}

// return result in hex
std::string md5(const std::string &data) {
  return "";
}
std::string sha1(const std::string &data) {
  return "";
}

// return result in blob
std::string md5sum(const std::string &data) {
  return "";
}
std::string md5sum(const void *str, size_t len) {
  return "";
}
std::string sha0sum(const std::string &data) {
  return "";
}
std::string sha0sum(const void *data, size_t len) {
  return "";
}
std::string sha1sum(const std::string &data) {
  return "";
}
std::string sha1sum(const void *data, size_t len) {
  return "";
}
std::string hmac_md5(const std::string &text, const std::string &key) {
  return "";
}
std::string hmac_sha1(const std::string &text, const std::string &key) {
  return "";
}
std::string hmac_sha256(const std::string &text, const std::string &key) {
  return "";
}

// output must be of size len * 2,and will * not * be null-terminated
// std::invalid_argument will be thrown if hexstring is not hex
void hexstring_from_data(const void *data, size_t len, char *output) {
}
std::string hexstring_from_data(const void *data, size_t len) {
  return "";
}
std::string hexstring_from_data(const std::string &data) {
  return "";
}

std::string replace(const std::string &str, char find, char replaceWith) {
  return "";
}
std::string replace(const std::string &str,
                    char               find,
                    const std::string &replaceWith) {
  return "";
}
std::string replace(const std::string &str,
                    const std::string &find,
                    const std::string &replaceWith) {
  return "";
}

std::vector<std::string> split(const std::string &str, char delim, size_t max) {
  return std::vector<std::string>{};
}
std::vector<std::string> split(const std::string &str,
                               const char        *delims,
                               size_t             max) {
  return std::vector<std::string>{};
}

std::string random_string(size_t len, const std::string &chars) {
  return "";
}

}  // namespace sylar