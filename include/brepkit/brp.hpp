#pragma once

#include <string>
#include <TopoDS_Shape.hxx>
#include <brepkit/type.hpp>

namespace brepkit
{
    TopoDS_Shape LoadBrep(const std::filesystem::path &path);

    Placement ReadTopLevelPlacement(const std::string &path);
    void ClearTopLevelPlacement(TopoDS_Shape &shape);
    std::string SerializePlacement(const Placement &p);
    std::size_t CountDistinctLocations(const std::string &path);
    void InspectLocations(const std::string &path);
    Placement Canonicalize(const std::string &input_path, const std::string &output_path);
    void BrpToGlb(const std::string &input_path, const std::string &output_path, const TessellationProfile &profile);
    void CysceneToGlb(const std::string &input_path, const std::string &output_path, const TessellationProfile &profile);
} // namespace brepkit
