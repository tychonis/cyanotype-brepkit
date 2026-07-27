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
} // namespace brepkit
