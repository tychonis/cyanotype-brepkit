#pragma once

#include <filesystem>
#include <string>
#include <brepkit/type.hpp>

namespace brepkit
{
    int run_inspect_step(const std::filesystem::path &step_path,
                         double linear_deflection,
                         double angular_deflection_rad);

} // namespace brepkit
