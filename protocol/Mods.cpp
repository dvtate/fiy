#include <filesystem>

#include "FIY.hpp"

#include "Mods.hpp"

void Mods::find_modules() {
    const auto mods_dir = g_fiy->m_config.m_data_dir + "/mods";
    std::string fail_reason;

    // TODO sort alphabetically?
    for (auto& p : std::filesystem::directory_iterator(mods_dir))
        if (p.is_directory()) {
            auto&& id = p.path().filename().string();
            auto* m = new Mod(id);
            if (!m->m_enabled) {
                DEBUG_LOG("Mod" <<m->m_id <<" is disabled.");
                delete m;
                continue;
            }
            m_mods.emplace_back(m);
        }

    // m_mtx should be locked for write
    const size_t mod_count = m_mods.size();
    m_mods_lookup.reserve(mod_count);
    m_mods_by_id.reserve(mod_count);

    // Add lookup by id
    for (size_t i = 0; i < mod_count; i++)
        m_mods_by_id[m_mods[i]->m_id] = m_mods[i];

    // Add lookup by path (prevent overlap)
    for (size_t i = 0; i < mod_count; i++) {
        if (m_mods_lookup.contains(m_mods[i]->m_path)) {
            LOG("Duplicate module path " << m_mods[i]->m_path <<" for module " <<m_mods[i]->m_id << " ignored");
            const Mod* m = m_mods_lookup[m_mods[i]->m_path];
            LOG("Module path " << m_mods[i]->m_path <<" already used by module " <<m->m_id);
        } else {
            m_mods_lookup[m_mods[i]->m_path] = m_mods[i];
        }
    }

    LOG("Found " + std::to_string(m_mods.size()) + " mods");
}

bool Mods::start_all() const {
    bool ret = true;

    for (Mod* mod: m_mods) {
        if (!mod->m_loaded)
            continue;
        DEBUG_LOG("Starting module: " + mod->m_id + "...");
        if (!mod->start()) {
            LOG("Failed to start module " + mod->m_id <<".");
            ret = false;
        } else {
            LOG("Successfully started module " + mod->m_id <<".");
        }
    }
    return ret;
}


bool Mods::stop_all() const {
    bool ret = true;
    for (auto* mod : m_mods)
        if (mod->status() == Mod::Status::RUNNING)
            if (!mod->stop())
                ret = false;
    return ret;
}