#pragma once

#include <string>
#include <TopoDS_Shape.hxx>
#include <brepkit/type.hpp>

namespace brepkit
{
    TopoDS_Shape LoadCyscene(const std::filesystem::path &input_path);
}
