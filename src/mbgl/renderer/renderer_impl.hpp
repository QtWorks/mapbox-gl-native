#pragma once
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/renderer/renderer_observer.hpp>
#include <mbgl/renderer/render_style_observer.hpp>
#include <mbgl/style/style.hpp>

#include <mbgl/map/map.hpp>
#include <mbgl/map/backend.hpp>
#include <mbgl/map/transform_state.hpp>
#include <mbgl/map/view.hpp>

#include <memory>
#include <string>

namespace mbgl {

enum class RenderState : uint8_t {
    Never,
    Partial,
    Fully,
};

class Painter;
class RenderStyle;
class View;


class RenderResult {
public:
    bool needsRepaint;
};

class RenderedQueryParameters {
public:
    const ScreenLineString geometry;
    const TransformState transformState;
    const RenderedQueryOptions options;
};

class SourceQueryParameters {
public:
    const std::string sourceID;
    const SourceQueryOptions options;
};

class Renderer::Impl : public RenderStyleObserver {
public:
    Impl(float pixelRatio_, Scheduler&, FileSource&,
         MapMode, GLContextMode, const optional<std::string>);
    ~Impl() final;

    void setObserver(RendererObserver*);

    void render(Backend&, View&, const UpdateParameters&);

    std::vector<Feature> queryRenderedFeatures(const RenderedQueryParameters&) const;

    std::vector<Feature> querySourceFeatures(const SourceQueryParameters&) const;


    // TODO: needs backendScope
    void onLowMemory();

    void dumDebugLogs() ;

    // RenderStyleObserver implementation
    void onInvalidate()override;
    void onResourceError(std::exception_ptr) override;

private:
    friend class Renderer;

    RendererObserver* observer;

    const MapMode mode;
    const GLContextMode contextMode;
    const float pixelRatio;
    const optional<std::string> programCacheDir;

    RenderState renderState = RenderState::Never;

    std::unique_ptr<RenderStyle> renderStyle;
    std::unique_ptr<Painter> painter;

    size_t sourceCacheSize;

};

} // namespace mbgl
