//
// Created by tate on 7/30/25.
//

#include <nlohmann/json.hpp>

#include "../../../modlib/fediymod.hpp"

#include "Contact.hpp"

#include "DB.hpp"



extern const fiy_host_info_t* g_host_info;

namespace DB {

    SQLite::Database& connection() {
        static thread_local SQLite::Database db{
            std::string(g_host_info->data_dir) + "/db.db3",
            SQLite::OPEN_READWRITE
        };
        return db;
    }
    void transaction_begin() {
        static thread_local SQLite::Statement q{connection(), "BEGIN TRANSACTION"};
        q.exec();
    }
    void transaction_commit() {
        static thread_local SQLite::Statement q{connection(), "COMMIT"};
        q.exec();
    }
    void transaction_rollback() {
        static thread_local SQLite::Statement q{connection(), "ROLLBACK"};
        q.exec();
    }



    bool is_profile_card(int64_t card_id) {
        static thread_local SQLite::Statement query{connection(), "SELECT owner, user FROM Cards WHERE id=?"};
        query.reset();
        query.bind(1, card_id);
        if (query.executeStep()) {
            return query.getColumn(0).getString() == query.getColumn(1).getString();
        } else {
            return false; // invalid contact id === not a profile
        }
    }

    bool create_new_contact(VC& card) {
        // Start transaction
        transaction_begin();

        try {
            static thread_local SQLite::Statement q_create_card{
                connection(),
                "INSERT INTO Cards (owner, user, updateTs) VALUES (?, ?, ?);"
                "SELECT id FROM cards WHERE rowid=last_insert_rowid();"
            };
            static thread_local SQLite::Statement q_insert_profile_property{
                connection(),
                "INSERT INTO Properties (name, params, value) VALUES (?, ?, ?); "
                "INSERT INTO ProfileCardProperties (cardId, propertyId, visibility) VALUES (?, last_insert_rowid(), ?); "
                "SELECT last_insert_rowid();"
            };
            static thread_local SQLite::Statement q_insert_property{
                connection(),
                "INSERT INTO Properties (name, params, value) VALUES (?, ?, ?); "
                "INSERT INTO CardProperties (cardId, propertyId) VALUES (?, last_insert_rowid()); "
                "SELECT last_insert_rowid();"
            };

            // Bind params to create card query
            q_create_card.bindNoCopy(1, card.owner);
            if (card.user.empty())
                q_create_card.bind(2);
            else
                q_create_card.bindNoCopy(2, card.user);
            q_create_card.bind(3, g_host_info->now());

            // Execute create card query
            while (q_create_card.executeStep())
                card.id = q_create_card.getColumn(0).getInt64();
            if (card.id == -1) {
                q_create_card.reset();
                transaction_rollback();
                return false;
            }

            // Insert properties
            if (card.owner == card.user) {
                // Profile card
                for (auto& p: card.props) {
                    q_insert_profile_property.bindNoCopy(1, p.name);
                    q_insert_profile_property.bindNoCopy(2, p.params);
                    q_insert_profile_property.bindNoCopy(3, p.value);
                    q_insert_profile_property.bind(4, card.id);
                    q_insert_profile_property.bind(5, p.visibility);
                    if (q_insert_profile_property.executeStep())
                        p.id = q_insert_profile_property.getColumn(0).getInt();
                    q_insert_profile_property.reset();
                }
            } else {
                // Not a profile card
                for (auto& p: card.props) {
                    q_insert_property.bindNoCopy(1, p.name);
                    q_insert_property.bindNoCopy(2, p.params);
                    q_insert_property.bindNoCopy(3, p.value);
                    q_insert_property.bind(4, card.id);
                    if (q_insert_property.executeStep())
                        p.id = q_insert_property.getColumn(0).getInt();
                    q_insert_property.reset();
                }
            }

            // Commit transaction
            transaction_commit();
            return true;
        } catch (const SQLite::Exception& e) {
            transaction_rollback();
            std::string msg = "contacts: new_contact: database error: ";
            msg += e.what();
            g_host_info->log(1, msg.c_str());
            return false;
        }
    }

    bool update_contact(VC& card) {
        // Update a card
        transaction_begin();

        try {

            static thread_local SQLite::Statement q_update_card{
                connection(),
                "UPDATE Cards SET user=?, updateTs=? WHERE id=?"
            };
            static thread_local SQLite::Statement q_remove_props{
                connection(),
                "DELETE FROM CardProperties WHERE cardId=?;"
                "DELETE FROM ProfileCardProperties WHERE cardId=?;"
                "DELETE FROM Properties WHERE id NOT IN ("
                    "SELECT propertyId FROM CardProperties "
                    "UNION SELECT propertyId FROM ProfileCardProperties"
                ");"
            };
            static thread_local SQLite::Statement q_insert_profile_property{
                connection(),
                "INSERT INTO Properties (name, params, value) VALUES (?, ?, ?); "
                "INSERT INTO ProfileCardProperties (cardId, propertyId, visibility) VALUES (?, last_insert_rowid(), ?); "
            };
            static thread_local SQLite::Statement q_insert_property{
                connection(),
                "INSERT INTO Properties (name, params, value) VALUES (?, ?, ?); "
                "INSERT INTO CardProperties (cardId, propertyId) VALUES (?, last_insert_rowid()); "
            };

            // Update user
            if (card.user.empty())
                q_update_card.bind(1);
            else
                q_update_card.bindNoCopy(1, card.user);
            q_update_card.bind(2, g_host_info->now());
            q_update_card.bind(3, card.id);
            q_update_card.exec();
            q_update_card.reset();

            // Clean out old data
            q_remove_props.bind(1, card.id);
            q_remove_props.bind(2, card.id);
            q_remove_props.exec();
            q_remove_props.reset();

            // Insert properties
            if (card.owner == card.user) {
                // Profile card
                for (auto& p: card.props) {
                    q_insert_profile_property.bindNoCopy(1, p.name);
                    q_insert_profile_property.bindNoCopy(2, p.params);
                    q_insert_profile_property.bindNoCopy(3, p.value);
                    q_insert_profile_property.bind(4, card.id);
                    q_insert_profile_property.bind(5, p.visibility);
                    q_insert_profile_property.exec();
                    q_insert_profile_property.reset();
                }
            } else {
                // Not a profile card
                for (auto& p: card.props) {
                    q_insert_property.bindNoCopy(1, p.name);
                    q_insert_property.bindNoCopy(2, p.params);
                    q_insert_property.bindNoCopy(3, p.value);
                    q_insert_property.bind(4, card.id);
                    q_insert_property.exec();
                    q_insert_property.reset();
                }
            }

            transaction_commit();
            return true;
        } catch (const SQLite::Exception& e) {
            transaction_rollback();
            std::string msg = "contacts: update_contact: database error: ";
            msg += e.what();
            g_host_info->log(1, msg.c_str());
            return false;
        }
    }

    /**
     * Create a new contact
     * @param card contact to store
     * @remark if card.id < 0, create new contact and update the card
     * @return true if successful
     */
    bool save_contact(VC& card) {
        return card.id < 0
            ? create_new_contact(card)
            : update_contact(card);
    }

    VC get_profile_base(const std::string& user) {
        static thread_local SQLite::Statement query{
            connection(),
            "SELECT id, updateTs FROM Cards WHERE owner = user AND user=? LIMIT 1"
        };
        query.reset();
        query.bindNoCopy(1, user);

        VC ret;
        ret.id = -1;
        ret.user = user;
        ret.owner = user;
        while (query.executeStep()) {
            ret.id = query.getColumn(0).getInt64();
            ret.update_ts = query.getColumn(1).getInt64();
        }
        if (ret.id == -1) {
            // Ignore non-existent local users
            if (user.find('@') == std::string_view::npos
                && g_host_info->user_info(user.c_str(), nullptr) != 0)
                    return ret;

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

    std::string get_profile(
        const std::string& user,
        const std::string& req_user,
        const std::string& req_domain
    ) {
        int trust_level = visibility(user, req_user, req_domain);

        // Start with just the card
        VC ret = get_profile_base(user);

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
            return ret.props.empty() ? "" : ret.to_vcard();
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
            return ret.props.empty() ? "" : ret.to_vcard();
        }
    }

} // namespace DB