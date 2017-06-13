
#pragma once

#include <mbgl/map/backend.hpp>
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/renderer/renderer_frontend.hpp>
#include <mbgl/util/async_task.hpp>

#include <functional>
#include <memory>

namespace mbgl {

class StubRendererFrontend : public RendererFrontend {
public:
    StubRendererFrontend(std::function<void (StubRendererFrontend&)>);
    StubRendererFrontend(Backend&, View&);
    ~StubRendererFrontend() override = default;

    void update(std::shared_ptr<UpdateParameters>) override;
    void render(Backend&, View& view);

    std::vector<Feature> queryRenderedFeatures(std::shared_ptr<RenderedQueryParameters>) const override;

    std::vector<Feature> querySourceFeatures(std::shared_ptr<SourceQueryParameters>) const override;

    void setRenderer(std::unique_ptr<Renderer>, RendererObserver&) override;

    void onLowMemory() override;

    void dumpDebugLogs() override;

private:
    std::unique_ptr<Renderer> renderer;
    std::shared_ptr<UpdateParameters> updateParameters;
    util::AsyncTask asyncInvalidate;
};

} // namespace mbgl
