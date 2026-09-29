#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Quaternion.hxx>
#include <gp_Trsf.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TDataStd_Name.hxx>
#include <RWGltf_CafWriter.hxx>

#include <brepkit/type.hpp>
#include <brepkit/brp.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <fstream>

namespace brepkit
{
    TopoDS_Shape LoadShape(const std::filesystem::path &path)
    {
        const std::string ext = path.extension().string();

        if (ext == ".brp" || ext == ".brep")
        {
            return LoadBrep(path);
        }

        // if (ext == ".step" || ext == ".stp")
        // {
        //     return LoadStep(path);
        // }

        throw std::runtime_error(
            "Unsupported shape format: " + ext);
    }

    void CysceneToGlb(
        const std::string &input_path,
        const std::string &output_path,
        const TessellationProfile &profile)
    {
        using json = nlohmann::json;

        std::ifstream stream(input_path);
        if (!stream)
        {
            throw std::runtime_error("Failed to read scene: " + input_path);
        }

        const json scene = json::parse(stream);

        Handle(TDocStd_Document) document =
            new TDocStd_Document("XmlXCAF");

        Handle(XCAFDoc_ShapeTool) shape_tool =
            XCAFDoc_DocumentTool::ShapeTool(document->Main());

        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);

        for (const auto &object : scene.at("objects"))
        {
            std::string source = object.at("source");

            constexpr std::string_view file_prefix = "file://";
            if (source.starts_with(file_prefix))
            {
                source.erase(0, file_prefix.size());
            }

            const std::filesystem::path source_path = source;
            TopoDS_Shape shape = LoadShape(source_path);

            const auto &placement = object.at("placement");
            const auto &r = placement.at("rotation");
            const auto &p = placement.at("position");

            gp_Quaternion rotation(
                r.at(0).get<double>(),
                r.at(1).get<double>(),
                r.at(2).get<double>(),
                r.at(3).get<double>());

            gp_Trsf transform;
            transform.SetRotation(rotation);
            transform.SetTranslationPart(gp_Vec(
                p.at(0).get<double>() * 1000.0,
                p.at(1).get<double>() * 1000.0,
                p.at(2).get<double>() * 1000.0));

            shape.Location(TopLoc_Location(transform));

            builder.Add(compound, shape);
        }

        BRepMesh_IncrementalMesh mesher(
            compound,
            profile.linear_deflection,
            profile.relative,
            profile.angular_deflection,
            profile.parallel);

        mesher.Perform();

        if (!mesher.IsDone())
        {
            throw std::runtime_error("Failed to tessellate scene");
        }

        const TDF_Label label = shape_tool->AddShape(compound, false);
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
} // namespace brepkit
