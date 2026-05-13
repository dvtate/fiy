//
// Created by tate on 4/3/26.
//

#include "RepoSearch.hpp"

#include "../DB.hpp"


const std::vector<RepoSearch::Parts> RepoSearch::default_fields{
    Parts::Path,
    Parts::Description
};

void RepoSearch::set_fields(const std::string_view csv) {
    static const boost::unordered_flat_map<std::string_view, Parts> parts{
        // { "invalid", Parts::Invalid },
        { "id", Parts::InternalId },
        { "path", Parts::Path },
        { "description", Parts::Description },
        { "likes", Parts::Likes },
        { "visibility", Parts::Visibility },
        { "fork", Parts::Fork },
        { "created", Parts::CreateTs },
        { "updated", Parts::UpdateTs },
        { "tickets", Parts::Tickets },
        { "1", Parts::InternalId },
        { "2", Parts::Path },
        { "3", Parts::Description },
        { "4", Parts::Likes },
        { "5", Parts::Visibility },
        { "6", Parts::Fork },
        { "7", Parts::CreateTs },
        { "8", Parts::UpdateTs },
        { "9", Parts::Tickets },
    };

    // This can be simplified

    std::string_view delim = ",";
    auto it = csv.find(delim);
    if (it == std::string_view::npos) {
        delim = "%2C";
        it = csv.find(delim);
    }
    std::string_view::size_type start = 0;

    while (it != std::string_view::npos) {
        auto part = csv.substr(start, it - start);
        auto p = parts.find(part);
        if (p != parts.end())
            this->fields.push_back(p->second);
        else
            this->fields.push_back(Parts::Invalid);
        it += delim.size();
        start = it;
        it = csv.find(delim, start);
    }
    if (start != csv.size()) {
        auto part = csv.substr(start, it - start);
        auto p = parts.find(part);
        if (p != parts.end())
            this->fields.push_back(p->second);
        else
            this->fields.push_back(Parts::Invalid);
    }
}

std::string RepoSearch::fields_select_part() {
    std::string ret = "SELECT ";

    const auto& fs = this->fields.empty()
        ? default_fields : this->fields;

    for (const auto f : fs) {
        switch (f) {
        case Parts::Invalid:
            ret += "'invalid field' AS invalid,";
            break;
        case Parts::InternalId:
            ret += "id,";
            break;
        case Parts::Path:
            ret += "CONCAT(userName, '/', repoName) as path,";
            break;
        case Parts::Description:
            ret += "description,";
            break;
        case Parts::Likes:
            ret += "(SELECT COUNT(*) FROM RepoLikes WHERE repoPath=CONCAT(userName, '/', repoName)) as likes,";
            break;
        case Parts::Visibility:
            ret += "visibility,";
            break;
        case Parts::Fork:
            ret += "(SELECT fromRepoPath FROM RepoForks WHERE toRepoPath=CONCAT(userName, '/', repoName) LIMIT 1) as fork";
            break;
        case Parts::CreateTs:
            ret += "createTs,";
            break;
        case Parts::UpdateTs:
            ret += "lastUpdateTs,";
            break;
        case Parts::Tickets:
            ret += "(SELECT COUNT(*) FROM RepoTickets WHERE repoId=id) as tickets,";
            break;
        }
    }

    if (ret.back() == ',')
        ret.pop_back();

    ret += " FROM Repos";

    return ret;
}

nlohmann::json RepoSearch::row_json(const SQLite::Statement& stmt) const {
    auto ret = nlohmann::json::array_t();

    const auto& fs = this->fields.empty()
        ? default_fields : this->fields;

    ret.reserve(fs.size());

    for (unsigned int col = 0; col < fs.size(); col++) {
        switch (fs[col]) {
            case Parts::Invalid:
            case Parts::Description:
            case Parts::Path:
            case Parts::Fork:
                ret.emplace_back(stmt.getColumn(col).getString());
                break;
            case Parts::CreateTs:
            case Parts::InternalId:
            case Parts::Likes:
            case Parts::Tickets:
            case Parts::UpdateTs:
                ret.emplace_back(stmt.getColumn(col).getInt64());
                break;
            case Parts::Visibility:
                ret.emplace_back(stmt.getColumn(col).getInt());
                break;
        }
    }

    return ret;
}

/*
std::vector<BasicRepo> RepoSearch::search(
    const std::string& requesting_user,
    const std::string& requesting_user_instance
) {
    // Considering there are over 200 possible queries,
    //  we can't use pre-compiled queries :(
    DB::QueryBuilder qb;

    qb.part("SELECT userName, repoName FROM Repos");

    // Add where clauses
    //

    // Only include repos the user has access to
    // We can skip this check if they're excluding private repos
    if (!filter_visibility || visibility == fiy::Locality::USER) {
        qb.and_cond();

        fiy::Locality visibility;
        std::string user_str;
        if (requesting_user.empty())
            visibility = fiy::Locality::PUBLIC;
        else
            if (requesting_user_instance.empty() || requesting_user_instance == fiy::host().domain) {
                user_str = requesting_user;
                visibility = fiy::Locality::INSTANCE;
            } else {
                user_str = requesting_user + "@" + requesting_user_instance;
                visibility = fiy::Locality::FEDIVERSE;
            }

        if (visibility == fiy::Locality::PUBLIC) {
            qb.part("visibility >= 3");
        } else {
            qb.part("(visibility > " + std::to_string((int)visibility) + " OR EXISTS ("
                "SELECT 1 FROM RepoAccess WHERE userName=? AND level > 0"
                " AND repoPath=concat(userName, '/', repoName)))",
            [user = std::move(user_str)](SQLite::Statement& q, int& index) {
                q.bind(index++, user);
            });
        }
    }
    if (!owner.empty()) {
        qb.and_cond();
        qb.part("userName=? ", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, owner);
        });
    }
    if (filter_visibility) {
        qb.and_cond();
        qb.part("visibility=?", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, (int)visibility);
        });
    }
    if (min_likes > 0) {
        qb.and_cond();
        qb.part("(SELECT COUNT(*) FROM RepoLikes WHERE repoPath=concat(userName, '/', repoName)) > ?",
            [this](SQLite::Statement& q, int& index) {
                q.bind(index++, min_likes);
            } );
    }
    if (forks != BooleanFilter::Any) {
        qb.and_cond();
        if (forks == BooleanFilter::No)
            qb.part("NOT ");
        qb.part("EXISTS (SELECT 1 FROM RepoForks WHERE toRepoPath=repoName AND user=userName)");
    }
    if (!name_like.empty()) {
        qb.and_cond();
        qb.part("repoName LIKE ?", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, name_like);
        });
    }
    if (!description_like.empty()) {
        qb.and_cond();
        qb.part("description LIKE ?", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, description_like);
        });
    }
    if (!query.empty()) {
        qb.and_cond();
        qb.part("(repoName LIKE ? OR description LIKE ?) ", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, query);
            q.bind(index++, query);
        });
    }

    // Order, limit, offset, etc.
    qb.part(" ORDER BY");
    switch (sort) {
        case Sort::EditDate:
            [[fallthrough]]; // TODO no way to determine this yet
        case Sort::Age:
            qb.part(" createTs");
            break;
        case Sort::Name:
            qb.part(" repoName");
            break;
        case Sort::Likes:
            qb.part(" (SELECT COUNT(*) FROM RepoLikes WHERE repoPath=concat(userName, '/', repoName))");
            break;
        default:
            fiy::host().log_warning("WTF? Invalid RepoSearch::sort: " + std::to_string(sort));
            qb.part(" repoName");
    }
    qb.part(order_desc ? " DESC" : " ASC");

    qb.part(" LIMIT ?", [this](SQLite::Statement& q, int& index) {
        q.bind(index++, limit);
    });
    if (page != 0)
        qb.part(" OFFSET ?", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, page * limit);
        });

    try {
        auto q = qb.build(DB::connection());

        std::vector<BasicRepo> ret;
        if (page != 0 || limit < 25)
            ret.reserve(limit);
        while (q.executeStep())
            ret.emplace_back(
                q.getColumn(0).getString(),
                q.getColumn(1).getString()
            );
        return ret;
    } catch (SQLite::Exception& e) {
        fiy::host().log_error("DB Error: " + std::string(e.what()) + ": " + e.getErrorStr() );
        fiy::host().log_error("DB Error: Statement: " + qb.raw_statement());
        throw e;
    }
}
*/


nlohmann::json RepoSearch::search(
    const std::string& requesting_user,
    const std::string& requesting_user_instance
) {
    auto ret = nlohmann::json::object();

    // Considering there are over 200 possible queries,
    //  we can't use pre-compiled queries :(
    DB::QueryBuilder qb;

    std::string fields_select = fields_select_part();
    qb.part(fields_select);

    // Add where clauses
    //

    // Only include repos the user has access to
    // We can skip this check if they're excluding private repos
    if (!filter_visibility || visibility == fiy::Locality::USER) {
        qb.and_cond();

        fiy::Locality visibility;
        std::string user_str;
        if (requesting_user.empty())
            visibility = fiy::Locality::PUBLIC;
        else
            if (requesting_user_instance.empty() || requesting_user_instance == fiy::host().domain) {
                user_str = requesting_user;
                visibility = fiy::Locality::INSTANCE;
            } else {
                user_str = requesting_user + "@" + requesting_user_instance;
                visibility = fiy::Locality::FEDIVERSE;
            }

        if (visibility == fiy::Locality::PUBLIC) {
            qb.part("visibility >= 3");
        } else {
            qb.part("(visibility > " + std::to_string((int)visibility) + " OR EXISTS ("
                "SELECT 1 FROM RepoAccess WHERE userName=? AND level > 0"
                " AND repoPath=concat(userName, '/', repoName)))",
            [user = std::move(user_str)](SQLite::Statement& q, int& index) {
                q.bind(index++, user);
            });
        }
    }
    if (!owner.empty()) {
        qb.and_cond();
        qb.part("userName=? ", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, owner);
        });
    }
    if (filter_visibility) {
        qb.and_cond();
        qb.part("visibility=?", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, (int)visibility);
        });
    }
    if (min_likes > 0) {
        qb.and_cond();
        qb.part("(SELECT COUNT(*) FROM RepoLikes WHERE repoPath=concat(userName, '/', repoName)) > ?",
            [this](SQLite::Statement& q, int& index) {
                q.bind(index++, min_likes);
            } );
    }
    if (forks != BooleanFilter::Any) {
        qb.and_cond();
        if (forks == BooleanFilter::No)
            qb.part("NOT ");
        qb.part("EXISTS (SELECT 1 FROM RepoForks WHERE toRepoPath=repoName AND user=userName)");
    }
    if (!name_like.empty()) {
        qb.and_cond();
        qb.part("repoName LIKE ?", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, name_like);
        });
    }
    if (!description_like.empty()) {
        qb.and_cond();
        qb.part("description LIKE ?", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, description_like);
        });
    }
    if (!query.empty()) {
        qb.and_cond();
        qb.part("(repoName LIKE ? OR description LIKE ?) ", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, query);
            q.bind(index++, query);
        });
    }

    // Get pagination data
    if (this->page_data) {
        // Replace fields selection with getting the count
        DB::QueryBuilder totals = qb;
        qb.replace_start(fields_select.size(), "SELECT COUNT(*) FROM Repos");
        auto q = qb.build();
        if (q.executeStep()) {
            ret["total"] = q.getColumn(0).getUInt();
        } else {
            ret["total"] = -1;
        }
    }

    // Order, limit, offset, etc.
    qb.part(" ORDER BY");
    switch (sort) {
        case Sort::EditDate:
            qb.part("lastUpdateTs");
            break;
        case Sort::Age:
            qb.part(" createTs");
            break;
        case Sort::Name:
            qb.part(" repoName");
            break;
        case Sort::Likes:
            qb.part(" (SELECT COUNT(*) FROM RepoLikes WHERE repoPath=CONCAT(userName, '/', repoName))");
            break;
        default:
            fiy::host().log_warning("WTF? Invalid RepoSearch::sort: " + std::to_string(sort));
            qb.part(" repoName");
    }
    qb.part(order_desc ? " DESC" : " ASC");

    qb.part(" LIMIT ?", [this](SQLite::Statement& q, int& index) {
        q.bind(index++, limit);
    });
    if (page != 0)
        qb.part(" OFFSET ?", [this](SQLite::Statement& q, int& index) {
            q.bind(index++, page * limit);
        });

    try {
        auto& results = (ret["results"] = nlohmann::json::array());
        auto q = qb.build(DB::connection());
        while (q.executeStep())
            results.emplace_back(this->row_json(q));
        return ret;
    } catch (SQLite::Exception& e) {
        fiy::host().log_error("DB Error: " + std::string(e.what()) + ": " + e.getErrorStr() );
        fiy::host().log_error("DB Error: Statement: " + qb.raw_statement());
        throw e;
    }
}