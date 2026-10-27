#include "brepkit/mesh.hpp"
#include "brepkit/brp.hpp"
#include "brepkit/step.hpp"
#include "brepkit/stl.hpp"
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
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <Image_AlienPixMap.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

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

        if (ext == ".stl")
        {
            return LoadStl(path);
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

    void ShapeToPpm(
        const TopoDS_Shape &shape,
        const std::string &output_path,
        int width = 512,
        int height = 288)
    {
        Handle(Aspect_DisplayConnection) display =
            new Aspect_DisplayConnection();

        Handle(OpenGl_GraphicDriver) driver =
            new OpenGl_GraphicDriver(display);

        Handle(V3d_Viewer) viewer =
            new V3d_Viewer(driver);

        Handle(V3d_View) view = viewer->CreateView();

        Handle(Aspect_NeutralWindow) window =
            new Aspect_NeutralWindow();

        window->SetSize(width, height);
        view->SetWindow(window);

        Handle(AIS_InteractiveContext) context =
            new AIS_InteractiveContext(viewer);

        Handle(AIS_Shape) ais_shape = new AIS_Shape(shape);
        context->Display(ais_shape, false);
        context->SetDisplayMode(ais_shape, AIS_Shaded, false);

        view->SetProj(-1.0, 1.0, 1.0);
        view->SetUp(0.0, 1.0, 0.0);

        view->SetBackgroundColor(Quantity_NOC_WHITE);
        view->FitAll(0.05);

        Image_AlienPixMap image;
        if (!view->ToPixMap(image, width, height))
        {
            throw std::runtime_error("Failed to render shape");
        }

        if (!image.Save(output_path.c_str()))
        {
            throw std::runtime_error(
                "Failed to save PPM: " + output_path);
        }
    }

    void FileToPpm(
        const std::string &input_path,
        const std::string &output_path)
    {
        TopoDS_Shape shape = LoadShape(input_path);
        ShapeToPpm(shape, output_path);
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
