//
// Created by tate on 7/31/25.
//

#include <iostream>

#include <gpgme.h>

#include "Crypto.hpp"

#define DEBUG_LOG(X) std::cerr <<X <<std::endl;

namespace Crypto {
    std::mt19937& rng(void) {
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        return gen;
    }

    std::string gpg_encrypt_text(const std::string& pubkey, const std::string& data) {
        gpgme_data_t key_data;
        gpgme_key_t keys[2] = { nullptr, nullptr };
        char* cipher_buffer;
        std::string encryptedText;

        gpgme_ctx_t ctx;
        gpgme_error_t err;
        err = gpgme_new(&ctx);
        if (err)
            goto handle_gpgme_error;

        // Set ASCII armor so we get base64 output
        gpgme_set_armor(ctx, 1);

        // Import the public key
        err = gpgme_data_new_from_mem(&key_data, pubkey.c_str(), pubkey.size(), 0)
              || gpgme_op_import(ctx, key_data);
        if (err)
            goto handle_gpgme_error;
        gpgme_data_release(key_data);

        // Find the key we just imported
        err = gpgme_op_keylist_start(ctx, nullptr, 0);
        if (err)
            goto handle_gpgme_error;
        if (gpgme_op_keylist_next(ctx, &keys[0]) != GPG_ERR_NO_ERROR) {
            DEBUG_LOG("gpgme err: Public key not found");
            return "";
        }

        // Encrypt the message
        gpgme_data_t plain, cipher;
        err = gpgme_data_new_from_mem(&plain, data.c_str(), data.size(), 0)
              || gpgme_data_new(&cipher);
        if (err)
            goto handle_gpgme_error;

        err = gpgme_op_encrypt(ctx, keys, GPGME_ENCRYPT_ALWAYS_TRUST, plain, cipher);
        if (err)
            goto handle_gpgme_error;

        // Get the encrypted result
        cipher_buffer = gpgme_data_release_and_get_mem(cipher, nullptr);
        encryptedText = cipher_buffer;
        free(cipher_buffer);

        gpgme_key_unref(keys[0]);
        gpgme_release(ctx);

        return encryptedText;

handle_gpgme_error:
        DEBUG_LOG("gpgme error: " <<gpgme_strerror(err));
        return "";
    }

    std::string gpg_sign(const std::string& privkey, const std::string& data) {
        // TODO
        return "";
    }
}