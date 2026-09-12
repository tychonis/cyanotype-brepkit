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

#include <brepkit/step.hpp>
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

    void ClearTopLevelPlacement(TopoDS_Shape &shape)
    {
        shape.Location(TopLoc_Location());
    }

    std::size_t CountDistinctLocations(const std::string &path)
    {
        BRep_Builder builder;
        TopoDS_Shape root;
        if (!BRepTools::Read(root, path.c_str(), builder))
        {
            throw std::runtime_error("Failed to read BREP: " + path);
        }

        ShapeMap shapes;
        TopExp::MapShapes(root, shapes);

        std::vector<TopLoc_Location> locations;

        for (int i = 1; i <= shapes.Extent(); ++i)
        {
            const auto &loc = shapes(i).Location();

            bool found = false;
            for (const auto &existing : locations)
            {
                if (loc.IsEqual(existing))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
                locations.push_back(loc);
        }

        return locations.size();
    }

    void InspectLocations(const std::string &path)
    {
        BRep_Builder builder;
        TopoDS_Shape root;
        if (!BRepTools::Read(root, path.c_str(), builder))
        {
            throw std::runtime_error("Failed to read BREP: " + path);
        }

        ShapeMap shapes;
        TopExp::MapShapes(root, shapes);

        struct Group
        {
            TopLoc_Location location;
            int count = 0;
            int rootCount = 0;
            int identityCount = 0;
        };

        std::vector<Group> groups;

        for (int i = 1; i <= shapes.Extent(); ++i)
        {
            const TopoDS_Shape &shape = shapes(i);
            const TopLoc_Location &loc = shape.Location();

            auto it = std::find_if(
                groups.begin(),
                groups.end(),
                [&](const Group &g)
                { return loc.IsEqual(g.location); });

            if (it == groups.end())
            {
                groups.push_back({loc});
                it = std::prev(groups.end());
            }

            ++it->count;

            if (shape.IsSame(root))
                ++it->rootCount;

            if (loc.IsIdentity())
                ++it->identityCount;
        }

        for (std::size_t i = 0; i < groups.size(); ++i)
        {
            std::cout
                << "location " << i
                << ": count=" << groups[i].count
                << ", root=" << groups[i].rootCount
                << ", identity=" << groups[i].identityCount
                << '\n';
        }
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

    void BrpToGlb(
        const std::string &input_path,
        const std::string &output_path,
        const TessellationProfile &profile)
    {
        BRep_Builder builder;
        TopoDS_Shape shape;

        if (!BRepTools::Read(shape, input_path.c_str(), builder))
        {
            throw std::runtime_error("Failed to read BREP: " + input_path);
        }

        BRepMesh_IncrementalMesh mesher(
            shape,
            profile.linear_deflection,
            profile.relative,
            profile.angular_deflection,
            profile.parallel);

        mesher.Perform();

        if (!mesher.IsDone())
        {
            throw std::runtime_error("Failed to tessellate BREP: " + input_path);
        }

        Handle(TDocStd_Document) document =
            new TDocStd_Document("XmlXCAF");

        Handle(XCAFDoc_ShapeTool) shape_tool =
            XCAFDoc_DocumentTool::ShapeTool(document->Main());

        const TDF_Label label = shape_tool->AddShape(shape, false);
        TDataStd_Name::Set(label, "shape");

        RWGltf_CafWriter writer(output_path.c_str(), true); // true = binary GLB
        writer.SetMergeFaces(profile.merge_faces);
        auto &converter = writer.ChangeCoordinateSystemConverter();
        converter.SetInputLengthUnit(0.001); // millimeters

        NCollection_IndexedDataMap<TCollection_AsciiString, TCollection_AsciiString> metadata;

        if (!writer.Perform(
                document,
                metadata,
                Message_ProgressRange()))
        {
            throw std::runtime_error("Failed to write GLB: " + output_path);
        }
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

        for (const auto &object : scene.at("objects"))
        {
            std::string source = object.at("source");

            constexpr std::string_view file_prefix = "file://";
            if (source.starts_with(file_prefix))
            {
                source.erase(0, file_prefix.size());
            }

            const std::filesystem::path brep_path = source;

            BRep_Builder builder;
            TopoDS_Shape shape;

            if (!BRepTools::Read(shape, brep_path.string().c_str(), builder))
            {
                throw std::runtime_error(
                    "Failed to read BREP: " + brep_path.string());
            }

            BRepMesh_IncrementalMesh mesher(
                shape,
                profile.linear_deflection,
                profile.relative,
                profile.angular_deflection,
                profile.parallel);

            mesher.Perform();

            if (!mesher.IsDone())
            {
                throw std::runtime_error(
                    "Failed to tessellate BREP: " + brep_path.string());
            }

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

            const TDF_Label label = shape_tool->AddShape(shape, false);
            TDataStd_Name::Set(
                label,
                brep_path.filename().string().c_str());
        }

        RWGltf_CafWriter writer(output_path.c_str(), true);
        writer.SetMergeFaces(profile.merge_faces);

        auto &converter = writer.ChangeCoordinateSystemConverter();
        converter.SetInputLengthUnit(0.001); // BREP/document coordinates are mm

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
