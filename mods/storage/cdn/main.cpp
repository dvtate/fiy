//
// Created by tate on 3/6/26.
//

#include <string>
#include <unordered_set>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <concepts>
#include <deque>

#include <stdio.h>
#include <errno.h>
#include <fstream>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/url.hpp>

#include <curl/curl.h>

#include <SQLiteCpp/SQLiteCpp.h>

#define HEX_DIGITS "0123456789ABCDEF"

SQLite::Statement operator ""_sql(const char* str, std::size_t);

// Globals
namespace Config {

    std::string db_path;

    std::string data_dir;

    // Authentication token
    // TODO instead client should use this to generate temporary access tokens
    std::string bearer_token;

    ssize_t max_storage;
    bool is_full;

    std::string listen_addr = "0.0.0.0";

    constexpr unsigned short default_port = 6636;
    unsigned short listen_port = default_port;

    unsigned threads = std::thread::hardware_concurrency();

    bool from_env() {
        char* v;
        v = std::getenv("DB_PATH");
        if (v == nullptr) {
            std::cerr << "Config: DB_PATH should be set to path to sqlite database.\n";
            return false;
        }
        db_path = v;

        v = std::getenv("DATA_DIR");
        if (v == nullptr) {
            std::cerr << "Config: DATA_DIR should be set to path where assets are stored.\n";
            return false;
        }
        data_dir = v;

        v = std::getenv("BEARER_TOKEN");
        if (v == nullptr) {
            std::cerr <<"Config: BEARER_TOKEN should be set to bearer token used in internal API calls\n";
            return false;
        }
        bearer_token = v;

        v = std::getenv("MAX_STORAGE");
        if (v == nullptr) {
            std::cerr <<"Config: MAX_STORAGE should be set to max storage to use for assets\n";
            return false;
        }
        char* end;
        max_storage = strtoll(v, &end, 10);
        if (v == end) {
            std::cerr <<"Config: failed to parse MAX_STORAGE\n";
            max_storage = -1;
            return false;
        }

        v = std::getenv("ADDRESS");
        listen_addr = v != nullptr ? v : "0.0.0.0";

        v = std::getenv("PORT");
        if (v == nullptr) {
            listen_port = default_port;
            std::cerr <<"Config: PORT not set, defaulting to " <<default_port <<std::endl;
        } else {
            listen_port = strtoll(v, &end, 10);
            if (v == end) {
                std::cerr <<"Config: failed to parse PORT\n";
                listen_port = default_port;
                return false;
            }
        }

        v = std::getenv("THREADS");
        if (v != nullptr) {
            threads = strtoll(v, &end, 10);
            if (v == end) {
                std::cerr <<"Config: failed to parse THREADS, should be a number\n";
                listen_port = default_port;
                return false;
            }
            if (threads <= 0)
                threads += std::thread::hardware_concurrency();
        }
        return true;
    }

    bool prep_data_dir() {
        static constexpr std::string_view digits = HEX_DIGITS;
        std::string dir = data_dir + "/XX";

        for (const char c1 : digits)
            for (const char c2 : digits) {
                dir[dir.size() - 1] = c1;
                dir[dir.size() - 2] = c2;
                if (std::filesystem::exists(dir))
                    continue;
                std::error_code ec;
                if (!std::filesystem::create_directory(dir, ec)) {
                    std::cerr <<"prep_data_dir(): failed to make data subdir: " << dir <<": " <<ec.message() << std::endl;
                    return false;
                }
            }

        return true;
    }

    bool prep_db() {
        // DB tables
        R"(CREATE TABLE IF NOT EXISTS Assets (
            id INTEGER PRIMARY KEY,
            bucket INTEGER NOT NULL,
            mimeType TEXT DEFAULT NULL,
            cacheControl TEXT DEFAULT NULL,
            createTs INTEGER DEFAULT 0,
            deleteTs INTEGER DEFAULT -1
        ))"_sql.exec();

        R"(CREATE TABLE IF NOT EXISTS Tokens (
            token INTEGER UNIQUE,
            assetId INTEGER REFERENCES Assets,
            expireTs INTEGER DEFAULT -1,
        ))"_sql.exec();
    }

    void check_storage() {
        // TODO check disk space utilized vs limit
    }
}


/// SQLite statement
inline SQLite::Statement operator ""_sql(const char* str, std::size_t) {
    thread_local SQLite::Database db{
        Config::db_path,
        SQLite::OPEN_READWRITE
    };
    return SQLite::Statement{ db, str };
}

/// Convert int to reverse hex representation
template <std::integral T>
constexpr std::size_t max_hex_chars =
    (std::numeric_limits<std::make_unsigned_t<T>>::digits + 3) / 4;
template <std::integral IntT, size_t N>
requires (N >= max_hex_chars<IntT> + 1)
void to_rhex(IntT n, char (&ret)[N]) {

    auto v = static_cast<std::make_unsigned_t<IntT>>(n);
    int i = 0;
    do {
        ret[i++] = HEX_DIGITS[v & 0xF]; // % 16
        v >>= 4; // /= 16
    } while (v != 0);
    ret[i] = '\0';
    // Note that ret is the reverse hex representation

    // reverse digits
    // for (std::size_t j = 0; j < i / 2; ++j)
    //     std::swap(buffer[j], buffer[i - j - 1]);
}

/// Opposite of to_rhex
int64_t from_rhex(const char* s, uint8_t n) {
    uint64_t ret = 0;
    uint8_t i = 0;
    while (i < n && *s) {
        switch (s[i++]) {
        case '0': break;
        case '1': ret += 1; break;
        case '2': ret += 2; break;
        case '3': ret += 3; break;
        case '4': ret += 4; break;
        case '5': ret += 5; break;
        case '6': ret += 6; break;
        case '7': ret += 7; break;
        case '8': ret += 8; break;
        case '9': ret += 9; break;
        case 'a': case 'A': ret += 0xA; break;
        case 'b': case 'B': ret += 0xB; break;
        case 'c': case 'C': ret += 0xC; break;
        case 'd': case 'D': ret += 0xD; break;
        default:
            std::cerr <<"from_rhex(): invalid character " <<(*s) << " in '" <<std::string_view(s, n) <<"'\n";
            continue;
        }
        ret <<= 4; // *= 16;
    }
    return ret;
}

/// Where in the FS is the data for this asset stored?
std::string get_asset_path(const uint8_t bucket, const uint32_t asset_id) {
    static constexpr char hex_digits[] = HEX_DIGITS;
    char bucket_str[2];
    bucket_str[0] = hex_digits[bucket & 0xF];
    bucket_str[1] = hex_digits[(bucket >> 4) & 0xF];

    std::string ret = Config::data_dir + "/";
    ret += std::string_view(bucket_str, 2);
    ret += "/";

    char asset_id_str[65];
    to_rhex(asset_id, asset_id_str);
    ret += asset_id_str;
    return ret;
}

/// Put a new asset into the database
std::pair<uint32_t, uint8_t> new_asset(
    const std::string& mime_type = "",
    int expiration_seconds = -1,
    const std::string& cache_control = ""
) {
    // Start with a random bucket id
    static std::atomic_int bucket_counter = rand();

    // Pick a bucket with roughly equal distribution
    const uint8_t bucket = (++bucket_counter) & 0xFF;

    // Make bucket in db
    thread_local auto q = "INSERT INTO Assets (bucket, mime_type, cacheControl, deleteTs, createTs) VALUES (?, ?, ?, ?, ?) RETURNING id"_sql;
    q.bind(1, bucket);

    if (mime_type.empty())
        q.bind(2);
    else
        q.bindNoCopy(2, mime_type);
    if (cache_control.empty())
        q.bind(3);
    else
        q.bindNoCopy(3, cache_control);
    q.bind(4, expiration_seconds >= 0
        ? expiration_seconds + time(nullptr)
        : expiration_seconds);
    q.bind(5, time(nullptr));
    if (!q.executeStep()) {
        std::cerr <<"failed to add new asset to db.\n";
        return {-1, 0};
    }
    const auto id = q.getColumn(0).getUInt();
    return {id, bucket};
}


int64_t grant_access_token(const uint32_t asset_id, const time_t expiration_seconds, int64_t token) {
    const uint64_t now = time(nullptr);
    time_t expire_ts = expiration_seconds;
    if (expire_ts >= 0)
        expire_ts += now;

    thread_local auto q = "INSERT INTO Tokens (token, assetId, expireTs) VALUES (?, ?, ?)"_sql;
    try_new_token:
    q.bind(1, token);
    q.bind(2, asset_id);
    q.bind(3, expire_ts);
    try {
        if (q.exec() == 0)
            goto try_new_token;
    } catch (SQLite::Exception& e) {
        std::cerr <<"New token error: " <<e.what() <<std::endl;
        return -1;
    }
    return token;
}

/// Generate a new access token
int64_t new_access_token(const uint32_t asset_id, const time_t expiration_seconds) {
    // TODO instead implement by calling grant_access_token()
    const uint64_t now = time(nullptr);
    time_t expire_ts = expiration_seconds;
    if (expire_ts >= 0)
        expire_ts += now;

    thread_local auto q = "INSERT INTO Tokens (token, assetId, expireTs) VALUES (?, ?, ?)"_sql;
try_new_token:
    const int64_t token = (now << 32) + rand();
    q.bind(1, token);
    q.bind(2, asset_id);
    q.bind(3, expire_ts);
    try {
        if (q.exec() == 0)
            goto try_new_token;
    } catch (SQLite::Exception& e) {
        std::cerr <<"New token error: " <<e.what() <<std::endl;
        return -1;
    }
    return token;
}

void delete_asset(const uint32_t asset_id) {
    thread_local auto q = "DELETE FROM Tokens WHERE assetId=?"_sql;
    q.bind(1, asset_id);
    q.exec();
    q.reset();
    thread_local auto q2 = "DELETE FROM Assets WHERE id=?"_sql;
    q2.bind(1, asset_id);
    q2.exec();
    q2.reset();
}
void delete_asset(const std::string_view asset_id) {
    delete_asset(from_rhex(asset_id.data(), asset_id.size()));
}

void delete_token(const int64_t token) {
    thread_local auto q = "DELETE FROM Tokens WHERE token=?"_sql;
    q.bind(1, token);
    q.exec();
}
void delete_token(const std::string_view token) {
    delete_token(from_rhex(token.data(), token.size()));
}

/// delete no longer needed files and db entries
void purge() {
    std::deque<std::pair<uint32_t, uint32_t>> assets_to_delete;
    try {
        // Start transaction
        thread_local auto t_start = "BEGIN TRANSACTION"_sql;
        t_start.exec();
        t_start.reset();

        // Delete expired tokens or tokens associated with deleted assets
        thread_local auto purge_tokens = "DELETE FROM Tokens"
            " WHERE (expireTs < ? AND expireTs != -1) "
            " OR assetId IN (SELECT id FROM Assets"
                " WHERE deleteTs < ? AND deleteTs != -1 AND createTs < ?)"_sql;
        const time_t now = time(nullptr);
        constexpr time_t creation_delay = 60 * 5; // 5 mins
        purge_tokens.bind(1, now);
        purge_tokens.bind(2, now);
        purge_tokens.bind(3, now - creation_delay);

        // Find assets to delete
        thread_local auto q_unused_assets = "SELECT bucket, id FROM Assets"
            " WHERE createTs < ? AND id NOT IN (SELECT assetId FROM Tokens)"_sql;
        q_unused_assets.bind(1, now - creation_delay);
        while (q_unused_assets.executeStep()) {
            assets_to_delete.emplace_back(
                q_unused_assets.getColumn(0).getUInt(),
                q_unused_assets.getColumn(1).getUInt()
            );
        }
        q_unused_assets.reset();

        // Remove assets from db
        thread_local auto purge_assets = "DELETE FROM Assets"
            " WHERE createTs < ? AND id NOT IN (SELECT assetId FROM Tokens)"_sql;
        purge_assets.bind(1, now - creation_delay);
        purge_assets.exec();
        purge_assets.reset();
    } catch (SQLite::Exception& e) {
        std::cerr <<"purge(): SQL: " << e.what() <<std::endl;
        thread_local auto t_rb = "ROLLBACK"_sql;
        t_rb.exec();
    }

    // End Transaction
    thread_local auto t_commit = "COMMIT"_sql;
    t_commit.exec();
    t_commit.reset();

    // Delete purged files
    for (const auto& [ bucket, asset_id ] : assets_to_delete) {
        std::error_code ec;
        if (!std::filesystem::remove(get_asset_path(bucket, asset_id), ec))
            std::cout <<"purge(): Failed to delete unused asset "
                <<get_asset_path(bucket, asset_id) <<": " <<ec.message() << std::endl;
    }
}

bool download_file_curl(const std::string& url, const std::string& file_path) {
    CURL* curl = curl_easy_init();
    if (curl == nullptr)
        return false;
    FILE* fp = fopen(file_path.c_str(), "wb");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    const CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);
    return res == CURLE_OK;
}

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

static const auto e404 = []() {
    http::response<http::empty_body> res;
    res.result(404);
    res.keep_alive(false);
    return res;
};

http::message_generator e500(const std::string& err = "please try again later") {
    http::response<http::string_body> res;
    res.result(500);
    res.set(http::field::content_type, "text/html");
    res.body() = "Server Error: " + err;
    res.keep_alive(false);
    res.prepare_payload();
    return res;
}

http::message_generator file_resp(const std::string_view path, bool keep_alive) {
    // Parse path
    //  /<token>/<asset_id>[/file.ext][#?...]
    if (path.empty() || path[0] != '/')
        return e404();
    const auto slash = path.find('/', 1);
    if (slash == std::string_view::npos || slash + 1 > path.size())
        return e404();
    const auto token = from_rhex(path.data() + 1, slash - 1);
    auto slash2 = path.find_first_not_of(HEX_DIGITS, slash + 1);
    const auto asset_id = from_rhex(
        path.data() + slash + 1,
        slash2 == std::string_view::npos
            ? path.size() - slash - 1
            : slash2 - slash - 1
    );

    // Get file path from db
    thread_local auto q = "SELECT bucket, mimeType, cacheControl FROM Assets WHERE id=? AND EXISTS ("
        "SELECT 1 FROM Tokens WHERE assetId=? AND token=?)"_sql;
    q.bind(1, asset_id);
    q.bind(2, asset_id);
    q.bind(3, token);
    if (!q.executeStep()) {
        q.reset();
        return e404();
    }
    const auto bucket = q.getColumn(0).getUInt();
    const auto mime_type = q.getColumn(1).getString();
    const auto cache_control = q.getColumn(2).getString();
    const auto file_path = get_asset_path(bucket, asset_id);

    // Prepare response
    http::response<http::file_body> res;
    boost::beast::error_code ec;
    res.body().open(
        file_path.c_str(),
        beast::file_mode::scan,
        ec
    );
    if (ec == beast::errc::no_such_file_or_directory)
        return e404();
    if (ec)
        return e500();
    if (!mime_type.empty())
        res.set(http::field::content_type, mime_type);
    res.set(http::field::cache_control, cache_control.empty()
        ? "max-age=" + std::to_string(60 * 60 * 12) // 12 hrs
        : cache_control);
    res.keep_alive(keep_alive);
    res.prepare_payload();
    return res;
}

// Return a response for the given request.
//
// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template <class Body, class Allocator>
http::message_generator
handle_request(
    http::request<Body, http::basic_fields<Allocator>>&& req)
{
    // Returns a bad request response
    auto const bad_request =
    [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // public cdn request
    std::string_view path = req.target();
    if (req.method() == http::verb::get)
        return file_resp(path, req.keep_alive());

    // Everything beyond this point should be using authenticated API endpoints
    if (!path.starts_with("/api/"))
        return e404();
    path.remove_prefix(4);

    // Check auth for api
    auto auth = req[http::field::authorization];
    if (auth.starts_with("Bearer "))
        auth.remove_prefix(7);
    if (auth != Config::bearer_token) {
        http::response<http::empty_body> res{http::status::unauthorized, req.version()};
        res.keep_alive(req.keep_alive());
        return res;
    }

    boost::urls::url_view uv;
    {
        const auto uvr = boost::urls::parse_origin_form(path);
        if (!uvr)
            return bad_request("Invalid path: " + uvr.error().what());
        uv = *uvr;
    }
    const auto segments = uv.encoded_segments();
    auto seg = segments.begin();

    switch (req.method()) {
    case http::verb::post: {
        if (*seg == "token") {
            if (++seg == segments.end())
                return e404();

            // Parse segments
            // /token/<asset_id>[/<token>]
            const int64_t asset_id = from_rhex(seg->data(), seg->size());
            ++seg;
            int64_t token = seg == segments.end()
                ? -1
                : from_rhex(seg->data(), seg->size());

            // Parse query params
            int32_t expire_seconds = -1;
            for (const auto p : uv.params()) {
                if (p.key == "expire") {
                    if (!p.has_value)
                        continue;
                    if (p.value.empty() || p.value[0] == '-')
                        continue;
                    try {
                        expire_seconds = std::stoi(p.value);
                    } catch (...) {
                        return bad_request("Invalid expire time: " + p.value);
                    }
                }
            }

            if (token == -1)
                token = new_access_token(asset_id, expire_seconds);
            else
                grant_access_token(asset_id, expire_seconds, token);

            char token_str[65];
            to_rhex(token, token_str);
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.body() = token_str;
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.prepare_payload();
            return res;
        } else if (*seg == "asset") {
            std::string url;
            time_t expire_seconds = -1;
            for (const auto p : uv.params()) {
                if (!p.has_value)
                    continue;
                if (p.key == "url") {
                    url = p.value;
                } else if (p.key == "eol") {
                    try {
                        expire_seconds = std::stoi(p.value);
                    } catch (...) {
                        continue;
                    }
                }
            }

            const auto [asset_id, bucket] = new_asset(
                req[http::field::content_type],
                expire_seconds,
                req[http::field::cache_control]
            );
            if (asset_id == (uint32_t) -1)
                return e500("db error");

            const auto fs_path = get_asset_path(bucket, asset_id);

            const auto ok = [&](){
                http::response<http::string_body> res;
                res.result(200);
                char id_str[36];
                to_rhex(asset_id, id_str);
                res.body() = id_str;
                res.set(http::field::content_type, "application/json");
                res.keep_alive(req.keep_alive());
                res.prepare_payload();
                return res;
            };

            if (!url.empty()) {
                if (download_file_curl(url, fs_path)) {
                    return ok();
                } else {
                    http::response<http::string_body> res;
                    res.result(500);
                    res.body() = "Failed to download file";
                    res.keep_alive(req.keep_alive());
                    res.set(http::field::content_type, "text/plain");
                    res.prepare_payload();
                    return res;

                    // Asset will eventually get deleted from db by purge() function
                }
            } else if (!req.body().empty()) {
                std::ofstream of{fs_path};
                if (of && of << req.body()) {
                    return ok();
                } else {
                    http::response<http::string_body> res;
                    res.result(500);
                    res.body() = "Failed to create asset";
                    res.set(http::field::content_type, "text/plain");
                    res.keep_alive(req.keep_alive());
                    res.prepare_payload();
                    return res;
                }
            } else {
                return bad_request("expected either a body or a url query parameter");
            }
        } else {
            return e404();
        }
    }
    break;

    case http::verb::delete_: {
        if (seg == segments.end())
            return e404();
        if (*seg == "token") {
            ++seg;
            if (seg == segments.end())
                return e404();

            delete_token(*seg);

            http::response<http::empty_body> res;
            res.result(200);
            res.keep_alive(req.keep_alive());
            res.prepare_payload();
            return res;
        } else if (*seg == "asset") {
            ++seg;
            if (seg == segments.end())
                return e404();

            delete_asset(*seg);

            http::response<http::empty_body> res;
            res.result(200);
            res.keep_alive(req.keep_alive());
            res.prepare_payload();
            return res;
        } else {
            return e404();
        }
    }

    default:
        return bad_request("Invalid http method");
    }
}

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}


// Handles an HTTP server connection
void
do_session(
    beast::tcp_stream& stream,
    net::yield_context yield)
{
    beast::error_code ec;

    // This buffer is required to persist across reads
    beast::flat_buffer buffer;

    // This lambda is used to send messages
    for(;;)
    {
        // Set the timeout.
        stream.expires_after(std::chrono::seconds(30));

        // Read a request
        http::request<http::string_body> req;
        http::async_read(stream, buffer, req, yield[ec]);
        if(ec == http::error::end_of_stream)
            break;
        if(ec)
            return fail(ec, "read");

        // Handle the request
        http::message_generator msg = handle_request(std::move(req));

        // Determine if we should close the connection
        bool keep_alive = msg.keep_alive();

        // Send the response
        beast::async_write(stream, std::move(msg), yield[ec]);

        if(ec)
            return fail(ec, "write");

        if(! keep_alive)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            break;
        }
    }

    // Send a TCP shutdown
    stream.socket().shutdown(tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
}

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
void
do_listen(
    net::io_context& ioc,
    tcp::endpoint endpoint,
    net::yield_context yield)
{
    beast::error_code ec;

    // Open the acceptor
    tcp::acceptor acceptor(ioc);
    acceptor.open(endpoint.protocol(), ec);
    if(ec)
        return fail(ec, "open");

    // Allow address reuse
    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if(ec)
        return fail(ec, "set_option");

    // Bind to the server address
    acceptor.bind(endpoint, ec);
    if(ec)
        return fail(ec, "bind");

    // Start listening for connections
    acceptor.listen(net::socket_base::max_listen_connections, ec);
    if(ec)
        return fail(ec, "listen");

    for(;;)
    {
        tcp::socket socket(ioc);
        acceptor.async_accept(socket, yield[ec]);
        if(ec)
            fail(ec, "accept");
        else
            boost::asio::spawn(
                acceptor.get_executor(),
                std::bind(
                    &do_session,
                    beast::tcp_stream(std::move(socket)),
                    std::placeholders::_1),
                    // we ignore the result of the session,
                    // most errors are handled with error_code
                    boost::asio::detached);
    }
}

int main(int argc, char* argv[]) {
    if (!Config::from_env())
        return -1;
    Config::prep_data_dir();
    srand(time(nullptr));

    const auto address = net::ip::make_address(Config::listen_addr);
    const auto port = static_cast<unsigned short>(Config::listen_port);

    const auto threads = std::max<int>(1, std::atoi(argv[4]));

    // The io_context is required for all I/O
    net::io_context ioc{threads};

    // Spawn a listening port
    boost::asio::spawn(ioc,
        std::bind(
            &do_listen,
            std::ref(ioc),
            tcp::endpoint{address, port},
            std::placeholders::_1),
        // on completion, spawn will call this function
        [](std::exception_ptr ex)
        {
            // if an exception occurred in the coroutine,
            // it's something critical, e.g. out of memory
            // we capture normal errors in the ec
            // so we just rethrow the exception here,
            // which will cause `ioc.run()` to throw
            if (ex)
                std::rethrow_exception(ex);
        });

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    return EXIT_SUCCESS;
}