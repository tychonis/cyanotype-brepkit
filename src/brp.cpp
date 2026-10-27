#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Shape.hxx>
#include <NCollection_IndexedMap.hxx>
#include <TopExp.hxx>
#include <gp_Quaternion.hxx>
#include <gp_Trsf.hxx>
#include <gp_XYZ.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TDataStd_Name.hxx>
#include <RWGltf_CafWriter.hxx>

#include <brepkit/type.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace brepkit
{
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

    Placement Canonicalize(const std::string &input_path, const std::string &output_path)
    {
        BRep_Builder builder;
        TopoDS_Shape root;
        if (!BRepTools::Read(root, input_path.c_str(), builder))
        {
            throw std::runtime_error("Failed to read BREP: " + input_path);
        }

        const gp_Trsf trsf = root.Location().Transformation();
        const gp_XYZ t = trsf.TranslationPart();
        const gp_Quaternion q = trsf.GetRotation();

        const Placement ret = {
            .tx = t.X(),
            .ty = t.Y(),
            .tz = t.Z(),
            .qx = q.X(),
            .qy = q.Y(),
            .qz = q.Z(),
            .qw = q.W(),
        };
        root = root.Located(TopLoc_Location());

        if (!BRepTools::Write(root, output_path.c_str()))
        {
            throw std::runtime_error("Failed to write BREP: " + output_path);
        }

        return ret;
    }

    TopoDS_Shape LoadBrep(const std::filesystem::path &path)
    {
        BRep_Builder builder;
        TopoDS_Shape shape;

        if (!BRepTools::Read(shape, path.string().c_str(), builder))
        {
            throw std::runtime_error(
                "Failed to read BREP: " + path.string());
        }

        return shape;
    }
} // namespace brepkit
