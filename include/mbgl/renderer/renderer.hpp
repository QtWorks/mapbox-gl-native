#pragma once

#include <mbgl/map/mode.hpp>
#include <mbgl/map/query.hpp>
#include <mbgl/util/feature.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mbgl {

class Backend;
class FileSource;
class UpdateParameters;
class RenderedQueryParameters;
class SourceQueryParameters;
class UpdateParameters;
class RendererObserver;
class Scheduler;
class View;

class Renderer {
public:
    Renderer(float pixelRatio_, Scheduler&, FileSource&,
             MapMode, GLContextMode, const optional<std::string>);
    ~Renderer();

    void render(Backend&, View& view, const UpdateParameters&);

    void setObserver(RendererObserver*);

    std::vector<Feature> queryRenderedFeatures(const RenderedQueryParameters&) const;

    std::vector<Feature> querySourceFeatures(const SourceQueryParameters&) const;

    void dumpDebugLogs();

    void onLowMemory();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace mbgl
