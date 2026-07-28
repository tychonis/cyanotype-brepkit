#pragma once

#include <filesystem>
#include <string>

namespace brepkit
{

    struct MeshStats
    {
        std::size_t face_count = 0;
        std::size_t triangulated_face_count = 0;
        std::size_t triangle_count = 0;
        std::size_t vertex_count = 0;
    };

    int run_inspect_step(const std::filesystem::path &step_path,
                         double linear_deflection,
                         double angular_deflection_rad);

} // namespace brepkit
