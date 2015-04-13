#ifndef BASE64_HPP
#define BASE64_HPP
#include <string>

// decode i2p base64
uint8_t *base64_decode(std::string data, size_t *output_length);

#endif
