#include "synchronous_renderer_frontend.hpp"

#include <mbgl/renderer/renderer.hpp>
#include <mbgl/map/backend_scope.hpp>
#include <mbgl/util/async_task.hpp>

namespace mbgl {
namespace android {

SynchronousRendererFrontend::SynchronousRendererFrontend(
        Backend& backend_,
        InvalidateCallback invalidate)
        : backend(backend_)
        , asyncInvalidate([=, invalidate=std::move(invalidate)]() {
            invalidate();
        }) {
}

SynchronousRendererFrontend::~SynchronousRendererFrontend() {
    BackendScope guard { backend };
    renderer.reset();
}

void SynchronousRendererFrontend::onLowMemory() {
    if (!renderer) return;
    renderer->onLowMemory();
}

void SynchronousRendererFrontend::dumpDebugLogs() {
    if (!renderer) return;
    renderer->dumpDebugLogs();
}

void SynchronousRendererFrontend::setRenderer(
        std::unique_ptr<Renderer> renderer_,
        RendererObserver& observer) {
    renderer = std::move(renderer_);
    renderer->setObserver(&observer);
}

void SynchronousRendererFrontend::update(std::shared_ptr<UpdateParameters> params) {
    updateParameters = std::move(params);
    asyncInvalidate.send();
}

void SynchronousRendererFrontend::render(View& view) {
    assert(BackendScope::exists());

    if (!updateParameters) {
        return;
    }

    renderer->render(backend, view, *updateParameters);
}

std::vector<Feature> SynchronousRendererFrontend::queryRenderedFeatures(
        std::shared_ptr<RenderedQueryParameters> params) const {
    return renderer->queryRenderedFeatures(*params);
}

std::vector<Feature> SynchronousRendererFrontend::querySourceFeatures(
        std::shared_ptr<SourceQueryParameters> params) const {
    return renderer->querySourceFeatures(*params);
}

} // namespace android
} // namespace mbgl

