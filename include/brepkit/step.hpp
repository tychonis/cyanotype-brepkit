// step.hpp
#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <map>

#include <brepkit/type.hpp>

namespace brepkit
{
    TopoDS_Shape LoadStep(const std::filesystem::path &step_path);
} // namespace brepkit
