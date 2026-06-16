#ifndef _metaUtils_h
#define _metaUtils_h

#include <string>

std::string compressAndEncode(const std::string& input);
std::string decodeAndDecompress(const std::string& input, size_t maxOutputSize = 1024 * 1024);
#endif
