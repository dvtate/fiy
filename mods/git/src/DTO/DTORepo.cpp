//
// Created by tate on 4/30/26.
//

#include "DTORepo.hpp"

bool commit_from_json(GitRepo::Commit& commit, const nlohmann::json& json) {
    if (!json.is_object())
        return false;

    // Commit id
    auto it = json.find("id");
    if (it == json.end())
        return false;
    if (!it->is_string())
        return false;
    if (!commit.id_str(it->get<std::string_view>()))
        return false;

    // Commit message
    it = json.find("message");
    if (it != json.end())
        if (json["message"].is_string())
            commit.message = json["message"].get<std::string>();
        // else
        //     return false;

    // Timestamp
    it = json.find("ts");
    if (it != json.end() || !it->is_number_integer())
        return false;
    it->get_to(commit.ts);
    static_assert(std::is_same_v<nlohmann::json::number_integer_t, time_t>);

    // Author & committer
    try {
        it = json.find("author");
        if (it != json.end())
            it->get_to(commit.author);
        it = json.find("committer");
        if (it != json.end())
            it->get_to(commit.committer);
    } catch (const nlohmann::json::exception& e) {
        DEBUG_LOG("json exception: " << e.what());
        return false;
    }
    return true;
}

nlohmann::json DTORepo::DTORepoEntries::to_json() {
    nlohmann::json::array_t ret;
    ret.reserve(this->entries.size());
    for (const auto& [type, path, last_commit] : this->entries) {
        auto o = nlohmann::json::object();
        o["commit"] = {
            { "id", std::string_view(last_commit.id) },
            { "message", last_commit.message },
            { "ts", last_commit.ts },
            // Note: we don't care about author
        };
        o["path"] = path;
        o["type"] = type;
        ret.emplace_back(std::move(o));
    }
    return ret;
}

bool DTORepo::DTORepoEntries::from_json(const nlohmann::json& json) {
    if (json.is_array() == false)
        return false;

    for (const auto& e : json.get<nlohmann::json::array_t>()) {
        if (!e.is_object())
            return false;
        GitRepo::Entry entry;
        auto it = e.find("commit");
        if (it != e.end())
            if (!commit_from_json(entry.last_commit, *it))
                return false;

        it = e.find("path");
        if (it == e.end() || !it->is_string())
            return false;
        it->get_to(entry.path);

        if (!e.contains("type"))
            return false;
        if (e["type"].is_number_unsigned()) {
            switch (e["type"].get<nlohmann::json::number_unsigned_t>()) {
                case GitRepo::Entry::FILE:
                    entry.type = GitRepo::Entry::FILE;
                    break;
                case GitRepo::Entry::DIRECTORY:
                    entry.type = GitRepo::Entry::DIRECTORY;
                    break;
                case GitRepo::Entry::COMMIT:
                    entry.type = GitRepo::Entry::COMMIT;
                    break;
                default:
                    return false;
            }
        } else if (e["type"].is_string()) [[unlikely]] {
            const auto& s = e["type"].get<std::string>();
            if (s == "BLOB" || s == "FILE")
                entry.type = GitRepo::Entry::FILE;
            else if (s == "DIR" || s == "DIRECTORY")
                entry.type = GitRepo::Entry::DIRECTORY;
            else if (s == "COMMIT")
                entry.type = GitRepo::Entry::COMMIT;
            else
                return false;
        } else {
            return false;
        }

        this->entries.emplace_back(entry);
    }

    return true;
}

nlohmann::json DTORepo::DTORepoTreeData::to_json() {
    nlohmann::json ret = nlohmann::json::object();
    auto lc = nlohmann::json::object();
    lc["ts"] = this->last_commit.ts;
    lc["message"] = this->last_commit.message;
    lc["id"] = std::string(this->last_commit.id);
    lc["author"] = {
        { "name", this->last_commit.author.name },
        { "email", this->last_commit.author.email },
        { "user", this->last_commit.author.canonical_user() },
    };
    lc["committer"] = {
        { "name", this->last_commit.committer.name },
        { "email", this->last_commit.committer.email },
        { "user", this->last_commit.committer.canonical_user() },
    };
    ret["commit"] = std::move(lc);
    ret["branch"] = this->active_branch;
    ret["path"] = this->path;
    return ret;
}

bool DTORepo::DTORepoTreeData::from_json(const nlohmann::json& json) {
    if (!json.is_object())
        return false;

    auto it = json.find("commit");
    if (it == json.end())
        return false;
    const auto& lc = *it;
    if (!lc.is_null()) {
        it = lc.find("ts");
        if (it == lc.end())
            return false;
        if (!it->is_number_integer())
            return false;
        it->get_to(this->last_commit.ts);
        static_assert(std::is_same_v<time_t, nlohmann::json::number_integer_t>);

        it = lc.find("id");
        if (it == lc.end() || !it->is_string())
            return false;
        if (!this->last_commit.id_str(it->get<std::string_view>()));
            return false;
    }

    it = json.find("branch");
    if (it == json.end() || !it->is_string())
        return false;
    it->get_to(this->active_branch);

    it = json.find("path");
    if (it == json.end() || !it->is_string())
        return false;
    it->get_to(this->path);

    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DTORepo::DTORepoStats,
    commits_count, tags_count, branches_count, likes_count, tickets_count, forks_count);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DTORepo::DTORepoInfo,
    instance, owner, name, description, fork_of, create_ts, visibility, default_branch);

bool DTORepo::from_json(const nlohmann::json& json) {
    if (!json.is_object())
        return false;

    auto it = json.find("entries");
    if (it != json.end() && !this->entries.from_json(*it))
        return false;

    it = json.find("tree");
    if (it != json.end() && !this->tree.from_json(*it))
        return false;

    it = json.find("stats");
    if (it != json.end()) {
        try {
            it->get_to(this->stats);
        } catch (nlohmann::json::exception& e) {
            DEBUG_LOG("JSON error: " <<e.what());
            return false;
        }
    }

    try {
        it = json.find("info");
        if (it != json.end())
            it->get_to(this->info);
    } catch (nlohmann::json::exception& e) {
        DEBUG_LOG("JSON error: " <<e.what());
        return false;
    }
    return true;
}

nlohmann::json DTORepo::to_json() {
    nlohmann::json ret = nlohmann::json::object();
    ret["entries"] = this->entries.to_json();
    ret["tree"] = this->tree.to_json();
    ret["stats"] = this->stats;
    ret["info"] = this->info;
    return ret;
}