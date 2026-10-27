#include "brepkit/stl.hpp"
#include <Poly_Triangulation.hxx>
#include <RWStl.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Face.hxx>

namespace brepkit
{
    TopoDS_Shape LoadStl(const std::filesystem::path &stl_path)
    {
        Handle(Poly_Triangulation) mesh =
            RWStl::ReadFile(stl_path.string().c_str());

        if (mesh.IsNull() || mesh->NbTriangles() == 0)
        {
            throw std::runtime_error("failed to read STL file");
        }

        TopoDS_Face face;
        BRep_Builder builder;
        builder.MakeFace(face, mesh);

        return face;
    }
} // namespace brepkit
