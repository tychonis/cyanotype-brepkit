#pragma once

#include <filesystem>
#include <string>
#include <brepkit/type.hpp>

namespace brepkit
{
    TopoDS_Shape LoadShape(const std::filesystem::path &path);
    void ShapeToGlb(
        const TopoDS_Shape &input_shape,
        const std::string &output_path,
        const TessellationProfile &profile);
    void FileToGlb(
        const std::string &input_path,
        const std::string &output_path,
        const TessellationProfile &profile);
    void FileToJpeg(
        const std::string &input_path,
        const std::string &output_path);
    int InspectStep(const std::filesystem::path &step_path,
                    double linear_deflection,
                    double angular_deflection_rad);

} // namespace brepkit
