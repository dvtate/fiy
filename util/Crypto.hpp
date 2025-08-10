//
// Created by tate on 7/31/25.
//

#pragma once

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <random>
#include <string>
#include <string_view>

#include "base64.hpp"

struct Crypto {
    // TODO instead use one a more performant library
    static std::string b64enc(const unsigned char* buffer, size_t length) {
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO* bio_mem = BIO_new(BIO_s_mem());
        BIO_push(b64, bio_mem);
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);  // No newlines

        BIO_write(b64, buffer, length);
        BIO_flush(b64);

        BUF_MEM* bufferPtr;
        BIO_get_mem_ptr(bio_mem, &bufferPtr);
        std::string encoded(bufferPtr->data, bufferPtr->length);

        BIO_free_all(b64);
        return encoded;
    }

    /// Cryptographically hash
    static std::string hmac(const std::string_view key, const std::string_view message,
                            const evp_md_st* evp_md = EVP_sha512_256()) {
        unsigned char result[EVP_MAX_MD_SIZE];
        unsigned int len = 0;
        unsigned char* ret = HMAC(evp_md, key.data(), key.size(), (unsigned char*)message.data(),
                                  message.size(), result, &len);
        if (ret == nullptr) {
            return "";
        }

        // TODO leverage known output size + simd
        return b64enc(result, len);
    }

    /// Get a random device
    static std::mt19937& rng();

    template <std::size_t TOKEN_LEN>
    static std::string get_token_string() {
        std::uniform_int_distribution<unsigned short> dist(1, 63);
        const char charset[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

        // Generate token using random chars from charset
        // Probably a better way to do this that doesn't use operator+=
        std::string ret;
        ret.reserve(TOKEN_LEN);
        ret += '.';  // start with '.' to differentiate from user tokens
        for (int i = 1; i < TOKEN_LEN; i++)
            ret += charset[dist(Crypto::rng())];
        return ret;
    }
};
