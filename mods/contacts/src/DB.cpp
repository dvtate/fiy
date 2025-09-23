//
// Created by tate on 7/30/25.
//

#include <nlohmann/json.hpp>

#include "../../../modlib/fediymod.hpp"

#include "Contact.hpp"

#include "DB.hpp"

extern fiy::HostInfo g_host_info;

namespace DB {
    SQLite::Database& connection() {
        static thread_local SQLite::Database db{
            std::string(g_host_info.data_dir) + "/db.db3",
            SQLite::OPEN_READWRITE
        };
        return db;
    }
    inline SQLite::Statement statement(const char* sql) {
        return SQLite::Statement{ connection(), sql };
    }
    void transaction_begin() {
        static thread_local SQLite::Statement q{connection(), "BEGIN TRANSACTION"};
        q.exec();
        q.reset();
    }
    void transaction_commit() {
        static thread_local SQLite::Statement q{connection(), "COMMIT"};
        q.exec();
        q.reset();
    }
    void transaction_rollback() {
        static thread_local SQLite::Statement q{connection(), "ROLLBACK"};
        q.exec();
        q.reset();
    }

    bool is_profile_card(const int64_t card_id) {
        thread_local auto query = statement("SELECT owner, user FROM Cards WHERE id=?");
        query.bind(1, card_id);
        if (query.executeStep()) {
            query.reset();
            return query.getColumn(0).getString() == query.getColumn(1).getString();
        }
        query.reset();
        return false; // invalid contact id === not a profile
    }

    Result create_new_contact(VC& card) {
        // Start transaction
        transaction_begin();
        g_host_info.log(3, "Creating new contact");

        try {
            thread_local auto q_create_card = statement(
                "INSERT INTO Cards (owner, user, updateTs) VALUES (?, ?, ?);"
            );
            thread_local auto q_get_card = statement(
                "SELECT id, updateTs FROM Cards WHERE rowid=last_insert_rowid();"
            );
            thread_local auto q_insert_prop = statement(
            "INSERT INTO Properties (name, params, value) VALUES (?, ?, ?); "
            );
            thread_local auto q_insert_profile_prop = statement(
                "INSERT INTO ProfileCardProperties (cardId, propertyId, visibility) VALUES (?, last_insert_rowid(), ?); "
            );
            thread_local auto q_insert_card_prop = statement(
                "INSERT INTO CardProperties (cardId, propertyId) VALUES (?, last_insert_rowid()); "
            );
            thread_local auto q_last_insert_rowid = statement(
                "SELECT last_insert_rowid();"
            );

            // Bind params to create card query
            q_create_card.bindNoCopy(1, card.owner);
            if (card.user.empty())
                q_create_card.bind(2);
            else
                q_create_card.bindNoCopy(2, card.user);
            q_create_card.bind(3, g_host_info.now());
            int rows_modified = q_create_card.exec();
            if (rows_modified == 0)
                g_host_info.log(1, "Didn't create card???");
            q_create_card.reset();

            // Execute create card query
            if (q_get_card.executeStep()) {
                card.id = q_get_card.getColumn(0).getInt64();
                card.update_ts = q_get_card.getColumn(1).getInt64();
            }
            if (card.id == -1) {
                transaction_rollback();
                g_host_info.log(1, "Couldn't get created card id");
                q_get_card.reset();
                return Error;
            }
            q_get_card.reset();

            // Insert properties
            if (card.owner == card.user) {
                // Profile card
                for (auto& p: card.props) {
                    // Create card property
                    q_insert_prop.bindNoCopy(1, p.name);
                    q_insert_prop.bindNoCopy(2, p.params);
                    q_insert_prop.bindNoCopy(3, p.value);
                    if (q_insert_prop.exec() == 0) {
                        q_insert_prop.reset();
                        transaction_rollback();
                        g_host_info.log(1, "Couldn't create profile property");
                        return Error;
                    }
                    q_insert_prop.reset();

                    // Create profile card property
                    q_insert_profile_prop.bind(1, card.id);
                    q_insert_profile_prop.bind(2, p.visibility);
                    if (q_insert_profile_prop.exec() == 0) {
                        q_insert_profile_prop.reset();
                        transaction_rollback();
                        g_host_info.log(1, "Couldn't create profile card property");
                        return Error;
                    }
                    q_insert_profile_prop.reset();

                    // Update contact
                    if (q_last_insert_rowid.executeStep())
                        p.id = q_last_insert_rowid.getColumn(0).getInt();
                    q_last_insert_rowid.reset();
                }
            } else {
                // Not a profile card
                for (auto& p: card.props) {
                    // Create property
                    q_insert_prop.bindNoCopy(1, p.name);
                    q_insert_prop.bindNoCopy(2, p.params);
                    q_insert_prop.bindNoCopy(3, p.value);
                    if (q_insert_prop.exec() == 0) {
                        q_insert_prop.reset();
                        transaction_rollback();
                        g_host_info.log(1, "Couldn't create contact card property");
                        return Error;
                    }
                    q_insert_prop.reset();

                    // Create card property
                    q_insert_card_prop.bind(1, card.id);
                    if (q_insert_card_prop.exec() == 0) {
                        q_insert_card_prop.reset();
                        transaction_rollback();
                        g_host_info.log(1, "Couldn't create card property");
                        return Error;
                    }
                    q_insert_card_prop.reset();

                    // Update contact
                    if (q_last_insert_rowid.executeStep())
                        p.id = q_last_insert_rowid.getColumn(0).getInt64();
                    q_last_insert_rowid.reset();
                }
            }

            // Commit transaction
            transaction_commit();
            return Success;
        } catch (const SQLite::Exception& e) {
            transaction_rollback();
            std::string msg = "contacts: new_contact: database error: ";
            msg += e.what();
            g_host_info.log(1, msg.c_str());
            g_host_info.log(1, e.getErrorStr());
            return Error;
        }
    }

    Result update_contact(VC& card) {
        // Update a card
        transaction_begin();
        g_host_info.log(3, "Updating contact");

        try {
            // Update user
            static thread_local SQLite::Statement q_update_card{
                connection(),
                "UPDATE Cards SET user=?, updateTs=? WHERE id=? AND owner=?"
            };
            if (card.user.empty())
                q_update_card.bind(1);
            else
                q_update_card.bindNoCopy(1, card.user);
            q_update_card.bind(2, card.update_ts = g_host_info.now());
            q_update_card.bind(3, card.id);
            q_update_card.bind(4, card.owner);
            const int updated = q_update_card.exec();
            q_update_card.reset();
            if (!updated) {
                transaction_rollback();
                return Unauthorized; // Probably authentication issue
            }

            // Clean out old data
            static thread_local SQLite::Statement q_remove_props1{
                connection(),
                "DELETE FROM CardProperties WHERE cardId=?;"
            };

            q_remove_props1.bind(1, card.id);
            q_remove_props1.exec();
            q_remove_props1.reset();
            static thread_local SQLite::Statement q_remove_props2{
                connection(),
                "DELETE FROM ProfileCardProperties WHERE cardId=?;"
            };
            q_remove_props2.bind(1, card.id);
            q_remove_props2.exec();
            q_remove_props2.reset();
            static thread_local SQLite::Statement q_remove_props3{
                connection(),
                "DELETE FROM Properties WHERE id NOT IN ("
                    "SELECT propertyId FROM CardProperties "
                    "UNION SELECT propertyId FROM ProfileCardProperties"
                ");"
            };
            q_remove_props3.exec();
            q_remove_props3.reset();


            static thread_local SQLite::Statement q_insert_prop{
                connection(),
                "INSERT INTO Properties (name, params, value) VALUES (?, ?, ?); "
            };
            static thread_local SQLite::Statement q_insert_profile_prop{
                connection(),
                "INSERT INTO ProfileCardProperties (cardId, propertyId, visibility) VALUES (?, last_insert_rowid(), ?); "
            };
            static thread_local SQLite::Statement q_insert_card_prop{
                connection(),
                "INSERT INTO CardProperties (cardId, propertyId) VALUES (?, last_insert_rowid()); "
            };
            thread_local auto q_last_insert_rowid = statement(
                "SELECT last_insert_rowid();"
            );

            // Insert properties
            if (card.owner == card.user) {
                // Profile card
                for (auto& p: card.props) {
                    q_insert_prop.bindNoCopy(1, p.name);
                    q_insert_prop.bindNoCopy(2, p.params);
                    q_insert_prop.bindNoCopy(3, p.value);
                    bool updated = q_insert_prop.exec() != 0;
                    q_insert_prop.reset();
                    if (!updated) {
                        transaction_rollback();
                        g_host_info.log(1, "Couldn't insert contact property");
                        return Error;
                    }
                    q_insert_prop.reset();

                    q_insert_profile_prop.bind(1, card.id);
                    q_insert_profile_prop.bind(2, p.visibility);
                    updated = q_insert_profile_prop.exec() != 0;
                    q_insert_profile_prop.reset();
                    if (!updated) {
                        transaction_rollback();
                        g_host_info.log(1, "Couldn't insert profile card property");
                        return Error;
                    }
                    q_insert_profile_prop.reset();

                    if (q_last_insert_rowid.executeStep())
                        p.id = q_last_insert_rowid.getColumn(0).getInt64();
                    q_last_insert_rowid.reset();
                }
            } else {
                // Not a profile card
                for (auto& p: card.props) {
                    q_insert_prop.bindNoCopy(1, p.name);
                    q_insert_prop.bindNoCopy(2, p.params);
                    q_insert_prop.bindNoCopy(3, p.value);
                    bool updated = q_insert_prop.exec() != 0;
                    q_insert_prop.reset();
                    if (!updated) {
                        transaction_rollback();
                        g_host_info.log(1, "Couldn't insert contact card property");
                        return Error;
                    }
                    q_insert_prop.reset();

                    q_insert_card_prop.bind(1, card.id);
                    updated = q_insert_card_prop.exec() != 0;
                    if (!updated) {
                        q_insert_card_prop.reset();
                        transaction_rollback();
                        g_host_info.log(1, "Couldn't insert card property");
                        return Error;
                    }
                    q_insert_card_prop.reset();

                    if (q_last_insert_rowid.executeStep())
                        p.id = q_last_insert_rowid.getColumn(0).getInt64();
                    q_last_insert_rowid.reset();
                }
            }

            transaction_commit();
            return Success;
        } catch (const SQLite::Exception& e) {
            transaction_rollback();
            std::string msg = "contacts: update_contact: database error: ";
            msg += e.what();
            g_host_info.log(1, msg.c_str());
            return Error;
        }
    }

    /**
     * Create a new contact
     * @param card contact to store
     * @remark if card.id < 0, create new contact and update the card
     * @return true if successful
     */
    Result save_contact(VC& card) {
        return card.id < 0
            ? create_new_contact(card)
            : update_contact(card);
    }

    VC get_profile_base(const std::string& user) {
        if (user.find('@') != std::string::npos) {
            return {};
        }

        static thread_local SQLite::Statement query{
            connection(),
            "SELECT id, updateTs FROM Cards WHERE owner = user AND user=? LIMIT 1"
        };
        query.bindNoCopy(1, user);

        VC ret;
        ret.id = -1;
        ret.user = user;
        ret.owner = user;
        ret.update_ts = 0;
        while (query.executeStep()) {
            ret.id = query.getColumn(0).getInt64();
            ret.update_ts = query.getColumn(1).getInt64();
        }
        query.reset();
        if (ret.id == -1) {
            // Ignore non-existent local users
            if (user.find('@') == std::string_view::npos
                && g_host_info.user_info(user.c_str(), nullptr) != 0) {
                g_host_info.log(2, "Non-existent user" + user);
                return ret;
            }

            // Create a profile for them
            save_contact(ret);
        }
        return ret;
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
        return req_user.empty()
            ? 3
            : req_domain.empty()
                ? (user != req_user ? 1 : 0)
                : 2;
    }
    static constexpr int visibility(
        const std::string& user,
        const char* req_user,
        const char* req_domain
    ) {
        return req_user == nullptr
            ? 3
            : req_domain == nullptr
                ? (user != req_user ? 1 : 0)
                : 2;
    }

    std::string get_profile(
        const std::string& user,
        const char* req_user,
        const char* req_domain
    ) {
        const int trust_level = visibility(user, req_user, req_domain);

        // Start with just the card
        VC ret = get_profile_base(user);
        if (ret.invalid()) {
            g_host_info.log(2, "Could not get profile base for user " + user);
            return "";
        }

        if (trust_level == 0 || trust_level == 1) {
            static thread_local SQLite::Statement query{connection(),
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
        } else {
            // TODO also get card id?
            static thread_local SQLite::Statement query{connection(),
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
        }
        return ret.to_vcard();
    }

    std::string get_user_rolodex(const std::string& local_user) {
        static thread_local SQLite::Statement q_get_cards{
            connection(),
            "SELECT id, user, updateTs FROM Cards WHERE owner=?"};
        static thread_local SQLite::Statement q_get_card_props{
            connection(),
            "SELECT id, name, params, value FROM Properties"
                " WHERE id IN (SELECT propertyId AS id FROM CardProperties WHERE cardId=?);"
        };
        static thread_local SQLite::Statement q_get_prof_props{
            connection(),
            "SELECT propertyId, name, params, value, visibility"
            " FROM Properties INNER JOIN ProfileCardProperties on Properties.id = ProfileCardProperties.propertyId"
            " WHERE cardId=?;"
        };

        bool found_profile = false;
        std::string ret;

        q_get_cards.reset();
        q_get_cards.bindNoCopy(1, local_user);
        while (q_get_cards.executeStep()) {
            VC c;
            c.id = q_get_cards.getColumn(0).getInt64();
            c.owner = local_user;
            c.user = q_get_cards.getColumn(1).getString();
            c.update_ts = q_get_cards.getColumn(2).getInt64();

            if (found_profile || c.user != c.owner) {
                q_get_card_props.reset();
                q_get_card_props.bind(1, c.id);
                while (q_get_card_props.executeStep()) {
                    c.props.emplace_back(VC::Prop{
                        .id=q_get_card_props.getColumn(0).getInt64(),
                        .name=q_get_card_props.getColumn(1).getString(),
                        .params=q_get_card_props.getColumn(2).getString(),
                        .value=q_get_card_props.getColumn(3).getString()
                    });
                }
            } else {
                q_get_prof_props.reset();
                q_get_prof_props.bind(1, c.id);
                while (q_get_prof_props.executeStep()) {
                    c.props.emplace_back(VC::Prop{
                        .id=q_get_prof_props.getColumn(0).getInt64(),
                        .name=q_get_prof_props.getColumn(1).getString(),
                        .params=q_get_prof_props.getColumn(2).getString(),
                        .value=q_get_prof_props.getColumn(3).getString(),
                        .visibility=(int8_t)q_get_prof_props.getColumn(4).getInt()
                    });
                }
                found_profile = true;
            }

            ret += c.to_vcard();
            ret += "\n";
        }

        // Add profile to the cards list if it doesn't exist
        if (!found_profile) {
            ret += get_profile(local_user, local_user.c_str(), "");
        }

        return ret;
    }

    std::string get_pfp(
        const std::string& user,
        const char* req_user,
        const char* req_domain
    ) {
        const int vis = visibility(user, req_user, req_domain);

        // Check their contacts
        if (req_user != nullptr && req_domain == nullptr) {
            thread_local auto query_contacts = statement(
                "SELECT value FROM Properties WHERE id IN ("
                    " SELECT propertyId AS id FROM CardProperties WHERE cardId IN ("
                        " SELECT id AS cardId FROM Cards WHERE owner=? AND user=?"
                    " )"
                ") AND name = 'PHOTO'"
            );

            query_contacts.bindNoCopy(1, req_user);
            query_contacts.bindNoCopy(2, user);
            while (query_contacts.executeStep()) {
                auto ret = query_contacts.getColumn(0).getString();
                if (ret.empty())
                    continue;
                query_contacts.reset();
                return ret;
            }
            query_contacts.reset();
        }

        // Check profiles

        thread_local auto query_profiles = statement(
            "SELECT value FROM Properties WHERE id IN ("
                " SELECT propertyId AS id FROM ProfileCardProperties"
                " WHERE cardId=(SELECT id FROM Cards WHERE user=? AND owner=user)"
                    " AND visibility>=?"
            " ) AND name = 'PHOTO'"
        );

        query_profiles.bindNoCopy(1, user);

        query_profiles.bind(2, vis);
        while (query_profiles.executeStep()) {
            auto ret = query_profiles.getColumn(0).getString();
            if (ret.empty())
                continue;
            query_profiles.reset();
            return ret;
        }
        query_profiles.reset();
        return "";
    }


    bool get_contact(VC& card) {
        if (card.id != -1) {
            thread_local auto q_card_trusted = statement(
                "SELECT user, updateTs FROM Cards WHERE id=?"
            );
            thread_local auto q_card_check_owner = statement(
                "SELECT user, updateTs FROM Cards WHERE id=? AND owner=?"
            );

            SQLite::Statement* q;
            if (card.owner.empty()) {
                q = &q_card_trusted;
                q->bind(1, card.id);
            } else {
                q = &q_card_check_owner;
                q->bind(1, card.id);
                q->bindNoCopy(2, card.owner);
            }

            if (!q->executeStep()) {
                q->reset();
                return false;
            }

            card.user = q->getColumn(0).getString();
            card.update_ts = q->getColumn(1).getInt64();
            q->reset();

            thread_local auto q_props = statement(
                "SELECT id, name, params, value FROM Properties WHERE id IN ("
                    "SELECT propertyId AS id FROM ("
                        "SELECT cardId, propertyId FROM CardProperties"
                        // We already checked that they own the card
                        " UNION "
                        "SELECT cardId, propertyId FROM ProfileCardProperties"
                    ") WHERE cardId = (SELECT id FROM Cards WHERE id=?)"
                ")"
            );

            q_props.bind(1, card.id);
            while (q_props.executeStep())
                card.props.emplace_back(VC::Prop{
                    .id=q_props.getColumn(0).getInt64(),
                    .name=q_props.getColumn(1).getString(),
                    .params=q_props.getColumn(2).getString(),
                    .value=q_props.getColumn(3).getString()
                });
            q_props.reset();

            return true;
        }

        if (!card.owner.empty() && !card.user.empty()) {
            // TODO Get card by name+owner
            return false;
        }

        // Get user by id or by name+owner
        return false;
    }


} // namespace DB