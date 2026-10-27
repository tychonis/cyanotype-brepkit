#pragma once

#include <string>
#include <filesystem>
#include <TopoDS_Shape.hxx>
#include <brepkit/type.hpp>

namespace brepkit
{
    TopoDS_Shape LoadBrep(const std::filesystem::path &path);
    Placement Canonicalize(const std::string &input_path, const std::string &output_path);

    std::string SerializePlacement(const Placement &p);
} // namespace brepkit
