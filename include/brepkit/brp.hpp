#pragma once

#include <string>
#include <TopoDS_Shape.hxx>

namespace brepkit
{
    struct Placement
    {
        double tx, ty, tz;
        double qx, qy, qz, qw;
    };

    Placement ReadTopLevelPlacement(const std::string &path);
    void ClearTopLevelPlacement(TopoDS_Shape &shape);
    std::string SerializePlacement(const Placement &p);
    std::size_t CountDistinctLocations(const std::string &path);
    void InspectLocations(const std::string &path);
    Placement Canonicalize(const std::string &input_path, const std::string &output_path);
} // namespace brepkit
