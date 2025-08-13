//
// Created by tate on 7/30/25.
//

#include <nlohmann/json.hpp>

#include "../../../modlib/fediymod.hpp"

#include "Contact.hpp"

#include "DB.hpp"



extern const fiy_host_info_t* g_host_info;

SQLite::Database& connection() {
    static thread_local SQLite::Database db{
        std::string(g_host_info->data_dir) + "/db.db3",
        SQLite::OPEN_READWRITE
    };
    return db;
}

bool is_profile_card(int64_t card_id) {
    static thread_local SQLite::Statement query{DB::connection(), "SELECT owner, user FROM Cards WHERE id=?"};
    query.reset();
    query.bind(1, card_id);
    if (query.executeStep()) {
        return query.getColumn(0).getString() == query.getColumn(1).getString();
    } else {
        return false; // invalid contact id === not a profile
    }
}

/**
 * Trust level for the request
 * @param user
 * @param req_user
 * @param req_domain
 * @return number indicating trust level
 *  0 - same user
 *  1 - user on same instance
 *  2 - user on different instance
 *  3 - public/unknown/bot user
 */
static constexpr int visibility(
    const std::string& user,
    const std::string& req_user,
    const std::string& req_domain
) {
    return req_user.empty() ? 3
        : req_domain.empty()
        ? (user != req_user ? 1 : 0)
        : 2;
}

std::string DB::get_profile(
    const std::string& user,
    const std::string& req_user,
    const std::string& req_domain
) {
    int trust_level = visibility(user, req_user, req_domain);

    VC ret;
    ret.user = user;
    ret.owner = req_user;

    if (trust_level == 0 || trust_level == 1) {
        static thread_local SQLite::Statement query{DB::connection(),
            "SELECT id, name, params, value FROM Properties WHERE id IN ( "
                "SELECT propertyId AS id FROM ProfileCardProperties "
                "WHERE cardId=(SELECT id FROM Cards WHERE owner = user AND user=?) "
                    "AND visibility >= ?"

                " UNION " // this also removes duplicates

                "SELECT propertyId AS id FROM CardProperties WHERE cardId IN ( "
                    "SELECT id AS cardId FROM Cards "
                    "WHERE owner=? AND user=?"
                ")"
            ");"};
        query.reset();

        query.bindNoCopy(1, user);
        query.bind(2, trust_level);
        query.bindNoCopy(3, req_user);
        query.bindNoCopy(4, user);

        while (query.executeStep()) {
            ret.props.emplace_back(VC::Prop{
                .id=query.getColumn(0).getInt64(),
                .name=query.getColumn(1).getOriginName(),
                .params=query.getColumn(2).getOriginName(),
                .value=query.getColumn(3).getOriginName()
            });
        }
        return ret.props.empty() ? "" : ret.to_vcard();
    } else {
        // TODO also get card id?
        static thread_local SQLite::Statement query{DB::connection(),
              "SELECT id, name, params, value FROM Properties WHERE id IN ( "
                  "SELECT propertyId AS id FROM ProfileCardProperties "
                  "WHERE cardId=(SELECT id FROM Cards WHERE owner = user AND user=?) "
                    "AND visibility >= ?"
              ")"};
        query.reset();

        query.bindNoCopy(1, user);
        query.bind(2, trust_level);

        while (query.executeStep()) {
            ret.props.emplace_back(VC::Prop{
                .id=query.getColumn(0).getInt64(),
                .name=query.getColumn(1).getOriginName(),
                .params=query.getColumn(2).getOriginName(),
                .value=query.getColumn(3).getOriginName()
            });
        }
        return ret.props.empty() ? "" : ret.to_vcard();
    }
}
