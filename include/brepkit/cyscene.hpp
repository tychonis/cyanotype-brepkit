#pragma once

#include <string>
#include <TopoDS_Shape.hxx>
#include <brepkit/type.hpp>

namespace brepkit
{
    void CysceneToGlb(const std::string &input_path, const std::string &output_path, const TessellationProfile &profile);
}
