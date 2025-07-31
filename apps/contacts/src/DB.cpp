//
// Created by tate on 7/30/25.
//

#include "DB.hpp"

#include <nlohmann/json.hpp>

#include "../../../modlib/fediymodpp.hpp"

extern const fiy_host_info_t* g_host_info;


DB::DB():
    m_db(std::string(g_host_info->data_dir) + "/db.db3", SQLite::OPEN_FULLMUTEX | SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE ),
    m_get_user_contacts(m_db, "SELECT * FROM Contacts WHERE ownerUserName=?")
{
}

std::vector<Contact> DB::get_contacts(const std::string& owner) {
    std::vector<Contact> ret;
    std::lock_guard mtx{m_mtx};

    m_get_user_contacts.bindNoCopy(1, owner);

    while (m_get_user_contacts.executeStep()) {
        Contact c;
        c.m_id = m_get_user_contacts.getColumn(0).getInt64();
        c.m_name = m_get_user_contacts.getColumn(2).getString();
        c.m_fiy_user = m_get_user_contacts.getColumn(3).getString();

        auto fields_json_str = m_get_user_contacts.getColumn(4).getString();
        if (!fields_json_str.empty()) {
            using namespace nlohmann;
            auto fields_json = json::parse(std::move(fields_json_str));
            std::vector<std::pair<std::string, std::string>> fields;
            if (fields_json.is_array() && !fields_json.empty()) {
                auto n = fields_json.size();
                if (n % 2 == 0)
                    for (int i = 0; i < n; i += 2)
                        fields.emplace_back(fields_json[i], fields_json[i+1]);
                else
                    for (json f : fields_json)
                        if (f.is_array() && f.size() == 2)
                            fields.emplace_back(f[0], f[1]);
                        else
                            g_host_info->log(1, "Database contains invalid data(3)");
            } else {
                g_host_info->log(1, "Database contains invalid data");
            }
            c.m_fields = std::move(fields);
        }
        ret.emplace_back(std::move(c));
    }
    return ret;
}