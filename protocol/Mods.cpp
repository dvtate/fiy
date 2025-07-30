#include <filesystem>

#include "App.hpp"

#include "Mods.hpp"

void Mods::find_modules() {
    auto apps_dir = g_app->m_config.m_data_dir + "/apps";
    std::string fail_reason;

    // TODO parallel
    for (auto& p : std::filesystem::directory_iterator(apps_dir))
        if (p.is_directory()) {
            auto&& id = p.path().filename().string();
            m_mods.emplace_back(new Mod(id));
        }

    // m_mtx should be locked for write
    auto mod_count = m_mods.size();
    m_mods_lookup.reserve(mod_count * 2);

    // Add lookup by id
    for (size_t i = 0; i < mod_count; i++)
        m_mods_lookup[m_mods[i]->m_id] = m_mods[i];

    // Add lookup by path (prevent overlap)
    for (size_t i = 0; i < mod_count; i++) {
        if (m_mods_lookup.contains(m_mods[i]->m_path)) {
            DEBUG_LOG("Duplicate module path " << m_mods[i]->m_path <<" for module " <<m_mods[i]->m_id << " ignored");
            auto m = m_mods_lookup[m_mods[i]->m_path];
            DEBUG_LOG("Module path " << m_mods[i]->m_path <<" already used by module " <<m->m_id);
        } else {
            m_mods_lookup[m_mods[i]->m_path] = m_mods[i];
        }
    }

    DEBUG_LOG("Found " + std::to_string(m_mods.size()) + " apps");
}

bool Mods::start_all() {
    bool ret = true;

    for (auto* mod: m_mods) {
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


bool Mods::stop_all() {
    bool ret = true;
    for (auto* mod : m_mods)
        if (mod->status() == Mod::Status::RUNNING)
            if (!mod->stop())
                ret = false;
    return ret;
}