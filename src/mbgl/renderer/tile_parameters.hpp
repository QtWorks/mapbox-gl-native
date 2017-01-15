#pragma once

#include <mbgl/map/mode.hpp>

namespace mbgl {

class TransformState;
class Scheduler;
class FileSource;
class AnnotationManager;
class ImageManager;
class GlyphManager;

class TileParameters {
public:
    float pixelRatio;
    MapDebugOptions debugOptions;
    const TransformState& transformState;
    Scheduler& workerScheduler;
    FileSource& fileSource;
    const MapMode mode;
    AnnotationManager& annotationManager;
    ImageManager& imageManager;
    GlyphManager& glyphManager;
    const optional<uint8_t> fixedPrefetchZoom = {};
    const optional<uint8_t> dynamicPrefetchZoomDelta = {};
};

} // namespace mbgl
