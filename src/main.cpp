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
            << "  " << argv0 << " inspect <input.step|input.stp>\n"
            << "  " << argv0 << " export  <input.step|input.stp> <out-dir>\n";
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

    struct options
    {
        std::optional<std::string> map_file;
    };

    options ParseArgs(int argc, char **argv)
    {
        options opts;

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];

            if (arg == "--map")
            {
                if (i + 1 >= argc)
                {
                    throw std::runtime_error("--map requires a file path");
                }

                opts.map_file = argv[++i];
            }
        }

        return opts;
    }

} // namespace

int CmdInspect(int argc, char **argv)
{
    if (argc != 3)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::filesystem::path path = argv[2];
    const double linear_deflection = (argc >= 4) ? std::stod(argv[3]) : 0.1;
    const double angular_deflection_rad = (argc >= 5) ? std::stod(argv[4]) : 0.5;

    return brepkit::InspectStep(path, linear_deflection, angular_deflection_rad);
}

int CmdExportAll(int argc, char **argv)
{
    if (argc < 4)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }
    const std::filesystem::path out_dir = argv[3];

    brepkit::ExportOptions exportOpts = {
        .profiles = GetDefaultProfiles()};

    auto opts = ParseArgs(argc, argv);

    std::map<std::string, std::string> digest_map_storage;
    if (opts.map_file)
    {
        digest_map_storage = cyanotype::load_digest_map(*opts.map_file);
        exportOpts.digest_map = &digest_map_storage;
    }

    const brepkit::ExportManifest manifest =
        brepkit::ExportStepHierarchyGlbs(argv[2], out_dir, exportOpts);

    std::cout << "exported " << manifest.nodes.size()
              << " hierarchy nodes into " << out_dir << "\n";

    return EXIT_SUCCESS;
}

int CmdExport(int argc, char **argv)
{
    if (argc < 4)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }
    const std::filesystem::path out_dir = argv[3];

    brepkit::ExportOptions exportOpts = {
        .profiles = GetDefaultProfiles()};

    auto opts = ParseArgs(argc, argv);

    std::map<std::string, std::string> digest_map_storage;
    if (opts.map_file)
    {
        digest_map_storage = cyanotype::load_digest_map(*opts.map_file);
        exportOpts.digest_map = &digest_map_storage;
    }

    const brepkit::ExportManifest manifest =
        brepkit::StepToGlb(argv[2], out_dir, exportOpts);

    std::cout << "exported " << manifest.nodes.size()
              << " hierarchy nodes into " << out_dir << "\n";

    return EXIT_SUCCESS;
}

int CmdLocation(int argc, char **argv)
{
    if (argc != 3)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::filesystem::path path = argv[2];

    try
    {
        const brepkit::Placement placement = brepkit::ReadTopLevelPlacement(path);
        std::cout << brepkit::SerializePlacement(placement) << "\n";

        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}

int CmdCount(int argc, char **argv)
{
    if (argc != 3)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::filesystem::path path = argv[2];

    try
    {
        const std::size_t count = brepkit::CountDistinctLocations(path);
        std::cout << count << "\n";

        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}

int CmdInspectLocations(int argc, char **argv)
{
    if (argc != 3)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::filesystem::path path = argv[2];

    try
    {
        brepkit::InspectLocations(path);
        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}

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

        if (cmd == "inspect")
        {
            return CmdInspect(argc, argv);
        }

        if (cmd == "export")
        {
            return CmdExport(argc, argv);
        }

        if (cmd == "location")
        {
            return CmdLocation(argc, argv);
        }

        if (cmd == "count")
        {
            return CmdCount(argc, argv);
        }

        if (cmd == "inspect_locations")
        {
            return CmdInspectLocations(argc, argv);
        }

        if (cmd == "canonicalize")
        {
            return CmdCanonicalize(argc, argv);
        }

        if (cmd == "export_all")
        {
            return CmdExportAll(argc, argv);
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
