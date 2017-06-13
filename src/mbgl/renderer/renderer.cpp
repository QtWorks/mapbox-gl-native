#include <mbgl/renderer/renderer.hpp>
#include <mbgl/renderer/renderer_impl.hpp>
#include <mbgl/renderer/update_parameters.hpp>
#include <mbgl/map/backend_scope.hpp>

namespace mbgl {

Renderer::Renderer(float pixelRatio_,
                   Scheduler& scheduler_,
                   FileSource& fileSource_,
                   MapMode mode_,
                   GLContextMode contextMode_,
                   const optional<std::string> programCacheDir_)
        : impl(std::make_unique<Impl>(pixelRatio_, scheduler_, fileSource_, mode_,
                                      contextMode_, std::move(programCacheDir_))) {
}

Renderer::~Renderer() = default;

void Renderer::setObserver(RendererObserver* observer) {
    impl->setObserver(observer);
}

void Renderer::render(Backend& backend, View& view, const UpdateParameters& updateParameters) {
    assert(BackendScope::exists());
    impl->render(backend, view, updateParameters);
}

std::vector<Feature> Renderer::queryRenderedFeatures(const RenderedQueryParameters& params) const {
    return impl->queryRenderedFeatures(params);
}

std::vector<Feature> Renderer::querySourceFeatures(const SourceQueryParameters& params) const {
    return impl->querySourceFeatures(params);
}

void Renderer::dumpDebugLogs() {
    impl->dumDebugLogs();
}

void Renderer::onLowMemory() {
    impl->onLowMemory();
}

} // namespace mbgl
