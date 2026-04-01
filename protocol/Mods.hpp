#pragma once

#include "Mod.hpp"

#include <boost/unordered/unordered_flat_map.hpp>

#include <unordered_map>

// TODO refactor so that it handles ids and path lookups better

/**
 * Interface and cache for managing the installed FIY mods/apps
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
    boost::unordered_flat_map<std::string, Mod*> m_mods_lookup; // path -> mod
    boost::unordered_flat_map<std::string, Mod*> m_mods_by_id; // id -> mod

    // TODO instead just look up by id (must include in requests)
    //  and then dynamic cast into modnetconnector
    std::unordered_map<std::string, ModNetConnector*> m_net_connectors; // mod http bearer tokens

public:

    Mods() = default;
    ~Mods() {
        clear();
    }

    void find_modules();
    bool start_all() const;
    bool stop_all() const;

    void clear() {
        RWMutex::LockForWrite lock{m_mtx};
        for (auto* m : m_mods)
#ifndef FIY_DEBUG
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

    Mod* get_mod_by_id(const std::string& id) {
        RWMutex::LockForRead lock{m_mtx};
        auto ret = m_mods_by_id.find(id);
        if (ret != m_mods_by_id.end())
            return ret->second;
        else
            return nullptr;
    }

    std::vector<Mod*> get_mods() {
        RWMutex::LockForRead lock{m_mtx};
        return m_mods; // copy
    }

    /// Get JSON list of installed apps for the user portal
    [[nodiscard]] std::string get_mods_json() {
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
        // Find mod
        m_mtx.read_lock();
        const auto ret = m_mods_by_id.find(id);
        if (ret == m_mods_by_id.end()) {
            m_mtx.read_unlock();
            return false;
        }
        Mod* m = ret->second;

        // Remove mod
        m_mtx.read_to_write();
        m_mods.erase(std::ranges::find(m_mods, m));
        m_mods_lookup.erase(m->m_path);
        m_mods_by_id.erase(m->m_id);
        m_mtx.write_unlock();
        // TODO probably should delete the mod to prevent memory leak
        return m->stop();
    }

    bool add_net_connector(const std::string& token, ModNetConnector* connector) {
        RWMutex::LockForWrite lock{m_mtx};
        if (m_net_connectors.contains(token))
            return false;
        m_net_connectors[token] = connector;
        return true;
    }
    ModNetConnector*& get_net_connector(const std::string& token) {
        RWMutex::LockForRead lock{m_mtx};
        return m_net_connectors[token];
    }
    bool has_net_connector(const std::string& token) {
        RWMutex::LockForRead lock{m_mtx};
        return m_net_connectors.contains(token);
    }
    void remove_net_connector(const std::string& token) {
        RWMutex::LockForWrite lock{m_mtx};
        m_net_connectors.erase(token);
    }
};