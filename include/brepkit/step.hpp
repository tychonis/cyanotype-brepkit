// step.hpp
#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <map>

#include <brepkit/type.hpp>

namespace brepkit
{
    struct ExportOptions
    {
        std::vector<TessellationProfile> profiles;
        const std::map<std::string, std::string> *digest_map = nullptr;
    };

    struct ExportedGlb
    {
        std::string profile;
        std::filesystem::path path; // relative to out_dir
    };

    struct ExportedNode
    {
        std::string id;          // stable enough for manifest/debug
        std::string parent_id;   // empty for roots
        std::string label_entry; // OCCT label entry string like 0:1:1:3
        bool is_assembly = false;
        std::vector<ExportedGlb> glbs;
    };

    struct ExportManifest
    {
        std::filesystem::path source_step;
        std::filesystem::path out_dir;
        std::vector<ExportedNode> nodes;
    };

    ExportManifest export_step_hierarchy_glbs(
        const std::filesystem::path &step_file,
        const std::filesystem::path &out_dir,
        const ExportOptions &opts);

    ExportManifest export_glb(
        const std::filesystem::path &step_file,
        const std::filesystem::path &out_dir,
        const ExportOptions &opts);

} // namespace brepkit
