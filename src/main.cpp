#include "brepkit/mesh.hpp"
#include "brepkit/step_export.hpp"
#include "cyanotype/load.hpp"

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

    struct options
    {
        std::optional<std::string> map_file;
    };

    options parse_args(int argc, char **argv)
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

int cmd_inspect(int argc, char **argv)
{
    if (argc != 3)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::filesystem::path path = argv[2];
    const double linear_deflection = (argc >= 4) ? std::stod(argv[3]) : 0.1;
    const double angular_deflection_rad = (argc >= 5) ? std::stod(argv[4]) : 0.5;

    return brepkit::run_inspect_step(path, linear_deflection, angular_deflection_rad);
}

int cmd_export_all(int argc, char **argv)
{
    if (argc < 4)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    const std::filesystem::path out_dir = argv[3];

    brepkit::ExportOptions exportOpts = {
        .profiles = default_profiles()};

    auto opts = parse_args(argc, argv);

    std::map<std::string, std::string> digest_map_storage;
    if (opts.map_file)
    {
        digest_map_storage = cyanotype::load_digest_map(*opts.map_file);
        exportOpts.digest_map = &digest_map_storage;
    }

    const brepkit::ExportManifest manifest =
        brepkit::export_step_hierarchy_glbs(argv[2], out_dir, exportOpts);

    std::cout << "exported " << manifest.nodes.size()
              << " hierarchy nodes into " << out_dir << "\n";

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    try
    {
        const std::string_view cmd = argv[1];
        const std::filesystem::path input = argv[2];

        if (cmd == "inspect")
        {
            return cmd_inspect(argc, argv);
        }

        if (cmd == "export")
        {
            return cmd_export_all(argc, argv);
        }

        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    catch (const std::exception &e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
