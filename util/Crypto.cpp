//
// Created by tate on 7/31/25.
//

#include <iostream>

#include <gpgme.h>

// ssl_rsa_helpers.cpp
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/rsa.h>

#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

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
        return {};
    }

    // The following code is mostly from ChatGPT
    namespace SSL {
        // RAII helpers
        struct BioPtr {
            BIO* p;
            BioPtr(BIO* q = nullptr): p(q) { }
            ~BioPtr() { if (p) BIO_free_all(p); }
        };
        struct EvpMdCtxPtr {
            EVP_MD_CTX* p;
            EvpMdCtxPtr(EVP_MD_CTX* q = nullptr): p(q) { }
            ~EvpMdCtxPtr() { if (p) EVP_MD_CTX_free(p); }
        };

        // Utility: throw runtime_error with OpenSSL error queue
        static void throw_openssl_error(const char* msg) {
            unsigned long e = ERR_get_error();
            std::string err = msg ? msg : "OpenSSL error";
            if (e) {
                char buf[256];
                ERR_error_string_n(e, buf, sizeof(buf));
                err += ": ";
                err += buf;
            }
            throw std::runtime_error(err);
        }

        // Base64 encode/decode
        static std::string base64_encode(const unsigned char* data, size_t len) {
            BIO *b64 = BIO_new(BIO_f_base64());
            BIO *bmem = BIO_new(BIO_s_mem());
            if (!b64 || !bmem) { if(b64) BIO_free(b64); if(bmem) BIO_free(bmem); throw_openssl_error("BIO_new failed"); }
            // No newlines
            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
            BIO_push(b64, bmem);
            int written = BIO_write(b64, data, static_cast<int>(len));
            if (written <= 0) { BIO_free_all(b64); throw_openssl_error("BIO_write base64 failed"); }
            if (BIO_flush(b64) != 1) { BIO_free_all(b64); throw_openssl_error("BIO_flush base64 failed"); }

            BUF_MEM* bptr = nullptr;
            BIO_get_mem_ptr(b64, &bptr);
            std::string out;
            if (bptr && bptr->length > 0) out.assign(bptr->data, bptr->length);

            BIO_free_all(b64);
            return out;
        }

        static std::vector<unsigned char> base64_decode(const std::string& in) {
            BIO *b64 = BIO_new(BIO_f_base64());
            BIO *bmem = BIO_new_mem_buf(in.data(), static_cast<int>(in.size()));
            if (!b64 || !bmem) { if(b64) BIO_free(b64); if(bmem) BIO_free(bmem); throw_openssl_error("BIO_new failed"); }
            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
            BIO_push(b64, bmem);

            std::vector<unsigned char> out(in.size()); // upper bound
            int decoded = BIO_read(b64, out.data(), static_cast<int>(out.size()));
            if (decoded < 0) { BIO_free_all(b64); throw_openssl_error("BIO_read base64 failed"); }
            out.resize(decoded);
            BIO_free_all(b64);
            return out;
        }

        // Load public key from PEM string (returns owned EVP_PKEY*)
        static EVP_PKEY* load_public_key_from_pem(const std::string& pubkey_pem) {
            BioPtr bio(BIO_new_mem_buf(pubkey_pem.data(), static_cast<int>(pubkey_pem.size())));
            if (!bio.p) throw_openssl_error("BIO_new_mem_buf failed for public key");
            EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio.p, nullptr, nullptr, nullptr);
            if (!pkey) throw_openssl_error("PEM_read_bio_PUBKEY failed");
            return pkey;
        }

        // Load private key from PEM string (returns owned EVP_PKEY*)
        EVP_PKEY* load_private_key_from_pem(const std::string& privkey_pem) {
            BioPtr bio(BIO_new_mem_buf(privkey_pem.data(), static_cast<int>(privkey_pem.size())));
            if (!bio.p) throw_openssl_error("BIO_new_mem_buf failed for private key");
            EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio.p, nullptr, nullptr, nullptr);
            if (!pkey) throw_openssl_error("PEM_read_bio_PrivateKey failed");
            return pkey;
        }

        // Encrypt using RSA OAEP with public key (returns base64)
        std::string encrypt(EVP_PKEY* pub, const std::string& data) {
            if (data.empty() || pub == nullptr)
                return {};

            EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pub, nullptr);
            if (!ctx)
                throw_openssl_error("EVP_PKEY_CTX_new failed (encrypt)");

            if (EVP_PKEY_encrypt_init(ctx) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                throw_openssl_error("EVP_PKEY_encrypt_init failed");
            }
            // set RSA OAEP padding
            if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                throw_openssl_error("set_rsa_padding failed");
            }

            size_t outlen = 0;
            if (EVP_PKEY_encrypt(ctx, nullptr, &outlen, reinterpret_cast<const unsigned char*>(data.data()), data.size()) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                throw_openssl_error("EVP_PKEY_encrypt (determine size) failed");
            }

            std::vector<unsigned char> out(outlen);
            if (EVP_PKEY_encrypt(ctx, out.data(), &outlen, reinterpret_cast<const unsigned char*>(data.data()), data.size()) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                throw_openssl_error("EVP_PKEY_encrypt failed");
            }

            EVP_PKEY_CTX_free(ctx);
            return base64_encode(out.data(), outlen);
        }

        // Encrypt using RSA OAEP with public key (returns base64)
        std::string encrypt(const std::string& pubkey, const std::string& data) {
            const auto k = load_public_key_from_pem(pubkey);
            auto ret = encrypt(k, data);
            EVP_PKEY_free(k);
            return ret;
        }

        // Sign using RSA private key + SHA256, return base64 signature
        std::string sign(EVP_PKEY* privkey, const std::string& data) {
            EvpMdCtxPtr mdctx(EVP_MD_CTX_new());
            if (!mdctx.p)
                throw_openssl_error("EVP_MD_CTX_new failed");

            if (EVP_DigestSignInit(mdctx.p, nullptr, EVP_sha256(), nullptr, privkey) <= 0)
                throw_openssl_error("EVP_DigestSignInit failed");
            if (EVP_DigestSignUpdate(mdctx.p, data.data(), data.size()) <= 0)
                throw_openssl_error("EVP_DigestSignUpdate failed");

            size_t siglen = 0;
            if (EVP_DigestSignFinal(mdctx.p, nullptr, &siglen) <= 0)
                throw_openssl_error("EVP_DigestSignFinal (determine size) failed");
            std::vector<unsigned char> sig(siglen);
            if (EVP_DigestSignFinal(mdctx.p, sig.data(), &siglen) <= 0)
                throw_openssl_error("EVP_DigestSignFinal failed");
            sig.resize(siglen);
            return base64_encode(sig.data(), sig.size());
        }

        // Sign using RSA private key + SHA256, return base64 signature
        std::string sign(const std::string& privkey, const std::string& data) {
            auto* k = load_private_key_from_pem(privkey);
            auto ret = sign(k, data);
            EVP_PKEY_free(k);
            return ret;
        }

        // Decrypt base64 ciphertext using private key and OAEP padding. Returns plaintext string.
        std::string decrypt(EVP_PKEY* privkey, const std::string& b64data) {
            std::vector<unsigned char> ciphertext = base64_decode(b64data);
            if (ciphertext.empty()) return std::string();

            EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privkey, nullptr);
            if (!ctx)
                throw_openssl_error("EVP_PKEY_CTX_new failed (decrypt)");

            if (EVP_PKEY_decrypt_init(ctx) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                throw_openssl_error("EVP_PKEY_decrypt_init failed");
            }
            if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                throw_openssl_error("set_rsa_padding failed (decrypt)");
            }

            size_t outlen = 0;
            if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, ciphertext.data(), ciphertext.size()) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                throw_openssl_error("EVP_PKEY_decrypt (determine size) failed");
            }

            std::vector<unsigned char> out(outlen);
            if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, ciphertext.data(), ciphertext.size()) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                throw_openssl_error("EVP_PKEY_decrypt failed");
            }

            EVP_PKEY_CTX_free(ctx);
            return std::string(reinterpret_cast<char*>(out.data()), outlen);
        }

        // Decrypt base64 ciphertext using private key and OAEP padding. Returns plaintext string.
        std::string decrypt(const std::string& privkey, const std::string& b64data) {
            auto* k = load_private_key_from_pem(privkey);
            auto ret = decrypt(k, b64data);
            EVP_PKEY_free(k);
            return ret;
        }

        // Verify RSA+SHA256 signature (base64) using public key.
        // Returns true if valid, false if invalid.
        bool verify(EVP_PKEY* pubkey, const std::string& data, const std::string& b64sig) {
            std::vector<unsigned char> sig = base64_decode(b64sig);
            EvpMdCtxPtr mdctx(EVP_MD_CTX_new());
            if (!mdctx.p)
                throw_openssl_error("EVP_MD_CTX_new failed (verify)");

            if (EVP_DigestVerifyInit(mdctx.p, nullptr, EVP_sha256(), nullptr, pubkey) <= 0)
                throw_openssl_error("EVP_DigestVerifyInit failed");
            if (EVP_DigestVerifyUpdate(mdctx.p, data.data(), data.size()) <= 0)
                throw_openssl_error("EVP_DigestVerifyUpdate failed");

            int rc = EVP_DigestVerifyFinal(mdctx.p, sig.data(), sig.size());
            if (rc == 1)
                return true;  // signature valid
            if (rc != 0)
                throw_openssl_error("EVP_DigestVerifyFinal failed");
            return false;
        }

        // Verify RSA+SHA256 signature (base64) using public key.
        // Returns true if valid, false if invalid.
        bool verify(const std::string& pubkey, const std::string& data, const std::string& b64sig) {
            auto* k = load_public_key_from_pem(pubkey);
            bool ret = verify(k, data, b64sig);
            EVP_PKEY_free(k);
            return ret;
        }
    } // namespace SSL
} // namespace Crypto
