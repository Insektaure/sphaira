#pragma once

#include <switch.h>
#include <string>
#include <vector>
#include "ui/progress_box.hpp"

namespace sphaira {

enum class ForwarderAddressSpace : u8 {
    Bit36 = 1,
    Bit39 = 3,
};

enum class ForwarderSvcDebugMode : u8 {
    Automatic,
    Enabled,
    Disabled,
};

struct OwoConfig {
    std::string nro_path;
    std::string args{};
    std::string name{};
    std::string author{};
    NacpStruct nacp;
    std::vector<u8> icon;
    std::vector<u8> logo;
    std::vector<u8> gif;
    bool profile_selection{};
    ForwarderAddressSpace address_space{ForwarderAddressSpace::Bit36};
    bool screenshot{true};
    bool video_capture{true};
    ForwarderSvcDebugMode svc_debug_mode{ForwarderSvcDebugMode::Automatic};

    std::vector<u8> program_nca{};
};

auto install_forwarder(OwoConfig& config, NcmStorageId storage_id) -> Result;
auto install_forwarder(ui::ProgressBox* pbox, OwoConfig& config, NcmStorageId storage_id) -> Result;

} // namespace sphaira
