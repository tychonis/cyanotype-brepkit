#include "brepkit/mesh.hpp"
#include "brepkit/step.hpp"
#include "brepkit/brp.hpp"
#include "brepkit/cyscene.hpp"
#include "cyanotype/load.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
    void PrintUsage(const char *argv0)
    {
        std::cerr
            << "Usage:\n"
            << "  " << argv0 << " canonicalize <input> <output>\n"
            << "  " << argv0 << " glb <input> <output>\n";
    }

    std::vector<brepkit::TessellationProfile> GetDefaultProfiles()
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

int CmdCanonicalize(int argc, char **argv)
{
    if (argc != 4)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::filesystem::path input_path = argv[2];
    const std::filesystem::path output_path = argv[3];

    try
    {
        brepkit::Placement placement = brepkit::Canonicalize(input_path.string(), output_path.string());
        std::cout << brepkit::SerializePlacement(placement) << "\n";

        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}

int CmdGlb(int argc, char **argv)
{
    if (argc < 4)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::string input_file = argv[2];
    const std::string output_file = argv[3];

    try
    {
        brepkit::FileToGlb(
            input_file, output_file, GetDefaultProfiles()[0]);
        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}

int CmdPpm(int argc, char **argv)
{
    if (argc < 4)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::string input_file = argv[2];
    const std::string output_file = argv[3];

    try
    {
        brepkit::FileToPpm(input_file, output_file);
        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    try
    {
        const std::string_view cmd = argv[1];
        const std::filesystem::path input = argv[2];

        if (cmd == "canonicalize")
        {
            return CmdCanonicalize(argc, argv);
        }

        if (cmd == "glb")
        {
            return CmdGlb(argc, argv);
        }

        if (cmd == "ppm")
        {
            return CmdPpm(argc, argv);
        }

        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }
    catch (const std::exception &e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
