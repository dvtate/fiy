//
// Created by tate on 7/22/25.
//

#include <filesystem>

#include <openssl/x509_vfy.h>
#include <openssl/ssl.h>

#include "App.hpp"

#include "HttpsClient.hpp"


boost::asio::io_context* HttpsClient::get_io_context() {
    return g_app->m_ioc;
}

void HttpsClient::prep_ssl() {
    // If this doesn't work we could always manually load certs from
    //  /etc/ssl/cert.pem or /etc/pki/tls/cert.pem with
    //  m_ssl.add_certificate_authority(asio::buffer(cert.data(), cert.size()), ec);

    // Stolen from https://github.com/djarek/certify
    // Not supporting windows or macos keystores
    ::SSL_CTX_set_verify(
        m_ssl.native_handle(),
        ::SSL_CTX_get_verify_mode(m_ssl.native_handle()),
        [](int preverified, auto*) -> int { return preverified == 1; }
    );

    // Verify the remote server's certificate
    m_ssl.set_verify_mode(boost::asio::ssl::verify_peer);
}