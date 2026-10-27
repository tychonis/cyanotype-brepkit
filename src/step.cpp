// step.cpp
#include "brepkit/step.hpp"
#include <STEPControl_Reader.hxx>

namespace brepkit
{
    TopoDS_Shape LoadStep(const std::filesystem::path &step_path)
    {
        STEPControl_Reader reader;

        const IFSelect_ReturnStatus status =
            reader.ReadFile(step_path.string().c_str());

        if (status != IFSelect_RetDone)
        {
            throw std::runtime_error("failed to read STEP file");
        }

        const int root_count = reader.NbRootsForTransfer();
        if (root_count <= 0)
        {
            throw std::runtime_error("STEP file contains no transferable roots");
        }

        for (int i = 1; i <= root_count; ++i)
        {
            reader.TransferRoot(i);
        }

        TopoDS_Shape shape = reader.OneShape();
        if (shape.IsNull())
        {
            throw std::runtime_error("STEP transfer succeeded but produced no shape");
        }

        return shape;
    }
} // namespace brepkit
