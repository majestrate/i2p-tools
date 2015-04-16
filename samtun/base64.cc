#include "base64.hpp"
//
// http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
// 

char b64_encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                             'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                             'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                             'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                             'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                             'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                             'w', 'x', 'y', 'z', '0', '1', '2', '3',
                             '4', '5', '6', '7', '8', '9', '~', '-'};

char b32_encoding_table[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                             'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
                             'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
                             'y', 'z', '2', '3', '4', '5', '6', '7'};

char b32_emask_table[] = { 0x1f, 0x01, 0x03, 0x07, 0x0f };

char *decoding_table = nullptr;



void build_decoding_table() {

  decoding_table = new char[256];

  for (int i = 0; i < 64; i++)
    decoding_table[(uint8_t) b64_encoding_table[i]] = i;
}



uint8_t *base64_decode(std::string str, size_t * output_length) {
  const char * data = str.c_str();
  if (decoding_table == nullptr) build_decoding_table();
  auto input_length = str.size();
  if (input_length % 4 != 0) return nullptr;

  *output_length = input_length / 4 * 3;
  if (data[input_length - 1] == '=') (*output_length)--;
  if (data[input_length - 2] == '=') (*output_length)--;

  uint8_t *decoded_data = new uint8_t[*output_length];

  for (size_t i = 0, j = 0; i < input_length;) {

    uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
    uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
    uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
    uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

    uint32_t triple = (sextet_a << 3 * 6)
      + (sextet_b << 2 * 6)
      + (sextet_c << 1 * 6)
      + (sextet_d << 0 * 6);

    if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
    if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
    if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
  }

  return decoded_data;
}


std::string base32_encode(uint8_t * buff, size_t bufflen) {
  std::string ret;
  int32_t usedbits = 0;
  for (int idx = 0; idx < bufflen; ) {
    int32_t fivebits = 0;
    if (usedbits < 3) {
      fivebits = (buff[idx] >> (3 - usedbits)) & 0x1f;
      usedbits += 5;
    } else if(usedbits == 3) {
      fivebits = buff[idx++] & 0x1f;
      usedbits = 0;
    } else {
      fivebits = (buff[idx++] << (usedbits - 3)) & 0x1f;
      if ( idx < bufflen) {
        usedbits -= 3;
        fivebits |= (buff[idx] >> (8 - usedbits)) & b32_emask_table[usedbits];
      }
      ret += b32_encoding_table[fivebits];
    }
  }
  return ret;
}
