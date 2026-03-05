//
// Created by tate on 7/30/25.
//

#include <nlohmann/json.hpp>

#include "../../../modlib/fediymod.hpp"

#include "Contact.hpp"

#include "DB.hpp"

namespace DB {
    SQLite::Database& connection() {
        thread_local SQLite::Database db{
            std::string(fiy::host().data_dir) + "/db.db3",
            SQLite::OPEN_READWRITE
        };
        return db;
    }
    inline SQLite::Statement statement(const char* sql) {
        return SQLite::Statement{ connection(), sql };
    }
    inline SQLite::Statement operator ""_sql(const char* str, std::size_t) {
        return statement(str);
    }

    void transaction_begin() {
        thread_local auto q = "BEGIN TRANSACTION"_sql;
        q.exec();
        q.reset();
    }
    void transaction_commit() {
        thread_local auto q = "COMMIT"_sql;
        q.exec();
        q.reset();
    }
    void transaction_rollback() {
        thread_local auto q = "ROLLBACK"_sql;
        q.exec();
        q.reset();
    }

    bool is_profile_card(const int64_t card_id) {
        thread_local auto query = "SELECT owner, user FROM Cards WHERE id=?"_sql;
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
        fiy::log_info("Creating new contact");

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
            q_create_card.bind(3, fiy::host().now());
            int rows_modified = q_create_card.exec();
            if (rows_modified == 0)
                fiy::log_error("Failed to create new contact card properties");
            q_create_card.reset();

            // Execute create card query
            if (q_get_card.executeStep()) {
                card.id = q_get_card.getColumn(0).getInt64();
                card.update_ts = q_get_card.getColumn(1).getInt64();
            }
            if (card.id == -1) {
                transaction_rollback();
                fiy::log_error("Couldn't get created card id");
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
                        fiy::log_error("Couldn't create card property");
                        return Error;
                    }
                    q_insert_prop.reset();

                    // Create profile card property
                    q_insert_profile_prop.bind(1, card.id);
                    q_insert_profile_prop.bind(2, p.visibility);
                    if (q_insert_profile_prop.exec() == 0) {
                        q_insert_profile_prop.reset();
                        transaction_rollback();
                        fiy::log_error("Couldn't create profile card property");
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
                        fiy::log_error("Couldn't create contact card property");
                        return Error;
                    }
                    q_insert_prop.reset();

                    // Create card property
                    q_insert_card_prop.bind(1, card.id);
                    if (q_insert_card_prop.exec() == 0) {
                        q_insert_card_prop.reset();
                        transaction_rollback();
                        fiy::log_error("Couldn't create card property");
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
            msg += '\n';
            msg += e.getErrorStr();
            fiy::log_error(msg);
            return Error;
        }
    }

    /// Delete properties that are not in use
    void prune_properties() {
        thread_local auto q = "DELETE FROM Properties WHERE id NOT IN ("
            " SELECT propertyId FROM ProfileCardProperties"
            " UNION SELECT propertyId FROM CardProperties)"_sql;
        q.exec();
        q.reset();
    }

    Result update_contact(VC& card) {
        // Update a card
        transaction_begin();
        fiy::log_info("Updating contact");

        try {
            // Update user
            thread_local auto q_update_card = "UPDATE Cards SET user=?, updateTs=? WHERE id=? AND owner=?"_sql;
            if (card.user.empty())
                q_update_card.bind(1);
            else
                q_update_card.bindNoCopy(1, card.user);
            q_update_card.bind(2, card.update_ts = fiy::host().now());
            q_update_card.bind(3, card.id);
            q_update_card.bind(4, card.owner);
            const int updated = q_update_card.exec();
            q_update_card.reset();
            if (!updated) {
                transaction_rollback();
                return Unauthorized; // Probably authentication issue
            }

            // Clean out old data
            thread_local auto q_remove_props1 = "DELETE FROM CardProperties WHERE cardId=?;"_sql;
            q_remove_props1.bind(1, card.id);
            q_remove_props1.exec();
            q_remove_props1.reset();
            thread_local auto q_remove_props2 = "DELETE FROM ProfileCardProperties WHERE cardId=?;"_sql;
            q_remove_props2.bind(1, card.id);
            q_remove_props2.exec();
            q_remove_props2.reset();

            // Remove any properties that are no longer in use
            prune_properties();


            thread_local SQLite::Statement q_insert_prop{
                connection(),
                "INSERT INTO Properties (name, params, value) VALUES (?, ?, ?); "
            };
            thread_local SQLite::Statement q_insert_profile_prop{
                connection(),
                "INSERT INTO ProfileCardProperties (cardId, propertyId, visibility) VALUES (?, last_insert_rowid(), ?); "
            };
            thread_local SQLite::Statement q_insert_card_prop{
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
                        fiy::log_error("Couldn't insert contact property");
                        return Error;
                    }
                    q_insert_prop.reset();

                    q_insert_profile_prop.bind(1, card.id);
                    q_insert_profile_prop.bind(2, p.visibility);
                    updated = q_insert_profile_prop.exec() != 0;
                    q_insert_profile_prop.reset();
                    if (!updated) {
                        transaction_rollback();
                        fiy::log_error("Couldn't insert profile card property");
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
                        fiy::log_error("Couldn't insert contact card property");
                        return Error;
                    }
                    q_insert_prop.reset();

                    q_insert_card_prop.bind(1, card.id);
                    updated = q_insert_card_prop.exec() != 0;
                    if (!updated) {
                        q_insert_card_prop.reset();
                        transaction_rollback();
                        fiy::log_error("Couldn't insert card property");
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
            std::string msg = "update_contact: database error: ";
            msg += e.what();
            fiy::log_error(msg);
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

        thread_local auto query = "SELECT id, updateTs FROM Cards WHERE owner = user AND user=? LIMIT 1"_sql;
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
                && fiy::host().user_info(user.c_str(), nullptr) != 0) {
                fiy::log_warning("Non-existent user: " + user);
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

    VC get_profile(
        const std::string& user,
        const char* req_user,
        const char* req_domain
    ) {
        const int trust_level = visibility(user, req_user, req_domain);

        // Start with just the card
        VC ret = get_profile_base(user);
        if (ret.invalid()) {
            fiy::log_warning("Could not get profile base for user " + user);
            return {};
        }

        if (trust_level == 0 || trust_level == 1) {
            thread_local auto query = statement(
                "SELECT id, name, params, value FROM Properties WHERE id IN ( "
                    "SELECT propertyId AS id FROM ProfileCardProperties "
                    "WHERE cardId=(SELECT id FROM Cards WHERE owner = user AND user=?) "
                        "AND visibility >= ?"

                    " UNION " // this also removes duplicates

                    "SELECT propertyId AS id FROM CardProperties WHERE cardId IN ( "
                        "SELECT id AS cardId FROM Cards "
                        "WHERE owner=? AND user=?"
                    ")"
                ")"
            );
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
            thread_local auto query =
                  "SELECT id, name, params, value FROM Properties WHERE id IN ( "
                      "SELECT propertyId AS id FROM ProfileCardProperties "
                      "WHERE cardId=(SELECT id FROM Cards WHERE owner = user AND user=?) "
                        "AND visibility >= ?"
                  ")"_sql;

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
            query.reset();
        }
        return ret;
    }

    std::string get_user_rolodex(const std::string& local_user) {
        thread_local auto q_get_cards = "SELECT id, user, updateTs FROM Cards WHERE owner=?"_sql;
        thread_local auto q_get_card_props =
            "SELECT id, name, params, value FROM Properties"
                " WHERE id IN (SELECT propertyId AS id FROM CardProperties WHERE cardId=?)"_sql;
        thread_local auto q_get_prof_props =
            "SELECT propertyId, name, params, value, visibility"
            " FROM Properties INNER JOIN ProfileCardProperties on Properties.id = ProfileCardProperties.propertyId"
            " WHERE cardId=?"_sql;

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
            ret += get_profile(local_user, local_user.c_str(), "").to_vcard();
        }

        return ret;
    }

    std::string get_pfp(
        const std::string& user,
        const char* req_user,
        const char* req_domain
    ) {
        // Check their contacts
        if (req_user != nullptr && req_domain == nullptr) {
            thread_local auto query_contacts =
                "SELECT value FROM Properties WHERE id IN ("
                    " SELECT propertyId AS id FROM CardProperties WHERE cardId IN ("
                        " SELECT id AS cardId FROM Cards WHERE owner=? AND user=?"
                    " )"
                ") AND name = 'PHOTO'"_sql;

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
        thread_local auto query_profiles =
            "SELECT value FROM Properties WHERE id IN ("
                " SELECT propertyId AS id FROM ProfileCardProperties"
                " WHERE cardId=(SELECT id FROM Cards WHERE user=? AND owner=user)"
                    " AND visibility>=?"
            " ) AND name = 'PHOTO'"_sql;

        const int vis = visibility(user, req_user, req_domain);
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


    Result get_contact(VC& card) {
        // Get card by id
        if (card.id != -1) {
            thread_local auto q_card_trusted = "SELECT user, updateTs FROM Cards WHERE id=?"_sql;
            thread_local auto q_card_check_owner = "SELECT user, updateTs FROM Cards WHERE id=? AND owner=?"_sql;

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
                return Result::NotFound; // Could also mean unauthorized
            }

            card.user = q->getColumn(0).getString();
            card.update_ts = q->getColumn(1).getInt64();
            q->reset();

            thread_local auto q_props =
                "SELECT id, name, params, value FROM Properties WHERE id IN ("
                    "SELECT propertyId AS id FROM ("
                        "SELECT cardId, propertyId FROM CardProperties"
                        // We already checked that they own the card
                        " UNION "
                        "SELECT cardId, propertyId FROM ProfileCardProperties"
                    ") WHERE cardId = (SELECT id FROM Cards WHERE id=?)"
                ")"_sql;

            q_props.bind(1, card.id);
            while (q_props.executeStep())
                card.props.emplace_back(VC::Prop{
                    .id=q_props.getColumn(0).getInt64(),
                    .name=q_props.getColumn(1).getString(),
                    .params=q_props.getColumn(2).getString(),
                    .value=q_props.getColumn(3).getString()
                });
            q_props.reset();

            return Result::Success;
        }

        // Get card by name + owner
        // TODO
        if (!card.owner.empty() && !card.user.empty()) {
            return Result::Error;
        }

        // Get user by id or by name+owner
        fiy::log_error("DB::get_contact, invalid argument");
        return Result::Error;
    }


    void delete_user(const char* local_user) {
        thread_local auto delete_card_props = "DELETE FROM CardProperties"
            " WHERE cardId IN (SELECT id FROM Cards WHERE owner=?)"_sql;
        delete_card_props.bindNoCopy(1, local_user);
        delete_card_props.exec();
        delete_card_props.reset();

        thread_local auto delete_profile_props = "DELETE FROM ProfileCardProperties"
            " WHERE cardId IN (SELECT id FROM Cards WHERE owner=?)"_sql;
        delete_profile_props.bindNoCopy(1, local_user);
        delete_profile_props.exec();
        delete_profile_props.reset();

        thread_local auto delete_cards = "DELETE FROM Cards WHERE owner=?"_sql;
        delete_cards.bindNoCopy(1, local_user);
        delete_cards.exec();
        delete_cards.reset();

        prune_properties();

        thread_local auto delete_shares = "DELETE FROM SharedCards WHERE sendingUser=? OR acceptUser=?"_sql;
        delete_shares.bindNoCopy(1, local_user);
        delete_shares.bindNoCopy(2, local_user);
        delete_shares.exec();
        delete_shares.reset();
    }

    /**
     *
     * @param owner local user that owns the contact
     * @param id cardId of the contact to delete
     * @return true if the contact was deleted, false if there user doesn't have permission
     */
    Result delete_contact(const char* owner, const int64_t id) {
        // Make sure the contact exists and the user owns it
        thread_local auto check_contact = "SELECT 1 FROM Cards WHERE cardId=? AND owner=?"_sql;
        check_contact.bind(1, id);
        check_contact.bindNoCopy(2, owner);
        if (!check_contact.executeStep()) {
            check_contact.reset();
            return Result::NotFound; // Could also mean unauthorized
        }
        check_contact.reset();

        thread_local auto del_shared = "DELETE FROM SharedCards WHERE cardId=?"_sql;
        thread_local auto del_card_props = "DELETE FROM CardProperties WHERE cardId=?"_sql;
        thread_local auto del_profile_props = "DELETE FROM ProfileCardProperties WHERE cardId=?"_sql;
        thread_local auto del_cards = "DELETE FROM Cards WHERE cardId=?"_sql;
        for (auto* q : {
            &del_shared,
            &del_card_props,
            &del_profile_props,
            &del_cards
        }) {
            q->bind(1, id);
            q->exec();
            q->reset();
        }

        // Remove unused properties
        prune_properties();
        return Result::Success;
    }
} // namespace DB