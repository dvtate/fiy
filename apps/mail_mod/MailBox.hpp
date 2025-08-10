//
// Created by tate on 8/6/24.
//

#pragma once

#include <ctime>
#include <string>
#include <vector>

class Mail {
public:
    std::string m_from_user;
    std::vector<std::string> m_recipients;
    std::string m_subject;
    std::string m_body;
    time_t m_date;
    size_t m_index;

    Mail(std::string from, std::vector<std::string> to, std::string subject, std::string body,
         time_t ts = std::time(nullptr))
        : m_from_user(std::move(from)),
          m_recipients(std::move(to)),
          m_subject(std::move(subject)),
          m_body(std::move(body)),
          m_date(ts) {}

    [[nodiscard]] std::string short_view() const {
        std::string ret = "<tr>";
        ret += "<td><a href='view/";
        ret += std::to_string(m_index);
        ret += "'>";
        ret += asctime(gmtime(&m_date));
        ret += "</a></td><td>";
        ret += m_from_user;
        ret += "</td><td>";
        if (m_subject.size() > 20) {
            ret += m_subject.substr(0, 17) + "...";
        } else {
            ret += m_subject;
        }
        ret += "</td></tr>";
        return ret;
    }

    [[nodiscard]] std::string long_view() const {
        std::string ret;
        ret += "From: ";
        ret += m_from_user;
        ret += "<hr/>To: ";
        for (const auto& u : m_recipients)
            ret += u + ", ";
        ret.pop_back();
        ret.pop_back();
        ret += "<hr/>Subject: ";
        ret += m_subject;
        ret += "<hr/>";
        ret += m_body;
        ret += "<hr/>Received: ";
        ret += asctime(gmtime(&m_date));
        return ret;
    }
};

class MailBox {
    std::vector<Mail> m_mail;

public:
    std::vector<Mail> get_inbox(const std::string& local_user) {
        std::vector<Mail> ret;
        for (const auto& m : m_mail)
            for (const auto& u : m.m_recipients)
                if (u == local_user)
                    ret.emplace_back(m);
        return ret;
    }

    std::string get_inbox_str(const std::string& local_user) {
        std::string ret = "<table><tr><th>Date</th><th>From</th><th>Subject</ht><tr/>";

        for (const auto& m : m_mail)
            for (const auto& u : m.m_recipients)
                if (u == local_user)
                    ret += m.short_view();
        ret += "</table>";
        return ret;
    }

    std::vector<Mail> get_outbox(const std::string& local_user) {
        std::vector<Mail> ret;
        for (const auto& m : m_mail)
            if (m.m_from_user == local_user)
                ret.emplace_back(m);
        return ret;
    }

    std::string get_outbox_str(const std::string& local_user) {
        std::string ret = "<table><tr><th>Date</th><th>From</th><th>Subject</ht><tr/>";
        for (const auto& m : m_mail)
            if (m.m_from_user == local_user)
                ret += m.short_view();
        ret += "</table>";
        return ret;
    }

    size_t push(Mail&& mail) {
        size_t ret = m_mail.size();
        mail.m_index = ret;
        m_mail.emplace_back(mail);
        return ret;
    }

    Mail* get(size_t index) {
        if (index >= m_mail.size())
            return nullptr;
        else
            return &m_mail[index];
    }
};
