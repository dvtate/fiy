//
// Created by tate on 12/8/25.
//

#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <boost/unordered/unordered_flat_map.hpp>

#include "../../../../modlib/fiymod.hpp"

#include "LocalRepo.hpp"
#include "../../../../util/RWMutex.hpp"
#include "../../../../util/LRUCache.hpp"
//
// class Repos {
//     RWMutex m_mtx;
//
//     LRUCache<std::string, std::shared_ptr<LocalRepo>, boost::unordered_flat_map> m_repos;
//
// public:
//     Repos();
//
//     std::shared_ptr<LocalRepo> local_repo(const BasicRepo& repo) {
//         // Check cache
//         const auto rp = repo.path();
//         m_mtx.read_lock();
//         auto ret = m_repos.get(rp, nullptr);
//         if (ret != nullptr) {
//             m_mtx.read_unlock();
//             return ret;
//         }
//
//         // Add it to the cache
//         ret = std::shared_ptr<LocalRepo>(
//             static_cast<LocalRepo*>(malloc(sizeof(LocalRepo))),
//             [](LocalRepo* ptr) {
//                 ptr->~LocalRepo();
//                 free(ptr);
//             }
//         );
//         m_mtx.read_to_write();
//         m_repos.set(rp, ret);
//         m_mtx.write_unlock();
//
//         // Do the actual construction outside the mutex
//         ::new(&*ret) LocalRepo(repo);
//         return ret;
//     }
//
//     std::vector<std::shared_ptr<LocalRepo>> local_repos(const std::vector<BasicRepo>& repos) {
//         std::vector<std::shared_ptr<LocalRepo>> ret;
//         ret.reserve(repos.size());
//
//         RWMutex::LockForWrite lock{m_mtx};
//         for (const auto& repo : repos) {
//             auto r = m_repos.get(repo.path(), nullptr);
//             if (r != nullptr)
//                 ret.push_back(r);
//             else
//                 ret.emplace_back(new LocalRepo(repo));
//         }
//         return ret;
//     }
//
//     static Repos cache;
// };