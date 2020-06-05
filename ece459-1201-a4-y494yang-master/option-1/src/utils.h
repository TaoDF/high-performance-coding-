#pragma once

#include "Container.h"
#include <string>


Container<std::string> readFile(std::string fileName);

std::string readFileLine(std::string fileName, int targetLineNum);

std::string bytesToString(uint8_t* bytes, int len);

uint8_t* sha256(const std::string str);

//void initChecksum();

void xorChecksum(uint8_t* base_checksum, uint8_t* new_checksum);
