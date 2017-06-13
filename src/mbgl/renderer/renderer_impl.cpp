#include <mbgl/renderer/renderer_impl.hpp>
#include <mbgl/actor/scheduler.hpp>
#include <mbgl/storage/file_source.hpp>
#include <mbgl/renderer/render_style.hpp>
#include <mbgl/renderer/painter.hpp>
#include <mbgl/renderer/update_parameters.hpp>
// TODO move to renderer?
#include <mbgl/map/backend_scope.hpp>

namespace mbgl {

static RendererObserver& nullObserver() {
    static RendererObserver observer;
    return observer;
}

Renderer::Impl::Impl(float pixelRatio_,
                     Scheduler& scheduler_,
                     FileSource& fileSource_,
                     MapMode mode_,
                     GLContextMode contextMode_,
                     const optional<std::string> programCacheDir_)
        : observer(&nullObserver())
        , mode(mode_)
        , contextMode(contextMode_)
        , pixelRatio(pixelRatio_),
          programCacheDir(programCacheDir_)
        , renderStyle(std::make_unique<RenderStyle>(scheduler_, fileSource_)) {

    renderStyle->setObserver(this);
}

Renderer::Impl::~Impl() {
    renderStyle.reset();
};

void Renderer::Impl::setObserver(RendererObserver* observer_) {
    observer = observer_ ? observer_ : &nullObserver();
}

void Renderer::Impl::render(Backend& backend, View& view, const UpdateParameters& updateParameters) {

    // Update render style
    renderStyle->update(updateParameters);

    // Initialize painter
    if (!painter) {
        gl::Context& context = backend.getContext();
        painter = std::make_unique<Painter>(context,
                                            pixelRatio,
                                            programCacheDir);
    }

    // Update transform state on painter.
    painter->state = updateParameters.transformState;

    bool loaded = updateParameters.styleLoaded && renderStyle->isLoaded();

    if (mode == MapMode::Continuous) {
        if (renderState == RenderState::Never) {
            observer->onWillStartRenderingMap();
        }

        observer->onWillStartRenderingFrame();

        FrameData frameData { updateParameters.timePoint,
                              pixelRatio,
                              mode,
                              contextMode,
                              updateParameters.debugOptions };

        backend.updateAssumedState();

        painter->render(*renderStyle, frameData, view);
        painter->cleanup();

        observer->onDidFinishRenderingFrame(
                loaded ? RendererObserver::RenderMode::Full : RendererObserver::RenderMode::Partial,
                renderStyle->hasTransitions() || painter->needsAnimation()
        );

        if (!loaded) {
            renderState = RenderState::Partial;
        } else if (renderState != RenderState::Fully) {
            renderState = RenderState::Fully;
            observer->onDidFinishRenderingMap();
        }
    } else if (loaded) {
        // We can render the map in still mode
        observer->onWillStartRenderingMap();
        observer->onWillStartRenderingFrame();

        FrameData frameData { updateParameters.timePoint,
                              pixelRatio,
                              mode,
                              contextMode,
                              updateParameters.debugOptions };

        backend.updateAssumedState();

        painter->render(*renderStyle, frameData, view);

        // TODO, match the order of this to continuous rendering?
        observer->onDidFinishRenderingFrame(RendererObserver::RenderMode::Full, false);
        observer->onDidFinishRenderingMap();
        painter->cleanup();
    }
}

std::vector<Feature> Renderer::Impl::queryRenderedFeatures(const RenderedQueryParameters& params) const {
    if (!renderStyle) return {};

    return renderStyle->queryRenderedFeatures(params.geometry, params.transformState, params.options);
}

std::vector<Feature> Renderer::Impl::querySourceFeatures(const SourceQueryParameters& params) const {
    if (!renderStyle) return {};

    const RenderSource* source = renderStyle->getRenderSource(params.sourceID);
    if (!source) return {};

    return source->querySourceFeatures(params.options);
}

void Renderer::Impl::onInvalidate() {
    observer->onInvalidate();
};

void Renderer::Impl::onResourceError(std::exception_ptr ptr) {
    observer->onResourceError(ptr);
}

void Renderer::Impl::onLowMemory() {
    if (painter) {
        assert(BackendScope::exists());
        painter->cleanup();
    }
    if (renderStyle) {
        renderStyle->onLowMemory();
        observer->onInvalidate();
    }
}

void Renderer::Impl::dumDebugLogs() {
    if (!renderStyle) return;

    renderStyle->dumpDebugLogs();
};


}
