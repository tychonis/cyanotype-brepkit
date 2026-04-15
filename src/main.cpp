#include "brepkit/mesh.hxx"
#include "brepkit/step_export.hxx"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{

    void print_usage(const char *argv0)
    {
        std::cerr
            << "Usage:\n"
            << "  " << argv0 << " inspect <input.step|input.stp>\n"
            << "  " << argv0 << " export  <input.step|input.stp> <out-dir>\n";
    }

    std::vector<brepkit::TessellationProfile> default_profiles()
    {
        return {
            {
                .name = "coarse",
                .linear_deflection = 1.0,
                .angular_deflection = 0.7,
                .relative = false,
                .parallel = true,
                .merge_faces = true,
            },
            {
                .name = "medium",
                .linear_deflection = 0.2,
                .angular_deflection = 0.5,
                .relative = false,
                .parallel = true,
                .merge_faces = true,
            },
            {
                .name = "fine",
                .linear_deflection = 0.05,
                .angular_deflection = 0.35,
                .relative = false,
                .parallel = true,
                .merge_faces = true,
            },
        };
    }

} // namespace

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::string_view cmd = argv[1];

    if (cmd == "inspect")
    {
        const std::filesystem::path path = argv[2];
        const double linear_deflection = (argc >= 4) ? std::stod(argv[3]) : 0.1;
        const double angular_deflection_rad = (argc >= 5) ? std::stod(argv[4]) : 0.5;

        return brepkit::run_inspect_step(path, linear_deflection, angular_deflection_rad);
    }

    print_usage(argv[0]);
    return EXIT_FAILURE;
}
