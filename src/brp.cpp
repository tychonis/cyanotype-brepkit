#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Quaternion.hxx>
#include <gp_Trsf.hxx>
#include <gp_XYZ.hxx>

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace brepkit
{
    struct Placement
    {
        double tx, ty, tz;
        double qx, qy, qz, qw;
    };

    std::string SerializePlacement(const Placement &p)
    {
        nlohmann::json j = {
            {"translation", {p.tx, p.ty, p.tz}},
            {"rotation", {p.qx, p.qy, p.qz, p.qw}},
        };

        return j.dump();
    }

    Placement ReadTopLevelPlacement(const std::string &path)
    {
        BRep_Builder builder;
        TopoDS_Shape shape;

        if (!BRepTools::Read(shape, path.c_str(), builder))
        {
            throw std::runtime_error("Failed to read BREP: " + path);
        }

        const gp_Trsf trsf = shape.Location().Transformation();
        const gp_XYZ t = trsf.TranslationPart();
        const gp_Quaternion q = trsf.GetRotation();

        return {
            .tx = t.X(),
            .ty = t.Y(),
            .tz = t.Z(),
            .qx = q.X(),
            .qy = q.Y(),
            .qz = q.Z(),
            .qw = q.W(),
        };
    }

    void ClearTopLevelPlacement(TopoDS_Shape &shape)
    {
        shape.Location(TopLoc_Location());
    }
} // namespace brepkit
