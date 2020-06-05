#pragma once

#include "utils.h"
#include <string>
#include <mutex>
#include <openssl/sha.h>
#include <iostream>

// https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern

enum ChecksumType {
    IDEA,
    PACKAGE,
    NUM_CHECKSUMS
};

template <typename T, ChecksumType CT>
class ChecksumTracker
{
    static std::mutex checksumMutex[NUM_CHECKSUMS];
    static uint8_t* globalChecksum[NUM_CHECKSUMS];
    static std::string flag_fill[NUM_CHECKSUMS];

protected:
    static void updateGlobalChecksum(uint8_t* checksum) {
        std::unique_lock<std::mutex> lock(checksumMutex[CT]);
        if (flag_fill[CT].empty())
        {
            flag_fill[CT] = "filled";
            globalChecksum[CT] = new uint8_t[NUM_CHECKSUMS * SHA256_DIGEST_LENGTH];
            for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
            {
                globalChecksum[CT][i] = 0;
            }
        }
        xorChecksum(globalChecksum[CT], checksum);
    }

public:
    ChecksumTracker() {

    }

    ~ChecksumTracker() {
        // nop
    }

    static std::string getGlobalChecksum() {
        std::unique_lock<std::mutex> lock(checksumMutex[CT]);
        std::string checksum;
        checksum = bytesToString(globalChecksum[CT], SHA256_DIGEST_LENGTH);
        delete[] globalChecksum[CT];
        return checksum;
    }

};

template <typename T, ChecksumType CT> std::mutex ChecksumTracker<T, CT>::checksumMutex[NUM_CHECKSUMS];
template <typename T, ChecksumType CT> uint8_t* ChecksumTracker<T, CT>::globalChecksum[NUM_CHECKSUMS];
template <typename T, ChecksumType CT> std::string ChecksumTracker<T, CT>::flag_fill[NUM_CHECKSUMS];
