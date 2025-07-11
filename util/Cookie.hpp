//
// Created by tate on 7/8/25.
//

#pragma once

//
// Created by tate on 7/2/25.
//

#ifndef COOKIE_COOKIES_HPP
#define COOKIE_COOKIES_HPP
#include <string>
#include <string_view>
#include <cinttypes>
#include <regex>
#include <vector>
#include <cctype>
#include <iostream>

namespace Cookie {

    /**
     * Equivalent to decodeUriComponent in JS
     * @param begin
     * @param end
     * @return
     */
    std::string uri_decode(const char* begin, const char* end);

    /**
     * Equivalent to encodeUriComponent in JS
     * @param src
     * @return
     */
    std::string uri_encode(const std::string& src);

    /**
     * Parse from cookie header
     * @param value cookie header value
     */
    std::map<std::string, std::string> parse(std::string_view header) {
        std::string::size_type pos;
        std::map<std::string, std::string> ret;
        while ((pos = header.find(';')) != std::string::npos) {
            std::string_view coo = header.substr(0, pos);
            auto epos = coo.find('=');
            if (epos != std::string::npos) {
                // Get cookie name
                std::string::size_type start = 0;
                std::string::size_type end = epos;
                while (coo[start] == 0x20 || coo[start] == 0x09)
                    start++;
                while (coo[end] == 0x20 || coo[end] == 0x09)
                    end--;
                auto cookie_name = std::string(coo.substr(start, end - start));

                // Get cookie value
                start = epos + 1;
                end = coo.size() - 1;
                while (coo[start] == 0x20 || coo[start] == 0x09)
                    start++;
                while (coo[end] == 0x20 || coo[end] == 0x09)
                    end--;
                auto cookie_value = std::string(coo.substr(start, end));

                // Set cookie
                ret[std::move(cookie_name)] = std::move(cookie_value);
            }
            header.remove_prefix(pos + 1);
        }
        return ret;
    }

    struct CookieOptions {
        /**
         * Custom serialization function for cookie-value's
         * defaults to using our encodeUriComponent equivalent
         * set to nullptr to skip
         */
        std::string (*encode_fn)(const std::string&) {uri_encode};

        /**
         * Specifies the `number` (in seconds) to be the value for the [`Max-Age` `Set-Cookie` attribute](https://tools.ietf.org/html/rfc6265#section-5.2.2).
         *
         * if value < 0, don't include this attribute
         * value of 0 => earliest possible expiration
         * otherwise => number of seconds before expiry
         *
         * note: should take precedence over `expires`
         */
        ssize_t max_age{-1};

        /**
         * Specifies the `Date` object to be the value for the [`Expires` `Set-Cookie` attribute](https://tools.ietf.org/html/rfc6265#section-5.2.1).
         * When no expiration is set clients consider this a "non-persistent cookie" and delete it when the current session is over.
         *
         * The [cookie storage model specification](https://tools.ietf.org/html/rfc6265#section-5.3) states that if both `expires` and
         * `maxAge` are set, then `maxAge` takes precedence, but it is possible not all clients by obey this,
         * so if both are set, they should point to the same date and time.
         */
        std::string expires;

        /**
         * Specifies the value for the [`Domain` `Set-Cookie` attribute](https://tools.ietf.org/html/rfc6265#section-5.2.3).
         * When no domain is set clients consider the cookie to apply to the current domain only.
         */
        std::string domain;

        /**
         * Specifies the value for the [`Path` `Set-Cookie` attribute](https://tools.ietf.org/html/rfc6265#section-5.2.4).
         * When no path is set, the path is considered the ["default path"](https://tools.ietf.org/html/rfc6265#section-5.1.4).
         */
        std::string path;


        /**
         * Enables the [`HttpOnly` `Set-Cookie` attribute](https://tools.ietf.org/html/rfc6265#section-5.2.6).
         * When enabled, clients will not allow client-side JavaScript to see the cookie in `document.cookie`.
         */
        bool http_only{false};

        /**
         * Enables the [`Secure` `Set-Cookie` attribute](https://tools.ietf.org/html/rfc6265#section-5.2.5).
         * When enabled, clients will only send the cookie back if the browser has a HTTPS connection.
         */
        bool secure{false};

        /**
         * Enables the [`Partitioned` `Set-Cookie` attribute](https://tools.ietf.org/html/draft-cutler-httpbis-partitioned-cookies/).
         * When enabled, clients will only send the cookie back when the current domain _and_ top-level domain matches.
         *
         * This is an attribute that has not yet been fully standardized, and may change in the future.
         * This also means clients may ignore this attribute until they understand it. More information
         * about can be found in [the proposal](https://github.com/privacycg/CHIPS).
         */
        bool partitioned{false};

        /**
         * Specifies the value for the [`Priority` `Set-Cookie` attribute](https://tools.ietf.org/html/draft-west-cookie-priority-00#section-4.1).
         *
         * - `'low'` will set the `Priority` attribute to `Low`.
         * - `'medium'` will set the `Priority` attribute to `Medium`, the default priority when not set.
         * - `'high'` will set the `Priority` attribute to `High`.
         *
         * More information about priority levels can be found in [the specification](https://tools.ietf.org/html/draft-west-cookie-priority-00#section-4.1).
         */
        std::string priority;

        /**
         * Specifies the value for the [`SameSite` `Set-Cookie` attribute](https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-09#section-5.4.7).
         *
         * - `true` will set the `SameSite` attribute to `Strict` for strict same site enforcement.
         * - `'lax'` will set the `SameSite` attribute to `Lax` for lax same site enforcement.
         * - `'none'` will set the `SameSite` attribute to `None` for an explicit cross-site cookie.
         * - `'strict'` will set the `SameSite` attribute to `Strict` for strict same site enforcement.
         *
         * More information about enforcement levels can be found in [the specification](https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-09#section-5.4.7).
         */
        std::string same_site;
    };

    /**
     * Make a set-cookie string for given cookie
     * @param name
     * @param value
     * @return
     */
    std::string serialize(const std::string& name, std::string value);
    std::string serialize(const std::string& name, std::string value, CookieOptions options);

};

#endif //COOKIE_COOKIES_HPP
