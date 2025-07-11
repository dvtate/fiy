//
// Created by tate on 7/8/25.
//

#include "Cookie.hpp"


/**
 * RegExp to match cookie-name in RFC 6265 sec 4.1.1
 * This refers out to the obsoleted definition of token in RFC 2616 sec 2.2
 * which has been replaced by the token definition in RFC 7230 appendix B.
 *
 * cookie-name       = token
 * token             = 1*tchar
 * tchar             = "!" / "#" / "$" / "%" / "&" / "'" /
 *                     "*" / "+" / "-" / "." / "^" / "_" /
 *                     "`" / "|" / "~" / DIGIT / ALPHA
 *
 * Note: Allowing more characters - https://github.com/jshttp/cookie/issues/191
 * Allow same range as cookie value, except `=`, which delimits end of name.
 */
static const std::regex cookie_name_regex{"^[\u0021-\u003A\u003C\u003E-\u007E]+$"};

/**
 * RegExp to match cookie-value in RFC 6265 sec 4.1.1
 *
 * cookie-value      = *cookie-octet / ( DQUOTE *cookie-octet DQUOTE )
 * cookie-octet      = %x21 / %x23-2B / %x2D-3A / %x3C-5B / %x5D-7E
 *                     ; US-ASCII characters excluding CTLs,
 *                     ; whitespace DQUOTE, comma, semicolon,
 *                     ; and backslash
 *
 * Allowing more characters: https://github.com/jshttp/cookie/issues/191
 * Comma, backslash, and DQUOTE are not part of the parsing algorithm.
 */
static const std::regex cookie_value_regex{"^[\u0021-\u003A\u003C-\u007E]*$"};

/**
 * RegExp to match domain-value in RFC 6265 sec 4.1.1
 *
 * domain-value      = <subdomain>
 *                     ; defined in [RFC1034], Section 3.5, as
 *                     ; enhanced by [RFC1123], Section 2.1
 * <subdomain>       = <label> | <subdomain> "." <label>
 * <label>           = <let-dig> [ [ <ldh-str> ] <let-dig> ]
 *                     Labels must be 63 characters or less.
 *                     'let-dig' not 'letter' in the first char, per RFC1123
 * <ldh-str>         = <let-dig-hyp> | <let-dig-hyp> <ldh-str>
 * <let-dig-hyp>     = <let-dig> | "-"
 * <let-dig>         = <letter> | <digit>
 * <letter>          = any one of the 52 alphabetic characters A through Z in
 *                     upper case and a through z in lower case
 * <digit>           = any one of the ten digits 0 through 9
 *
 * Keep support for leading dot: https://github.com/jshttp/cookie/issues/173
 *
 * > (Note that a leading %x2E ("."), if present, is ignored even though that
 * character is not permitted, but a trailing %x2E ("."), if present, will
 * cause the user agent to ignore the attribute.)
 */
static const std::regex domain_value_regex{
        "^([.]?[a-z0-9]([a-z0-9-]{0,61}[a-z0-9])?)([.][a-z0-9]([a-z0-9-]{0,61}[a-z0-9])?)*$",
        std::regex_constants::icase
};

/**
 * RegExp to match path-value in RFC 6265 sec 4.1.1
 *
 * path-value        = <any CHAR except CTLs or ";">
 * CHAR              = %x01-7F
 *                     ; defined in RFC 5234 appendix B.1
 */
static const std::regex path_value_regex{"^[\u0020-\u003A\u003D-\u007E]*$"};

static inline std::string charToHex(char c) {
    char first = (c & 0xF0) >> 4;
    first += first > 9 ? 'A' - 10 : '0';
    char second = c & 0x0F;
    second += second > 9 ? 'A' - 10 : '0';

    std::string result;
    result += first;
    result += second;
    return result;
}

namespace Cookie {

    /**
     * Equivalent to decodeUriComponent in JS
     * @param begin
     * @param end
     * @return
     */
    std::string uri_decode(const char* begin, const char* end) {
        std::string result;
        size_t len = end - begin;
        result.reserve(len * 2);
        int hex = 0;
        for (size_t i = 0; i < len; ++i) {
            switch (begin[i]) {
                case '+':
                    result += ' ';
                    break;
                case '%':
                    if ((i + 2) < len
                        && std::isxdigit(begin[i + 1])
                        && std::isxdigit(begin[i + 2])
                            ) {
                        unsigned int x1 = begin[i + 1];
                        if (x1 >= '0' && x1 <= '9')
                            x1 -= '0';
                        else if (x1 >= 'a' && x1 <= 'f')
                            x1 = x1 - 'a' + 10;
                        else if (x1 >= 'A' && x1 <= 'F')
                            x1 = x1 - 'A' + 10;
                        unsigned int x2 = begin[i + 2];
                        if (x2 >= '0' && x2 <= '9')
                            x2 -= '0';
                        else if (x2 >= 'a' && x2 <= 'f')
                            x2 = x2 - 'a' + 10;
                        else if (x2 >= 'A' && x2 <= 'F')
                            x2 = x2 - 'A' + 10;
                        hex = x1 * 16 + x2;

                        result += char(hex);
                        i += 2;
                    } else {
                        result += '%';
                    }
                    break;
                default:
                    result += begin[i];
                    break;
            }
        }
        return result;
    }

    /**
     * Equivalent to encodeUriComponent in JS
     * @param src
     * @return
     */
    std::string uri_encode(const std::string& src) {
        std::string result;
        std::string::const_iterator iter;

        for (iter = src.begin(); iter != src.end(); ++iter) {
            switch (*iter) {
                case ' ':
                    result.append(1, '+');
                    break;

                    // alnum
                case 'A':
                case 'B':
                case 'C':
                case 'D':
                case 'E':
                case 'F':
                case 'G':
                case 'H':
                case 'I':
                case 'J':
                case 'K':
                case 'L':
                case 'M':
                case 'N':
                case 'O':
                case 'P':
                case 'Q':
                case 'R':
                case 'S':
                case 'T':
                case 'U':
                case 'V':
                case 'W':
                case 'X':
                case 'Y':
                case 'Z':
                case 'a':
                case 'b':
                case 'c':
                case 'd':
                case 'e':
                case 'f':
                case 'g':
                case 'h':
                case 'i':
                case 'j':
                case 'k':
                case 'l':
                case 'm':
                case 'n':
                case 'o':
                case 'p':
                case 'q':
                case 'r':
                case 's':
                case 't':
                case 'u':
                case 'v':
                case 'w':
                case 'x':
                case 'y':
                case 'z':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':

                    // mark
                case '-':
                case '_':
                case '.':
                case '!':
                case '~':
                case '*':
                case '(':
                case ')':
                    result.append(1, *iter);
                    break;

                    // escape
                default:
                    result.append(1, '%');
                    result.append(charToHex(*iter));
                    break;
            }
        }

        return result;
    }


    std::string serialize(const std::string& name, std::string value) {
        if (!std::regex_match(name, cookie_name_regex)) {
            std::cerr <<"Invalid cookie name\n";
            return "";
        }

        value = uri_encode(value);
        if (!std::regex_match(value, cookie_value_regex)) {
            std::cerr <<"Invalid cookie value\n";
        }

        std::string str = name + "=" + value;
        return str;
    }

    std::string serialize(const std::string& name, std::string value, CookieOptions options) {
        if (!std::regex_match(name, cookie_name_regex)) {
            std::cerr <<"Invalid cookie name\n";
            return "";
        }

        if (options.encode_fn)
            value = options.encode_fn(value);
        if (!std::regex_match(value, cookie_value_regex)) {
            std::cerr <<"Invalid cookie value\n";
            return "";
        }

        std::string str = name + "=" + value;

        if (options.max_age >= 0)
            str += "; Max-Age=" + std::to_string(options.max_age);
        if (!options.domain.empty()) {
            if (!std::regex_match(options.domain, domain_value_regex)) {
                std::cerr <<"Invalid cookie domain option: " <<options.domain <<std::endl;
            } else {
                str += "; Domain=" + options.domain;
            }
        }

        if (!options.path.empty()) {
            if (!std::regex_match(options.path, path_value_regex)) {
                std::cerr <<"Invalid cookie path option: " <<options.path <<std::endl;
            } else {
                str += "; Path=" + options.path;
            }
        }

        if (!options.expires.empty()) {
            // TODO make sure it's a valid UTC date string
            str += "; Expires=" + options.expires;
        }

        if (options.http_only)
            str += "; HttpOnly";

        if (options.secure)
            str += "; Secure";

        if (options.partitioned)
            str += "; Partitioned";

        if (!options.priority.empty()) {
            switch (options.priority[0]) {
                case 'l': case 'L':
                    str += "; Priority=Low";
                    break;
                case 'm': case 'M':
                    str += "; Priority=Medium";
                    break;
                case 'h': case 'H':
                    str += "; Priority=High";
                    break;
                default:
                    std::cerr <<"Invalid cookie priority option: " <<options.priority <<std::endl;
            }
        }

        if (!options.same_site.empty()) {
            switch (options.same_site[0]) {
                case 't': case 's': case 'S':
                    str += "; SameSite=Strict";
                    break;
                case 'l': case 'L':
                    str += "; SameSite=Lax";
                    break;
                case 'n': case 'N':
                    str += "; SameSite=None";
                    break;
                default:
                    std::cerr <<"Invalid cookie SameSite option: " <<options.same_site <<std::endl;
            }
        }

        return str;
    }

}