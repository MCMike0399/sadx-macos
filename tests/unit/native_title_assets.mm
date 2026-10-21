#include "apps/native_assets.hpp"
#include <functional>
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 2)
        throw std::runtime_error("native-title-assets-test IMPORTED_DIRECTORY");
    @autoreleasepool {
        const auto directory = std::filesystem::path(argv[1]);
        const auto original = sadx::native::app::read_metadata(directory / "title-menu.json");
        const auto encoded = [NSJSONSerialization dataWithJSONObject:original options:0 error:nil];
        const auto valid = sadx::native::app::title_assets(directory);
        if (valid.sequences.empty() || valid.selection.trial_stages[0].empty())
            throw std::runtime_error("valid source title assets rejected");
        auto pattern = (std::filesystem::temp_directory_path() / "sadx-title-assets-XXXXXX").string();
        if (!mkdtemp(pattern.data()))
            throw std::runtime_error("could not create temporary title validation directory");
        struct TemporaryDirectory {
            std::filesystem::path path;
            ~TemporaryDirectory() { std::filesystem::remove_all(path); }
        } temporary{pattern};
        const auto reject = [&](const std::function<void(NSMutableDictionary *)> &mutate) {
            auto document = static_cast<NSMutableDictionary *>([NSJSONSerialization
                JSONObjectWithData:encoded options:NSJSONReadingMutableContainers error:nil]);
            mutate(document);
            const auto bytes = [NSJSONSerialization dataWithJSONObject:document options:0 error:nil];
            const auto path = temporary.path / "title-menu.json";
            if (![bytes writeToFile:[NSString stringWithUTF8String:path.c_str()] atomically:YES])
                throw std::runtime_error("could not write malformed title fixture");
            try {
                (void)sadx::native::app::title_assets(temporary.path);
            } catch (const std::runtime_error &) {
                return;
            }
            throw std::runtime_error("malformed title catalog accepted");
        };
        reject([](NSMutableDictionary *root) {
            root[@"selection_data"][@"trial_stages"][0] = [NSMutableArray array];
        });
        reject([](NSMutableDictionary *root) {
            NSMutableArray *stages = root[@"selection_data"][@"trial_stages"][0];
            while (stages.count < 11)
                [stages addObject:@{@"stage": @1, @"act": @0}];
        });
        reject([](NSMutableDictionary *root) {
            root[@"selection_data"][@"trial_stages"][0][0][@"stage"] = @43;
        });
        reject([](NSMutableDictionary *root) {
            root[@"selection_data"][@"character_ids"][0] = @8;
        });
        std::cout << "Native title assets: valid import and four unsafe catalogs checked\n";
    }
}
