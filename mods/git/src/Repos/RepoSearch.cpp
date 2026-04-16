//
// Created by tate on 4/3/26.
//

#include "RepoSearch.hpp"

#include "../DB.hpp"

// enum Sort {
//     Age,
//     Likes,
//     Name,
//     EditDate,
// };
// Sort sort{Name};
// int limit{20};
// int page{0};

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
