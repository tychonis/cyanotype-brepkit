#include "brepkit/mesh.hpp"
#include "brepkit/brp.hpp"
#include "brepkit/step.hpp"
#include "brepkit/cyscene.hpp"
#include "nlohmann/json.hpp"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Poly_Triangulation.hxx>
#include <STEPControl_Reader.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopLoc_Location.hxx>
#include <BRep_Builder.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TDataStd_Name.hxx>
#include <RWGltf_CafWriter.hxx>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace brepkit
{
    TopoDS_Shape LoadShape(const std::filesystem::path &path)
    {
        const std::string ext = path.extension().string();

        if (ext == ".brp" || ext == ".brep")
        {
            return LoadBrep(path);
        }

        if (ext == ".step" || ext == ".stp")
        {
            return LoadStep(path);
        }

        if (ext == ".cyscene")
        {
            return LoadCyscene(path);
        }

        throw std::runtime_error(
            "Unsupported shape format: " + ext);
    }

    MeshStats GetMeshStats(const TopoDS_Shape &shape,
                           double linear_deflection,
                           double angular_deflection_rad)
    {
        // relative = false, parallel = true
        BRepMesh_IncrementalMesh mesher(
            shape,
            linear_deflection,
            false,
            angular_deflection_rad,
            true);

        MeshStats stats{};

        for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next())
        {
            ++stats.face_count;

            const TopoDS_Face face = TopoDS::Face(ex.Current());
            TopLoc_Location loc;
            const Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);

            if (tri.IsNull())
            {
                continue;
            }

            ++stats.triangulated_face_count;
            stats.triangle_count += static_cast<std::size_t>(tri->NbTriangles());
            stats.vertex_count += static_cast<std::size_t>(tri->NbNodes());
        }

        return stats;
    }

    void ShapeToGlb(
        const TopoDS_Shape &input_shape,
        const std::string &output_path,
        const TessellationProfile &profile)
    {
        BRepMesh_IncrementalMesh mesher(
            input_shape,
            profile.linear_deflection,
            profile.relative,
            profile.angular_deflection,
            profile.parallel);

        mesher.Perform();

        if (!mesher.IsDone())
        {
            throw std::runtime_error("Failed to tessellate scene");
        }

        Handle(TDocStd_Document) document =
            new TDocStd_Document("XmlXCAF");
        Handle(XCAFDoc_ShapeTool) shape_tool =
            XCAFDoc_DocumentTool::ShapeTool(document->Main());
        const TDF_Label label = shape_tool->AddShape(input_shape, false);
        TDataStd_Name::Set(label, "scene");

        RWGltf_CafWriter writer(output_path.c_str(), true);
        writer.SetMergeFaces(profile.merge_faces);

        auto &converter = writer.ChangeCoordinateSystemConverter();
        converter.SetInputLengthUnit(0.001);

        NCollection_IndexedDataMap<
            TCollection_AsciiString,
            TCollection_AsciiString>
            metadata;

        if (!writer.Perform(
                document,
                metadata,
                Message_ProgressRange()))
        {
            throw std::runtime_error(
                "Failed to write GLB: " + output_path);
        }
    }

    void FileToGlb(
        const std::string &input_path,
        const std::string &output_path,
        const TessellationProfile &profile)
    {
        TopoDS_Shape compound = LoadShape(input_path);
        ShapeToGlb(compound, output_path, profile);
    }

    int InspectStep(const std::filesystem::path &step_path,
                    double linear_deflection,
                    double angular_deflection_rad)
    {
        try
        {
            const TopoDS_Shape shape = LoadStep(step_path);
            const MeshStats stats = GetMeshStats(shape, linear_deflection, angular_deflection_rad);

            NCollection_Sequence<TCollection_AsciiString> lengthUnits;
            NCollection_Sequence<TCollection_AsciiString> angleUnits;
            NCollection_Sequence<TCollection_AsciiString> solidAngleUnits;
            STEPControl_Reader reader;
            reader.ReadFile(step_path.string().c_str());

            reader.FileUnits(lengthUnits, angleUnits, solidAngleUnits);
            std::string unit = lengthUnits.Value(1).ToCString();

            std::cout << "file: " << step_path << "\n";
            std::cout << "faces: " << stats.face_count << "\n";
            std::cout << "triangulated_faces: " << stats.triangulated_face_count << "\n";
            std::cout << "triangles: " << stats.triangle_count << "\n";
            std::cout << "vertices: " << stats.vertex_count << "\n";
            std::cout << "unit: " << unit << "\n";

            return EXIT_SUCCESS;
        }
        catch (const std::exception &e)
        {
            std::cerr << "error: " << e.what() << "\n";
            return EXIT_FAILURE;
        }
    }

} // namespace brepkit
