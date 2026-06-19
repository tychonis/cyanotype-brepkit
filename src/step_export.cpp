// step_export.cpp
#include "brepkit/step_export.hpp"

#include <BRepMesh_IncrementalMesh.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <RWGltf_CafWriter.hxx>
#include <RWGltf_WriterTrsfFormat.hxx>
#include <NCollection_IndexedDataMap.hxx>
#include <NCollection_Sequence.hxx>
#include <TCollection_AsciiString.hxx>
#include <TDF_Label.hxx>
#include <TDF_Tool.hxx>
#include <TDocStd_Document.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TDataStd_Name.hxx>

#include <stdexcept>

namespace
{
    Handle(XCAFDoc_ShapeTool) g_shape_tool;
}

namespace brepkit
{
    namespace
    {
        using OcctString = TCollection_AsciiString;
        using StringMap = NCollection_IndexedDataMap<OcctString, OcctString>;
        using LabelSeq = NCollection_Sequence<TDF_Label>;

        struct WalkCtx
        {
            Handle(TDocStd_Document) source_doc;
            Handle(XCAFDoc_ShapeTool) shape_tool;
            std::filesystem::path out_dir;
            std::vector<TessellationProfile> profiles;
            ExportManifest manifest;
        };

        std::string label_name(const TDF_Label &label)
        {
            Handle(TDataStd_Name) nameAttr;
            if (!label.FindAttribute(TDataStd_Name::GetID(), nameAttr))
                return "";

            const TCollection_ExtendedString &name = nameAttr->Get();

            std::vector<char> buf(name.LengthOfCString() + 1, '\0');
            Standard_PCharacter p = buf.data();
            name.ToUTF8CString(p);

            return std::string(buf.data());
        }

        std::string effective_label_name(
            const TDF_Label &label)
        {
            TDF_Label referred;
            if (g_shape_tool->GetReferredShape(label, referred))
            {
                return label_name(referred);
            }

            return "";
        }

        std::string label_entry_string(const TDF_Label &label)
        {
            OcctString entry;
            TDF_Tool::Entry(label, entry);
            std::cout << entry.ToCString() << "--" << label_name(label)
                      << "--" << effective_label_name(label) << "\n";
            return entry.ToCString();
        }

        std::string sanitize_id(std::string s)
        {
            for (char &c : s)
            {
                if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_'))
                {
                    c = '_';
                }
            }
            return s;
        }

        std::string node_id_from_label(const TDF_Label &label)
        {
            return label_entry_string(label);
        }

        Handle(TDocStd_Document) make_xde_document()
        {
            Handle(TDocStd_Document) doc;
            Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
            app->NewDocument("BinXCAF", doc);
            return doc;
        }

        Handle(TDocStd_Document) load_step_xde(const std::filesystem::path &step_file)
        {
            STEPCAFControl_Reader reader;
            const IFSelect_ReturnStatus status = reader.ReadFile(step_file.string().c_str());
            if (status != IFSelect_RetDone)
            {
                throw std::runtime_error("failed to read STEP file");
            }

            Handle(TDocStd_Document) doc = make_xde_document();
            if (!reader.Transfer(doc))
            {
                throw std::runtime_error("failed to transfer STEP into XDE document");
            }
            return doc;
        }

        void mesh_shape(const TopoDS_Shape &shape, const TessellationProfile &profile)
        {
            BRepMesh_IncrementalMesh mesher(
                shape,
                profile.linear_deflection,
                profile.relative,
                profile.angular_deflection,
                profile.parallel);
            (void)mesher;
        }

        void export_single_shape_glb(
            const TopoDS_Shape &shape,
            const std::filesystem::path &glb_path,
            const TessellationProfile &profile)
        {
            mesh_shape(shape, profile);

            Handle(TDocStd_Document) temp_doc = make_xde_document();
            Handle(XCAFDoc_ShapeTool) temp_shapes = XCAFDoc_DocumentTool::ShapeTool(temp_doc->Main());
            g_shape_tool = temp_shapes;

            // Treat compound as a whole here; this avoids unexpectedly exploding structure
            // inside each per-node export.
            TDF_Label root = temp_shapes->AddShape(shape, false, true);
            (void)root;

            RWGltf_CafWriter writer(TCollection_AsciiString(glb_path.string().c_str()), true);
            writer.SetMergeFaces(profile.merge_faces);
            writer.SetParallel(profile.parallel);
            writer.SetTransformationFormat(RWGltf_WriterTrsfFormat_Compact);

            StringMap file_info;
            file_info.Add("generator", "brepkit");

            if (!writer.Perform(temp_doc, file_info, Message_ProgressRange()))
            {
                throw std::runtime_error("failed to write GLB: " + glb_path.string());
            }
        }

        std::filesystem::path get_export_path(
            const TessellationProfile &profile,
            const std::string &filename)
        {
            return std::filesystem::path("meshes") /
                   profile.name /
                   (filename + ".glb");
        }

        void walk_label(
            WalkCtx &ctx,
            const TDF_Label &label,
            const std::string &parent_id,
            const std::map<std::string, std::string> *digest_map)
        {
            const std::string node_id = node_id_from_label(label);
            const bool is_assembly = ctx.shape_tool->IsAssembly(label);
            const TopoDS_Shape shape = XCAFDoc_ShapeTool::GetShape(label);

            ExportedNode node;
            node.id = node_id;
            node.parent_id = parent_id;
            node.label_entry = label_entry_string(label);
            node.is_assembly = is_assembly;

            if (!shape.IsNull())
            {
                for (const TessellationProfile &profile : ctx.profiles)
                {
                    std::string filename = sanitize_id(node_id);
                    if (digest_map != nullptr)
                    {
                        auto it = digest_map->find(node_id);
                        if (it != digest_map->end())
                        {
                            filename = it->second;
                        }
                    }
                    const std::filesystem::path rel = get_export_path(profile, filename);
                    const std::filesystem::path abs = ctx.out_dir / rel;

                    std::filesystem::create_directories(abs.parent_path());
                    export_single_shape_glb(shape, abs, profile);

                    node.glbs.push_back(ExportedGlb{
                        .profile = profile.name,
                        .path = rel});
                }
            }

            ctx.manifest.nodes.push_back(std::move(node));

            LabelSeq children;
            if (XCAFDoc_ShapeTool::GetComponents(label, children, false))
            {
                for (int i = 1; i <= children.Length(); ++i)
                {
                    walk_label(ctx, children.Value(i), node_id, digest_map);
                }
            }
        }

    } // namespace

    ExportManifest export_step_hierarchy_glbs(
        const std::filesystem::path &step_file,
        const std::filesystem::path &out_dir,
        const ExportOptions &opts)
    {
        if (opts.profiles.empty())
        {
            throw std::invalid_argument("profiles must not be empty");
        }

        std::filesystem::create_directories(out_dir);

        Handle(TDocStd_Document) doc = load_step_xde(step_file);
        Handle(XCAFDoc_ShapeTool) shape_tool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());

        WalkCtx ctx{
            .source_doc = doc,
            .shape_tool = shape_tool,
            .out_dir = out_dir,
            .profiles = opts.profiles,
            .manifest = ExportManifest{
                .source_step = step_file,
                .out_dir = out_dir,
                .nodes = {}}};

        LabelSeq free_shapes;
        shape_tool->GetFreeShapes(free_shapes);

        for (int i = 1; i <= free_shapes.Length(); ++i)
        {
            walk_label(ctx, free_shapes.Value(i), "", opts.digest_map);
        }

        return ctx.manifest;
    }

} // namespace brepkit
