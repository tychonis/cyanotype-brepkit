#pragma once

#include <string>

#include <NCollection_IndexedMap.hxx>
#include <TopoDS_Shape.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <NCollection_Sequence.hxx>
#include <TDF_Label.hxx>

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

    struct MeshStats
    {
        std::size_t face_count = 0;
        std::size_t triangulated_face_count = 0;
        std::size_t triangle_count = 0;
        std::size_t vertex_count = 0;
    };

    struct Placement
    {
        double tx, ty, tz;
        double qx, qy, qz, qw;
    };

    using ShapeMap =
        NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher>;
    using OcctString = TCollection_AsciiString;
    using StringMap = NCollection_IndexedDataMap<OcctString, OcctString>;
    using LabelSeq = NCollection_Sequence<TDF_Label>;
} // namespace brepkit
