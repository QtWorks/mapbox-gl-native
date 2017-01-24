#include <mbgl/test/util.hpp>
#include <mbgl/test/stub_file_source.hpp>

#include <mbgl/gl/headless_backend.hpp>
#include <mbgl/gl/offscreen_view.hpp>
#include <mbgl/map/backend_scope.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/storage/default_file_source.hpp>
#include <mbgl/style/style.hpp>
#include <mbgl/util/default_thread_pool.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/run_loop.hpp>

#include <algorithm>
#include <string>
#include <vector>

using namespace mbgl;
using namespace mbgl::style;
using namespace std::literals::string_literals;

TEST(Map, PrefetchTiles) {
    util::RunLoop runLoop;
    HeadlessBackend backend(test::sharedDisplay());
    BackendScope scope(backend);
    OffscreenView view(backend.getContext(), { 512, 512 });
    ThreadPool threadPool(4);
    StubFileSource fileSource;
    Map map(backend, view.getSize(), 1, fileSource, threadPool, MapMode::Still);

    std::vector<int> tiles;

    fileSource.response = [&] (const Resource& res) -> optional<Response> {
        Response response;

        auto zoom = std::stoi(res.url);
        tiles.push_back(zoom);

        // Return a red tile for prefetched tiles or green to the actual tile.
        // The end rendering result should be all green because the map is only
        // considered fully rendered when only ideal tiles are shown.
        if (zoom == int(map.getZoom()) + 1) {
            response.data = std::make_shared<std::string>(
                util::read_file("test/fixtures/map/prefetch/tile_green.png"));
        } else {
            response.data = std::make_shared<std::string>(
                util::read_file("test/fixtures/map/prefetch/tile_red.png"));
        }

        return { std::move(response) };
    };

    auto checkTilesForZoom = [&](int zoom, const std::vector<int>& expected) {
        tiles.clear();

        // Force tile reloading.
        map.getStyle().loadJSON(util::read_file("test/fixtures/map/prefetch/empty.json"));
        map.getStyle().loadJSON(util::read_file("test/fixtures/map/prefetch/style.json"));

        map.setLatLngZoom({ 40.726989, -73.992857 }, zoom); // Manhattan

        // Should always render the ideal tiles (i.e. a green map)
        test::checkImage("test/fixtures/map/prefetch", test::render(map, view));

        ASSERT_TRUE(std::is_permutation(tiles.begin(), tiles.end(), expected.begin()));
        ASSERT_FALSE(tiles.empty());
    };

    // No prefetching, raster tiles will use ideal
    // tiles instead of the actual zoom level, that is
    // why the zoom levels for non-prefetched tiles are
    // not the same.
    checkTilesForZoom(10, { 11, 11, 11, 11, 11, 11, 11, 11, 11 });

    map.setFixedPrefetchZoom(4);
    checkTilesForZoom(12, { 13, 13, 13, 13, 13, 13, 13, 13, 13, 4 });

    map.setFixedPrefetchZoom(0);
    checkTilesForZoom(10, { 11, 11, 11, 11, 11, 11, 11, 11, 11, 0 });

    // Should not prefetch, current zoom has lower zoom level.
    map.setFixedPrefetchZoom(12);
    checkTilesForZoom(9, { 10, 10, 10, 10, 10, 10, 10, 10, 10 });

    // Disabled.
    map.setFixedPrefetchZoom({});
    checkTilesForZoom(17, { 18, 18, 18, 18, 18, 18, 18, 18, 18 });

    map.setDynamicPrefetchZoomDelta(4);
    checkTilesForZoom(12, { 13, 13, 13, 13, 13, 13, 13, 13, 13, 9 });

    // Should clamp at `minzoom`.
    map.setDynamicPrefetchZoomDelta(20);
    checkTilesForZoom(10, { 11, 11, 11, 11, 11, 11, 11, 11, 11, 0 });

    // Disabled.
    map.setDynamicPrefetchZoomDelta({});
    checkTilesForZoom(13, { 14, 14, 14, 14, 14, 14, 14, 14, 14 });

    // Fixed takes precedence over dynamic.
    map.setDynamicPrefetchZoomDelta(2);
    map.setFixedPrefetchZoom(0);
    checkTilesForZoom(15, { 16, 16, 16, 16, 16, 16, 16, 16, 16, 0 });
}
