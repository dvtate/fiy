#pragma once

#include <map>

#include "Mod.hpp"

// TODO refactor so that it handles ids and path lookups better

/**
 * Interface and cache for managing the installed fediy mods/apps
 */
class Mods {
protected:
    /* Design considerations
     * Need fast lookups for mods by path+id
     * Also need fast iteration over mods
     *Don't care about insertion time
     */
    RWMutex m_mtx; // TODO compare performance of just using std::mutex

    std::vector<Mod*> m_mods;
    std::unordered_map<std::string, Mod*> m_mods_lookup;
    // TODO maybe should add back the by_id to prevent collisions
    //      if static/reverse-proxy apps added

public:

    Mods() = default;
    ~Mods() {
        clear();
    }

    void find_modules();
    bool start_all();
    bool stop_all();

    void clear() {
        RWMutex::LockForWrite lock{m_mtx};
        for (auto* m : m_mods)
#ifndef FEDIY_DEBUG
            if (m != nullptr)
#endif
            delete m;
    }

    Mod* get_mod(const std::string& path) {
        RWMutex::LockForRead lock{m_mtx};
        auto ret = m_mods_lookup.find(path);
        if (ret != m_mods_lookup.end())
            return ret->second;
        else
            return nullptr;
    }

    inline std::vector<Mod*> get_mods() {
        RWMutex::LockForRead lock{m_mtx};
        return m_mods; // copy
    }

    /// Get json list of installed apps for the user portal
    [[nodiscard]] inline std::string get_mods_json() {
        std::string ret = "[";
        RWMutex::LockForRead lock{m_mtx};
        for (auto& m : m_mods) {
            ret += m->user_json();
            ret += ',';
        }
        if (ret[ret.size() - 1] == ',')
            ret[ret.size() - 1] = ']';
        return ret;
    }

    bool remove_mod(const std::string& id) {
        RWMutex::LockForWrite lock{m_mtx};
        auto m = get_mod(id);
        if (m == nullptr)
            return false;
        auto it = std::find(m_mods.begin(), m_mods.end(), m);
        m_mods.erase(it);
        m_mods_lookup.erase(m->m_path);
        m_mods_lookup.erase(m->m_id);
        return m->stop();
    }

    bool update_path(const std::string& old_path, const std::string& new_path) {
        RWMutex::LockForWrite lock{m_mtx};
        if (m_mods_lookup.contains(new_path))
            return false;
        auto m = (m_mods_lookup[new_path] = m_mods_lookup[old_path]);
        m->set_path(new_path);
        m_mods_lookup.erase(old_path);
        return true;
    }
};