// step_export.hpp
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace brepkit
{

    struct TessellationProfile
    {
        std::string name;          // e.g. "coarse", "medium", "fine"
        double linear_deflection;  // e.g. 1.0, 0.2, 0.05
        double angular_deflection; // radians, e.g. 0.5
        bool relative = false;
        bool parallel = true;
        bool merge_faces = true;
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
        const std::vector<TessellationProfile> &profiles);

} // namespace brepkit
