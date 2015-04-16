//
// base64.hpp
// copywrong you're mom 2015
//
#ifndef BASE64_HPP
#define BASE64_HPP
#include <string>

// decode i2p base64
uint8_t *base64_decode(std::string data, size_t *output_length);

// encode to i2p base32
std::string base32_encode(uint8_t * buff, size_t bufflen);

#endif
