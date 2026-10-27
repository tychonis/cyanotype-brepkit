// stl.hpp
#pragma once

#include <filesystem>
#include <TopoDS_Shape.hxx>

namespace brepkit
{
    TopoDS_Shape LoadStl(const std::filesystem::path &stl_path);
} // namespace brepkit
