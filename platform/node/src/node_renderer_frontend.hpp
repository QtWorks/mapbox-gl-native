#pragma once

#include <mbgl/map/backend.hpp>
#include <mbgl/renderer/renderer_frontend.hpp>
#include <mbgl/util/async_task.hpp>

#include <functional>
#include <memory>

namespace node_mbgl {

class NodeRendererFrontend : public mbgl::RendererFrontend {
public:
    using ViewAccessorFunction = std::function<mbgl::View* ()>;
    NodeRendererFrontend(mbgl::Backend&, ViewAccessorFunction);

    ~NodeRendererFrontend();

    void update(std::shared_ptr<mbgl::UpdateParameters>) override;

    std::vector<mbgl::Feature> queryRenderedFeatures(std::shared_ptr<mbgl::RenderedQueryParameters>) const override;
    std::vector<mbgl::Feature> querySourceFeatures(std::shared_ptr<mbgl::SourceQueryParameters>) const override;

    void setRenderer(std::unique_ptr<mbgl::Renderer>, mbgl::RendererObserver&) override;

    void setSourceTileCacheSize(size_t) override;
    void onLowMemory() override;
    void dumpDebugLogs() override;

private:
    mbgl::Backend& backend;
    std::unique_ptr<mbgl::Renderer> renderer;
    std::shared_ptr<mbgl::UpdateParameters> updateParameters;
    mbgl::util::AsyncTask asyncInvalidate;
};

} // namespace node_mbgl
