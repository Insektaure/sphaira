#pragma once

#include "fs.hpp"
#include "download.hpp"
#include <vector>
#include <unordered_map>
#include <switch.h>

namespace sphaira::ui { struct ProgressBox; }

namespace sphaira::nx_versions {

struct MissingEntry {
    u64 application_id{};
    u32 version{};
    u8 meta_type{}; // NcmContentMetaType_Patch or NcmContentMetaType_AddOnContent
};

using VersionMap = std::unordered_map<u64, u32>;

// parse versions.txt data into a TID -> version map.
auto Parse(const std::vector<u8>& data) -> VersionMap;

// find missing updates/DLC for a given app_id.
// installed: map of TID -> version for locally installed content.
auto GetMissing(u64 app_id, const VersionMap& versions, const std::unordered_map<u64, u32>& installed) -> std::vector<MissingEntry>;

// load versions.txt from local cache. returns empty map if no cache exists.
auto Load() -> VersionMap;

// returns true if the cache file is older than 1 week or doesn't exist.
auto IsCacheStale() -> bool;

// download versions.txt synchronously. meant to be called from a ProgressBox thread.
// returns success/failure result.
auto Download(ui::ProgressBox* pbox) -> Result;

} // namespace sphaira::nx_versions
