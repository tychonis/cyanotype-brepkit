#pragma once

#include <filesystem>
#include <string>
#include <brepkit/type.hpp>

namespace brepkit
{
    int InspectStep(const std::filesystem::path &step_path,
                    double linear_deflection,
                    double angular_deflection_rad);

} // namespace brepkit
