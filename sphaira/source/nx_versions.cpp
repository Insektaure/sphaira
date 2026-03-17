#include "nx_versions.hpp"
#include "ui/progress_box.hpp"
#include "option.hpp"
#include "log.hpp"

#include <cstring>
#include <charconv>
#include <algorithm>
#include <ctime>

namespace sphaira::nx_versions {
namespace {

constexpr const char* DEFAULT_VERSIONS_URL = "https://raw.githubusercontent.com/16BitWonder/nx-versions/master/versions.txt";
constexpr fs::FsPath CACHE_PATH{"/switch/sphaira/cache/nx_versions.txt"};
constexpr s64 CACHE_MAX_AGE = 7 * 24 * 60 * 60; // 1 week

option::OptionString s_versions_url{"nx_versions", "url", DEFAULT_VERSIONS_URL};

} // namespace

auto Parse(const std::vector<u8>& data) -> VersionMap {
    VersionMap versions;

    if (data.size() < 12) {
        return versions;
    }

    // format: 12-byte header, then entries of [16-char hex TID][separator][version digits]\r\n
    const auto* buf = reinterpret_cast<const char*>(data.data());
    const auto size = static_cast<long>(data.size());
    long index = 12;

    while (index + 16 < size) {
        // check if entry is valid (bytes at pos 13,14,15 not all zero)
        if (buf[index + 13] == 0 && buf[index + 14] == 0 && buf[index + 15] == 0) {
            index += 17;
            while (index < size && buf[index] != '\r')
                index++;
            index += 2;
            continue;
        }

        // parse TID: 16 hex chars -> u64
        char tid_str[17]{};
        std::memcpy(tid_str, buf + index, 16);
        tid_str[16] = '\0';
        index += 17;

        u64 tid = 0;
        auto [ptr, ec] = std::from_chars(tid_str, tid_str + 16, tid, 16);
        if (ec != std::errc{}) {
            while (index < size && buf[index] != '\r')
                index++;
            index += 2;
            continue;
        }

        // parse version: decimal digits until \r
        u32 version = 0;
        while (index < size && buf[index] != '\r') {
            version = version * 10 + (buf[index] - '0');
            index++;
        }
        index += 2; // skip \r\n

        versions[tid] = version;
    }

    return versions;
}

auto GetMissing(u64 app_id, const VersionMap& versions, const std::unordered_map<u64, u32>& installed) -> std::vector<MissingEntry> {
    std::vector<MissingEntry> missing;

    const u64 update_tid = (app_id & ~0xFFFULL) | 0x800ULL;
    const u64 dlc_base = (app_id & ~0xFFFFULL) + 0x1000ULL;

    // check for missing/outdated update
    auto it = versions.find(update_tid);
    if (it != versions.end()) {
        auto inst_it = installed.find(update_tid);
        if (inst_it == installed.end() || it->second > inst_it->second) {
            missing.push_back({update_tid, it->second, NcmContentMetaType_Patch});
        }
    }

    // check for missing DLC
    for (const auto& [tid, ver] : versions) {
        if ((tid & ~0xFFFULL) == dlc_base) {
            if (installed.find(tid) == installed.end()) {
                missing.push_back({tid, ver, NcmContentMetaType_AddOnContent});
            }
        }
    }

    std::sort(missing.begin(), missing.end(), [](const auto& a, const auto& b) {
        return a.application_id < b.application_id;
    });

    return missing;
}

auto Load() -> VersionMap {
    std::vector<u8> data;
    if (R_SUCCEEDED(fs::FsStdio().read_entire_file(CACHE_PATH, data))) {
        return Parse(data);
    }
    return {};
}

auto IsCacheStale() -> bool {
    FsTimeStampRaw ts{};
    if (R_FAILED(fs::GetFileTimeStampRaw(CACHE_PATH, &ts))) {
        return true;
    }
    const auto now = std::time(nullptr);
    return now - ts.modified >= CACHE_MAX_AGE;
}

auto Download(ui::ProgressBox* pbox) -> Result {
    pbox->NewTransfer("versions.txt");

    const auto url = s_versions_url.Get();
    const auto result = curl::Api().ToFile(
        curl::Url{url},
        curl::Path{CACHE_PATH},
        curl::OnProgress{pbox->OnDownloadProgressCallback()}
    );

    R_UNLESS(result.success, 1);
    R_SUCCEED();
}

} // namespace sphaira::nx_versions
