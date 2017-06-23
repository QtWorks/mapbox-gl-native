#include "node_renderer_frontend.hpp"
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/map/backend_scope.hpp>

namespace node_mbgl {

    NodeRendererFrontend::NodeRendererFrontend(mbgl::Backend& backend_, ViewAccessorFunction getView)
    : backend(backend_)
    , asyncInvalidate([&, this, getView] {
        if (renderer && updateParameters) {
            mbgl::BackendScope guard { backend };
            mbgl::View* view = getView();
            renderer->render(backend, *view, *updateParameters);
        }
    }) {
}

NodeRendererFrontend::~NodeRendererFrontend() {
    mbgl::BackendScope guard { backend };
    renderer.reset();
}

void NodeRendererFrontend::update(std::shared_ptr<mbgl::UpdateParameters> updateParameters_) {
    updateParameters = updateParameters_;
    asyncInvalidate.send();
}

std::vector<mbgl::Feature> NodeRendererFrontend::queryRenderedFeatures(std::shared_ptr<mbgl::RenderedQueryParameters> params) const {
    if (!renderer) return {};
    return renderer->queryRenderedFeatures(*params);
}
std::vector<mbgl::Feature> NodeRendererFrontend::querySourceFeatures(std::shared_ptr<mbgl::SourceQueryParameters> params) const {
    if (!renderer) return {};
    return renderer->querySourceFeatures(*params);
}

void NodeRendererFrontend::setRenderer(std::unique_ptr<mbgl::Renderer> renderer_, mbgl::RendererObserver& observer_) {
    renderer = std::move(renderer_);
    renderer->setObserver(&observer_);
}

void NodeRendererFrontend::setSourceTileCacheSize(size_t size) {
    if (!renderer) return;
    return renderer->setSourceTileCacheSize(size);
}

void NodeRendererFrontend::onLowMemory() {
    if (!renderer) return;
    return renderer->onLowMemory();
}

void NodeRendererFrontend::dumpDebugLogs() {
    if (!renderer) return;
    return renderer->dumpDebugLogs();
}

} // namespace node_mbgl
