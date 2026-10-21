#include "apps/native_file_select_assets.hpp"
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    assert(argc == 2);
    @autoreleasepool {
        using namespace sadx::native::app;
        const auto assets = load_file_select_display_assets(argv[1]);
        assert(assets.tables.frame_rows.size() == 3 && assets.tables.sky_tiles.size() == 8);
        assert(assets.tables.frame_rows[0].right_x == 568);
        assert(assets.tables.frame_rows[0].bottom_y == 413);
        assert(assets.tables.frame_rows[0].depth == -12);
        const auto utc = [NSTimeZone timeZoneForSecondsFromGMT:0];
        constexpr std::uint64_t unix_epoch = 116444736000000000ULL;
        auto date = file_calendar_in_zone(unix_epoch, utc);
        assert(date.year == 1970 && date.month == 1 && date.day == 1);
        assert(date.hour == 0 && date.minute == 0 && date.second == 0);
        date = file_calendar_in_zone(unix_epoch + 9999999, utc);
        assert(date.day == 1 && date.hour == 0 && date.minute == 0 && date.second == 0);
        date = file_calendar_in_zone(unix_epoch - 1, utc);
        assert(date.year == 1969 && date.month == 12 && date.day == 31);
        assert(date.hour == 23 && date.minute == 59 && date.second == 59);
        date = file_calendar_in_zone(unix_epoch, [NSTimeZone timeZoneForSecondsFromGMT:3600]);
        assert(date.year == 1970 && date.month == 1 && date.day == 1 && date.hour == 1);
        const auto local = local_file_calendar(unix_epoch);
        const auto direct = file_calendar_in_zone(unix_epoch, NSTimeZone.localTimeZone);
        assert(local.year == direct.year && local.month == direct.month && local.day == direct.day);
        assert(local.hour == direct.hour && local.minute == direct.minute && local.second == direct.second);
        std::array<std::uint8_t, 4> values{};
        bool rejected = false;
        try {
            file_select_integer_table(@[@0, @1, @256, @3], values, "byte table");
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        assert(rejected);
    }
    std::cout << "Imported file-select tables and real local FILETIME calendar pass\n";
}
