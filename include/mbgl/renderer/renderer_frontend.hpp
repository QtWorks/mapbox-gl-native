#pragma once

#include <mbgl/util/feature.hpp>
#include <mbgl/map/map_observer.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mbgl {

class UpdateParameters;
class Renderer;
class RendererObserver;
class RenderedQueryParameters;
class Scheduler;
class SourceQueryParameters;

class RendererFrontend {
public:

    virtual ~RendererFrontend() = default;

    virtual void update(std::shared_ptr<UpdateParameters>) = 0;

    virtual std::vector<Feature> queryRenderedFeatures(std::shared_ptr<RenderedQueryParameters>) const = 0;

    virtual std::vector<Feature> querySourceFeatures(std::shared_ptr<SourceQueryParameters>) const = 0;

    // Implementer must bind the renderer observer to the renderer in a
    // appropriate manner so that the callbacks occur on the main thread
    virtual void setRenderer(std::unique_ptr<Renderer>, RendererObserver&) = 0;

    virtual void onLowMemory() = 0;

    virtual void dumpDebugLogs() = 0;
};

} // namespace mbgl
