//
// Created by tate on 8/6/24.
//

#include "../../modlib/fediymod.hpp"

#include "MailBox.hpp"

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <set>

fiy::HostInfo g_host_info;

MailBox g_mailbox;

std::vector<std::string> split_string(const std::string& str,
                                      const std::string& delimiter)
{
    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos)
    {
        strings.emplace_back(str.substr(prev, pos - prev));
        prev = pos + delimiter.size();
    }

    // To get the last substring (or only, if delimiter is not found)
    strings.emplace_back(str.substr(prev));

    return strings;
}

std::string user_str(const fiy_request_t* request) {
    std::string ret = request->user;
    ret += '@';
    ret += request->domain != nullptr ? request->domain : g_host_info.domain;
    return ret;
}

void send_mail(fiy::Request& req, fiy_callback_t cb) {
    // Unauthenticated
    if (req.user == nullptr) {
        req.respond(cb, 401, "Unauthenticated");
        return;
    }

    // Invalid body
    // It should be destination(s) \n subject \n mail body
    std::string body = req.body;
    auto i = body.find('\n');
    if (i == std::string::npos) {
        req.respond(cb, 400, "Invalid body");
        return;
    }

    // Get destinations
    const std::string to_str = body.substr(0, i);
    auto to = split_string(to_str, ",");
    const auto old_i = i + 1;
    i = body.find('\n', old_i);
    if (i == std::string::npos) {
        req.respond(cb, 400);
        return;
    }

    // Get subject
    std::string subject = body.substr(old_i, i - old_i);
    const std::string local_dom = std::string("@") + g_host_info.domain;
    for (auto& recip: to)
        if (recip.find('@') == std::string::npos)
            recip += local_dom;

    // Get mail body
    std::string content = body.substr(i);
    std::cout <<"new mail: " <<user_str(&req) <<" : " <<to[0] <<" : " <<subject <<" : " <<content<<std::endl;
    g_mailbox.push(Mail(user_str(&req), to, subject, content));

    // Distribute to peers
    if (req.domain == nullptr) {
        // Get unique remote destinations
        std::set<std::string> doms;
        for (const auto& user : to) {
            auto i = user.find('@');
            if (i != std::string::npos) {
                doms.emplace(user.substr(i + 1));
            }
        }
        doms.erase(g_host_info.domain);

        // Send to destination servers
        for (const auto& dom : doms) {
            req.domain = dom.c_str();
            std::cout <<"Sending to: " <<dom <<std::endl;
            g_host_info.request("mail", &req, nullptr, nullptr);
        }
    }
    req.respond(cb);
}

const char* compose_html = "<!doctype html><html><body>\n"
                           "<form action='javascript:submit()'>\n"
                           "To: <input type='text' id='inp-to' placeholder='tate@dvtt.net,test@example.com,jerry' />\n"
                           "<br/>Subject: <input type='text' id='inp-subject' placeholder='Important message' />\n"
                           "<br/>Body: <textarea id='inp-body'>Write your email here</textarea>\n"
                           "<br/><button type='submit'>Send</button>\n"
                           "</form>\n"
                           "<script>\n"
                           "async function submit() {\n"
                           "    fetch('send', {\n"
                           "            method: 'POST',\n"
                           "            body: document.getElementById('inp-to').value\n"
                           "                  + '\\n' + document.getElementById('inp-subject').value\n"
                           "                  + '\\n' + document.getElementById('inp-body').value\n"
                           "    }).then(v => window.location = 'outbox')\n"
                           "    .catch(console.error);\n"
                           "}\n"
                           "</script>\n"
                           "</body></html>";

static void handle_request(fiy_request_t* request, fiy_callback_t cb) {
    auto& req = *(fiy::Request*) request;

    std::cout <<"Mail: Path: "<<req.path <<std::endl;
    std::cout <<"Mail: User: "<<req.user_str() <<std::endl;

    // Everything here requires a login
    if (req.user == nullptr) {
        static const fiy::Response no_auth_resp{
            303, nullptr, 0,
            "Location: " + g_host_info.host_base_uri() + "/portal/login"
        };
        req.respond(cb, no_auth_resp);
        return;
    }

    if (strcmp(req.path, "/send") == 0) {
        send_mail(req, cb);
        return;
    } else if (strcmp(req.path, "/") == 0) {
        static const std::string root = g_host_info.base_uri;
        static const std::string body = "<ul>"
                           "<li><a href='" + root + "/inbox'>Inbox</a></li>"
                           "<li><a href='" + root + "/outbox'>Outbox</a></li>"
                           "<li><a href='" + root + "/compose'>Compose</a></li>"
                           "</ul>";
        req.respond(cb, body, "Content-Type: text/html");
        return;
    } else if (strcmp(req.path, "/inbox") == 0) {
        // Authenticated local user
        if (req.domain == nullptr && req.user != nullptr) {
            auto inbox_str = g_mailbox.get_inbox_str(user_str(&req));
            req.respond(cb, inbox_str, "Content-Type: text/html");
            return;
        }

        // Unauthenticated
        req.respond(cb, 401, "Unauthenticated");
        return;
    } else if (strcmp(req.path, "/outbox") == 0) {
        if (req.domain == nullptr && req.user != nullptr) {
            auto outbox_str = g_mailbox.get_outbox_str(user_str(&req));
            req.respond(cb, outbox_str, "Content-Type: text/html");
            return;
        }

        // Unauthenticated
        req.respond(cb, 401, "Unauthenticated");
        return;

    } else if (strcmp(req.path, "/compose") == 0) {
        req.respond(cb, compose_html, "Content-Type: text/html");
        return;
    } else if (strncmp(req.path, "/view/", strlen("/view/")) == 0) {
        size_t idx = strtoul(req.path + strlen("/view/"), nullptr, 10);
        // TODO verify that the user has access to the email
        Mail* m = g_mailbox.get(idx);
        if (m == nullptr) {
            req.respond(cb, 404, "Not found");
            return;
        }
        std::string view = m->long_view();
        req.respond(cb, view, "Content-Type: text/html");
        return;
    }

    std::string body = "Hello, @";
    if (req.user != nullptr)
        body += req.user;
    body += "@";
    if (req.domain != nullptr)
        body += req.domain;
    body += "! <br/>Path: ";
//    body += req.method;
    body += " ";
    if (req.path != nullptr)
        body += req.path;

    req.respond(cb, 404, "Not found.");
}

//
//// if domain is null, then user is local
//void (* username_change_handler)(const char* domain, const char* old_username, const char* new_username);
//
///// peer domain changed
//void (* peer_domain_change_handler)(const char* old_domain, const char* new_domain);
//

extern "C" fiy_mod_info_t* start(const fiy_host_info_t* host_info) {
    static fiy_mod_info_t mod_info = {
        .on_request = handle_request,
        .delete_user = nullptr,
    };
    g_host_info = *host_info;
    return &mod_info;
}
