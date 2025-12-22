//
// Created by tate on 12/5/25.
//

#include "Repo.hpp"

#include <git2.h>
#include <nlohmann/json.hpp>

#include "DB.hpp"


using DB::operator ""_sql;

bool BasicRepo::can_access(
        const Access min_level,
        const char* user,
        const char* domain
) {
    if (user != nullptr && this->owner == user)
        return min_level <= Access::Owner;

    if (!this->is_local()) {
        // Proxy request to remote instance
        // if they don't have access it will be blocked
        return true;
    }

    // Check db
    if (min_level <= Access::Read) {
        int64_t id = -1;
        int visibility = 0;
        thread_local auto q_repo = "SELECT id, visibility FROM Repos WHERE userName=? AND repoName=?"_sql;
        q_repo.bindNoCopy(1, this->owner);
        q_repo.bindNoCopy(2, this->name);

        // Query failed
        if (!q_repo.executeStep()) {
            std::cerr <<"DB: Could not find repo : " <<this->path() <<std::endl;
            q_repo.reset();
            return false;
        }

        // Get data from query
        id = q_repo.getColumn(0).getInt64();
        visibility = q_repo.getColumn(1).getInt();
        q_repo.reset();

        switch ((fiy::Locality) visibility) {
            case fiy::Locality::PUBLIC:
                return true;
            case fiy::Locality::FEDIVERSE:
                return user != nullptr && domain != nullptr;
            case fiy::Locality::INSTANCE:
                return user != nullptr && domain == nullptr;
            case fiy::Locality::USER:
                return false;
            default:
                fiy::Host::info.log_warning(
                    "Invalid visibility column value" + std::to_string(visibility));
        }
        return false;
    }

    // Impossible for unauthenticated user to get more than view permissions
    if (user == nullptr)
        return false;


    // Need more granular check.
    thread_local auto q_access = "SELECT level FROM RepoAccess WHERE repoPath=? AND userName=?"_sql;
    q_access.bind(1, this->path());
    std::string user_str = user;
    if (domain != nullptr) {
        user_str += '@';
        user_str += domain;
    }
    q_access.bind(2, user_str);
    while (q_access.executeStep())
        if (q_access.getColumn(0).getInt() >= (int) min_level) {
            q_access.reset();
            return true;
        }
    q_access.reset();
    return false;
}

const char* RepoInfo::create() {
    std::string repo_path = fiy::Host::info.data_dir;
    repo_path += "/repos/" + owner + "/" + name;

    thread_local auto exists = "SELECT 1 FROM Repos WHERE userName=? AND repoName=?"_sql;
    exists.bindNoCopy(1, owner);
    exists.bindNoCopy(2, name);
    if (exists.executeStep())
        return "4XX: Repo with same name already exists";

    std::string user_repos_dir = fiy::Host::info.data_dir;
    user_repos_dir += "/repos/" + owner;
    if (!std::filesystem::is_directory(user_repos_dir))
        if (!std::filesystem::create_directory(user_repos_dir))
            return "5XX: Failed to create user repos directory";
    if (!std::filesystem::create_directory(repo_path))
        return "5XX: Failed to create repo directory";

    // Set options for a bare repository
    git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    opts.flags = GIT_REPOSITORY_INIT_BARE;
    opts.description = this->description.c_str(); // Optional: set a description

    // Create the bare repository
    git_repository *repo = nullptr;
    const int error = git_repository_init_ext(&repo, repo_path.c_str(), &opts);
    if (error < 0) {
        const git_error *e = giterr_last();
        fiy::Host::info.log_error("Error creating bare repository: " + std::string(e->message));
        return "5XX: Server Error: Failed to create bare git repo.";
    }
    git_repository_free(repo); // Free the repository object

    // Add to database
    thread_local auto q = "INSERT INTO Repos (userName, repoName, description, visibility, createTs) VALUES (?, ?, ?, ?, ?)"_sql;
    q.bindNoCopy(1, this->owner);
    q.bindNoCopy(2, this->name);
    q.bindNoCopy(3, this->description);
    q.bind(4, (int)this->visibility);
    q.bind(5, this->create_ts = fiy::Host::info.now());
    const bool ret = q.exec() > 0;
    q.reset();
    if (ret)
        return nullptr;
    return "5XX: Database Error: Failed to create database entry";
}

bool RepoInfo::fetch(
    std::string repo_url_path,
    std::function<void(RepoInfo*)> cb,
    const char* user,
    const char* domain
) {
    RepoInfo repo;
    if (!repo.from_path(std::move(repo_url_path))) {
        cb(nullptr);
        return false;
    }

    if (!repo.is_local()) {

        // This shouldn't happen
        if (domain != nullptr) {
            cb(nullptr);
            return false;
        }

        // TODO remote request
        const std::string request_path = "/api/repo/" + repo.owner + '/' + repo.name;
        const fiy_request_t request{
            .domain = repo.instance.c_str(),
            .user = user,
            .method = fiy::Request::GET,
            .path = request_path.c_str(),
            .headers = "",
            .body_len = 0,
            .body = nullptr
        };
        fiy::Host::info.request(
            fiy::Host::info.app_id,
            &request,
            new std::function(std::move(cb)),
            [](const fiy_response_t* response, void* data) {
                auto& callback = *static_cast<std::function<void(RepoInfo*)>*>(data);
                if (response == nullptr) {
                    fiy::Host::info.log_error("Remote request failed");
                    callback(nullptr);
                    delete &callback;
                    return;
                }
                if (response->status != 200) {
                    fiy::Host::info.log_error("Remote request bad status code");
                    callback(nullptr);
                    delete &callback;
                    return;
                }

                RepoInfo ret;
                if (ret.from_json(fiy::Body::to_string(response->body)))
                    callback(&ret);
                else
                    callback(nullptr);
                delete &callback;
            }
        );
        return true;
    }

    thread_local auto q = "SELECT description, visibility, createTs FROM Repos"
        "    WHERE userName=? AND repoName=?"_sql;
    q.bindNoCopy(1, repo.owner);
    q.bindNoCopy(2, repo.name);
    if (!q.executeStep()) {
        q.reset();
        cb(nullptr);
        return false;
    }
    repo.description = q.getColumn(0).getString();
    repo.visibility = (fiy::Locality)q.getColumn(1).getInt();
    repo.create_ts = q.getColumn(2).getInt64();
    q.reset();

    thread_local auto q_fork = "SELECT fromRepoPath FROM RepoForks WHERE toRepoPath=?"_sql;
    q_fork.bind(1, repo.path());
    if (q_fork.executeStep())
        repo.fork_of = q_fork.getColumn(0).getString();
    q_fork.reset();

    cb(&repo);
    return true;
}


std::string RepoInfo::to_json() {
    nlohmann::json ret;
    ret["instance"] = instance;
    ret["owner"] = owner;
    ret["name"] = name;
    ret["description"] = description;
    ret["visibility"] = (int)visibility;
    ret["fork_of"] = fork_of;
    ret["id"] = id;
    return ret.dump();
}

bool RepoInfo::from_json(const std::string& json) {
    try {
        auto o = nlohmann::json::parse(json.begin(), json.end());
        if (!o.is_object()) {
            return false;
        }
        if (o.contains("instance") && o["instance"].is_string())
            this->instance = o["instance"];
        if (o.contains("owner") && o["owner"].is_string())
            this->owner = o["owner"];
        if (o.contains("name") && o["name"].is_string())
            this->name = o["name"];
        if (o.contains("description") && o["description"].is_string())
            this->description = o["description"];
        if (o.contains("visibility") && o["visibility"].is_number_integer())
            this->visibility = o["visibility"];
        if (o.contains("fork_of") && o["fork_of"].is_string())
            this->fork_of = o["fork_of"];
        if (o.contains("id") && o["id"].is_number_integer())
            this->id = o["id"];
    } catch (const nlohmann::json::parse_error& e) {
        return false;
    }
    return true;
}
