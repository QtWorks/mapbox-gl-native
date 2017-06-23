#import "MGLAnnotationView.h"
#import "MGLAnnotationView_Private.h"
#import "MGLMapView_Private.h"
#import "MGLAnnotation.h"

#import "NSBundle+MGLAdditions.h"

#import <GLKit/GLKit.h>

#include <mbgl/util/constants.hpp>

CATransform3D MGLTransform3DFromMatrix4(GLKMatrix4 matrix) {
    CATransform3D transform;
    
    transform.m11 = matrix.m[0];
    transform.m12 = matrix.m[1];
    transform.m13 = matrix.m[2];
    transform.m14 = matrix.m[3];
    
    transform.m21 = matrix.m[4];
    transform.m22 = matrix.m[5];
    transform.m23 = matrix.m[6];
    transform.m24 = matrix.m[7];
    
    transform.m31 = matrix.m[8];
    transform.m32 = matrix.m[9];
    transform.m33 = matrix.m[10];
    transform.m34 = matrix.m[11];
    
    transform.m41 = matrix.m[12];
    transform.m42 = matrix.m[13];
    transform.m43 = matrix.m[14];
    transform.m44 = matrix.m[15];
    
    return transform;
}

@interface MGLAnnotationView () <UIGestureRecognizerDelegate>

@property (nonatomic, readwrite, nullable) NSString *reuseIdentifier;
@property (nonatomic, readwrite) CATransform3D lastAppliedTransform;
@property (nonatomic, weak) UIPanGestureRecognizer *panGestureRecognizer;
@property (nonatomic, weak) UILongPressGestureRecognizer *longPressRecognizer;
@property (nonatomic, weak) MGLMapView *mapView;

@end

@implementation MGLAnnotationView

+ (BOOL)supportsSecureCoding {
    return YES;
}

- (instancetype)initWithReuseIdentifier:(NSString *)reuseIdentifier {
    self = [super initWithFrame:CGRectZero];
    if (self) {
        [self commonInitWithAnnotation:nil reuseIdentifier:reuseIdentifier];
    }
    return self;
}

- (instancetype)initWithAnnotation:(nullable id<MGLAnnotation>)annotation reuseIdentifier:(nullable NSString *)reuseIdentifier {
    self = [super initWithFrame:CGRectZero];
    if (self) {
        [self commonInitWithAnnotation:annotation reuseIdentifier:reuseIdentifier];
    }
    return self;
}

- (void)commonInitWithAnnotation:(nullable id<MGLAnnotation>)annotation reuseIdentifier:(nullable NSString *)reuseIdentifier {
    _lastAppliedTransform = CATransform3DIdentity;
    _annotation = annotation;
    _reuseIdentifier = [reuseIdentifier copy];
    _scalesWithViewingDistance = YES;
    _enabled = YES;
}

- (instancetype)initWithCoder:(NSCoder *)decoder {
    if (self = [super initWithCoder:decoder]) {
        _reuseIdentifier = [decoder decodeObjectOfClass:[NSString class] forKey:@"reuseIdentifier"];
        _annotation = [decoder decodeObjectOfClass:[NSObject class] forKey:@"annotation"];
        _centerOffset = [decoder decodeCGVectorForKey:@"centerOffset"];
        NSInteger freeAxes = [decoder decodeIntegerForKey:@"freeAxes"];
        if (freeAxes < 0 || freeAxes > MGLAnnotationViewBillboardAxisAll) {
            return nil;
        }
        _freeAxes = freeAxes;
        _scalesWithViewingDistance = [decoder decodeBoolForKey:@"scalesWithViewingDistance"];
        _selected = [decoder decodeBoolForKey:@"selected"];
        _enabled = [decoder decodeBoolForKey:@"enabled"];
        self.draggable = [decoder decodeBoolForKey:@"draggable"];
    }
    return self;
}

- (void)encodeWithCoder:(NSCoder *)coder {
    [super encodeWithCoder:coder];
    [coder encodeObject:_reuseIdentifier forKey:@"reuseIdentifier"];
    [coder encodeObject:_annotation forKey:@"annotation"];
    [coder encodeCGVector:_centerOffset forKey:@"centerOffset"];
    [coder encodeInteger:_freeAxes forKey:@"freeAxes"];
    [coder encodeBool:_scalesWithViewingDistance forKey:@"scalesWithViewingDistance"];
    [coder encodeBool:_selected forKey:@"selected"];
    [coder encodeBool:_enabled forKey:@"enabled"];
    [coder encodeBool:_draggable forKey:@"draggable"];
}

- (void)prepareForReuse
{
    // Intentionally left blank. The default implementation of this method does nothing.
}

- (void)setCenterOffset:(CGVector)centerOffset
{
    _centerOffset = centerOffset;
    self.center = self.center;
}

- (void)setSelected:(BOOL)selected
{
    [self setSelected:selected animated:NO];
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{
    [self willChangeValueForKey:@"selected"];
    _selected = selected;
    [self didChangeValueForKey:@"selected"];
}

- (CGPoint)center
{
    CGPoint center = super.center;
    center.x -= _centerOffset.dx;
    center.y -= _centerOffset.dy;
    return center;
}

- (void)setCenter:(CGPoint)center
{
    center.x += _centerOffset.dx;
    center.y += _centerOffset.dy;

    super.center = center;
    [self updateTransform];
}

- (void)setScalesWithViewingDistance:(BOOL)scalesWithViewingDistance
{
    if (_scalesWithViewingDistance != scalesWithViewingDistance)
    {
        _scalesWithViewingDistance = scalesWithViewingDistance;
        [self updateTransform];
    }
}

- (void)setFreeAxes:(MGLAnnotationViewBillboardAxis)freeAxes
{
    if (_freeAxes != freeAxes)
    {
        _freeAxes = freeAxes;
        [self updateTransform];
    }
}

- (void)updateTransform
{
    if (self.dragState == MGLAnnotationViewDragStateDragging) return;
    
    // We keep track of each viewing distance scale transform that we apply. Each iteration,
    // we can account for it so that we don't get cumulative scaling every time we move.
    // We also avoid clobbering any existing transform passed in by the client, too.
    CATransform3D undoOfLastAppliedTransform = CATransform3DInvert(_lastAppliedTransform);
    
    CATransform3D freeTransform = CATransform3DIdentity;
    MGLMapCamera *camera = self.mapView.camera;
//    if (camera.pitch >= 0 && (self.freeAxes & MGLAnnotationViewBillboardAxisX))
    {
//        freeTransform = self.mapView.projectionTransform;
        
        CGRect superBounds = self.superview.bounds;
        
        // Build a projection matrix, paralleling the code found in mbgl.
        // mbgl::TransformState::fov
        double fov = 0.6435011087932844;
        double halfFov = fov / 2.0;
        double cameraToCenterDistance = 0.5 * CGRectGetHeight(superBounds) / tanf(halfFov);
        
        double groundAngle = M_PI_2 + MGLRadiansFromDegrees(camera.pitch);
        double topHalfSurfaceDistance = sinf(halfFov) * cameraToCenterDistance / sinf(M_PI - groundAngle - halfFov);
        
        // Calculate z distance of the farthest fragment that should be rendered.
        double furthestDistance = cosf(M_PI_2 - MGLRadiansFromDegrees(camera.pitch)) * topHalfSurfaceDistance + cameraToCenterDistance;
        
        // Add a bit extra to avoid precision problems when a fragment's distance is exactly `furthestDistance`.
        double farZ = furthestDistance * 1.01;
        
        GLKMatrix4 projectionMatrix = GLKMatrix4MakePerspective(fov, CGRectGetWidth(superBounds) / CGRectGetHeight(superBounds), 1, farZ);
        CATransform3D projectionTransform = MGLTransform3DFromMatrix4(projectionMatrix);
//        freeTransform = projectionTransform;
//        self.layer.sublayerTransform = projectionTransform;
        
        // Unlike the Mapbox GL JS camera, separate camera translation and rotation out into its world matrix.
        // If this is applied directly to the projection matrix, it will work OK but break raycasting.
        GLKMatrix4 cameraTranslateZ = GLKMatrix4MakeTranslation(0, 0, -cameraToCenterDistance);
        GLKMatrix4 cameraRotateX = GLKMatrix4MakeXRotation(MGLRadiansFromDegrees(camera.pitch));
        GLKMatrix4 cameraWorldMatrix = GLKMatrix4Multiply(cameraRotateX, cameraTranslateZ);
        GLKMatrix4 cameraRotateZ = GLKMatrix4MakeZRotation(-MGLRadiansFromDegrees(camera.heading));
        cameraWorldMatrix = GLKMatrix4Multiply(cameraRotateZ, cameraWorldMatrix);
        
//        self.layer.sublayerTransform = MGLTransform3DFromMatrix4(cameraWorldMatrix);
        
        double zoomPow = powf(2.0, self.mapView.zoomLevel);
        double worldSize = mbgl::util::tileSize * zoomPow;
        double x = (180 + camera.centerCoordinate.longitude) * worldSize / 360.0;
        double y = MGLDegreesFromRadians(logf(tanf(M_PI_4 + camera.centerCoordinate.latitude * M_PI / 360.0)));
        y = (180 - y) * worldSize / 360.0;
        
        // Handle scaling and translation of objects in the map in the world's matrix transform, not the camera.
        GLKMatrix4 rotateMap = GLKMatrix4MakeZRotation(M_PI);
        GLKMatrix4 translateCenter = GLKMatrix4MakeTranslation(mbgl::util::tileSize / 2.0, -mbgl::util::tileSize / 2.0, 0);
        GLKMatrix4 worldMatrix = GLKMatrix4Multiply(translateCenter, rotateMap);
        GLKMatrix4 scale = GLKMatrix4MakeScale(zoomPow, zoomPow, zoomPow);
        worldMatrix = GLKMatrix4Multiply(scale, worldMatrix);
        GLKMatrix4 translateMap = GLKMatrix4MakeTranslation(-x, y, 0);
        worldMatrix = GLKMatrix4Multiply(translateMap, worldMatrix);
//        freeTransform = MGLTransform3DFromMatrix4(worldMatrix);
        
//        bool isInvertible;
//        GLKMatrix4 inverseCameraWorldMatrix = GLKMatrix4Invert(cameraWorldMatrix, &isInvertible);
//        NSAssert(isInvertible, @"Camera world matrix should be invertible.");
//        GLKMatrix4 modelViewMatrix = GLKMatrix4Multiply(inverseCameraWorldMatrix, worldMatrix);
//        freeTransform = MGLTransform3DFromMatrix4(modelViewMatrix);
        
        
        
        // https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/CoreAnimation_guide/AdvancedAnimationTricks/AdvancedAnimationTricks.html#//apple_ref/doc/uid/TP40004514-CH8-SW13
        // FIXME: This is a rough, eyeballed value. Replace this transform with one derived from mbgl::TransformState::coordinatePointMatrix().
        double xPoint = (180 + self.annotation.coordinate.longitude) * worldSize / 360.0;
        double yPoint = MGLDegreesFromRadians(logf(tanf(M_PI_4 + self.annotation.coordinate.latitude * M_PI / 360.0)));
        y = (180 - y) * worldSize / 360.0;
        double xDelta = x - xPoint;
        freeTransform.m34 = -1.0 / (1.0 - furthestDistance * 0.5);
        
        freeTransform = CATransform3DRotate(freeTransform, MGLRadiansFromDegrees(camera.pitch), -1.0, 0, 0);
//        self.layer.anchorPoint = [self convertPoint:self.superview.center toView:self];
    }
//    if (camera.heading >= 0 && (self.freeAxes & MGLAnnotationViewBillboardAxisY))
//    {
//        freeTransform = CATransform3DRotate(freeTransform, MGLRadiansFromDegrees(-camera.heading), 0.0, 0.0, 1.0);
//    }
    
    CATransform3D scaleTransform = CATransform3DIdentity;
    CGFloat superviewHeight = CGRectGetHeight(self.superview.frame);
    if (self.scalesWithViewingDistance && superviewHeight > 0.0) {
        // Find the maximum amount of scale reduction to apply as the view's center moves from the top
        // of the superview to the bottom. For example, if this view's center has moved 25% of the way
        // from the top of the superview towards the bottom then the maximum scale reduction is 1 - .25
        // or 75%. The range goes from a maximum of 100% to 0% as the view moves from the top to the bottom
        // along the y axis of its superview.
        CGFloat maxScaleReduction = 1.0 - self.center.y / superviewHeight;

        // The pitch intensity represents how much the map view is actually pitched compared to
        // what is possible. The value will range from 0% (not pitched at all) to 100% (pitched as much
        // as the map view will allow). The map view's maximum pitch is defined in `mbgl::util::PITCH_MAX`.
        // Since it is possible for the map view to report a pitch less than 0 due to the nature of
        // how the gesture information is captured, the value is guarded with MAX.
        CGFloat pitchIntensity = MAX(self.mapView.camera.pitch, 0) / MGLDegreesFromRadians(mbgl::util::PITCH_MAX);

        // The pitch adjusted scale is the inverse proportion of the maximum possible scale reduction
        // multiplied by the pitch intensity. For example, if the maximum scale reduction is 75% and the
        // map view is 50% pitched then the annotation view should be reduced by 37.5% (.75 * .5). The
        // reduction is then normalized for a scale of 1.0.
        CGFloat pitchAdjustedScale = 1.0 - maxScaleReduction * pitchIntensity;
//        scaleTransform = CATransform3DMakeScale(pitchAdjustedScale, pitchAdjustedScale, 1);
    }
    
    CATransform3D effectiveTransform = freeTransform;//CATransform3DConcat(freeTransform, scaleTransform);
    self.layer.transform = CATransform3DConcat(self.layer.transform, CATransform3DConcat(undoOfLastAppliedTransform, effectiveTransform));
    _lastAppliedTransform = effectiveTransform;
}

#pragma mark - Draggable

- (void)setDraggable:(BOOL)draggable
{
    [self willChangeValueForKey:@"draggable"];
    _draggable = draggable;
    [self didChangeValueForKey:@"draggable"];

    if (draggable)
    {
        [self enableDrag];
    }
    else
    {
        [self disableDrag];
    }
}

- (void)enableDrag
{
    if (!_longPressRecognizer)
    {
        UILongPressGestureRecognizer *recognizer = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)];
        recognizer.delegate = self;
        [self addGestureRecognizer:recognizer];
        _longPressRecognizer = recognizer;
    }

    if (!_panGestureRecognizer)
    {
        UIPanGestureRecognizer *recognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)];
        recognizer.delegate = self;
        [self addGestureRecognizer:recognizer];
        _panGestureRecognizer = recognizer;
    }
}

- (void)disableDrag
{
    [self removeGestureRecognizer:_longPressRecognizer];
    [self removeGestureRecognizer:_panGestureRecognizer];
}

- (void)handleLongPress:(UILongPressGestureRecognizer *)sender
{
    switch (sender.state) {
        case UIGestureRecognizerStateBegan:
            self.dragState = MGLAnnotationViewDragStateStarting;
            break;
        case UIGestureRecognizerStateChanged:
            self.dragState = MGLAnnotationViewDragStateDragging;
            break;
        case UIGestureRecognizerStateCancelled:
            self.dragState = MGLAnnotationViewDragStateCanceling;
            break;
        case UIGestureRecognizerStateEnded:
            self.dragState = MGLAnnotationViewDragStateEnding;
            break;
        case UIGestureRecognizerStateFailed:
            self.dragState = MGLAnnotationViewDragStateNone;
            break;
        case UIGestureRecognizerStatePossible:
            break;
    }
}

- (void)handlePan:(UIPanGestureRecognizer *)sender
{
    self.center = [sender locationInView:sender.view.superview];

    if (sender.state == UIGestureRecognizerStateEnded) {
        self.dragState = MGLAnnotationViewDragStateNone;
    }
}

- (void)setDragState:(MGLAnnotationViewDragState)dragState
{
    [self setDragState:dragState animated:YES];
}

- (void)setDragState:(MGLAnnotationViewDragState)dragState animated:(BOOL)animated
{
    [self willChangeValueForKey:@"dragState"];
    _dragState = dragState;
    [self didChangeValueForKey:@"dragState"];

    if (dragState == MGLAnnotationViewDragStateStarting)
    {
        [self.mapView.calloutViewForSelectedAnnotation dismissCalloutAnimated:animated];
        [self.superview bringSubviewToFront:self];
    }
    else if (dragState == MGLAnnotationViewDragStateCanceling)
    {
        if (!self.annotation) {
            [NSException raise:NSInvalidArgumentException
                        format:@"Annotation property should not be nil."];
        }
        self.panGestureRecognizer.enabled = NO;
        self.longPressRecognizer.enabled = NO;
        self.center = [self.mapView convertCoordinate:self.annotation.coordinate toPointToView:self.mapView];
        self.panGestureRecognizer.enabled = YES;
        self.longPressRecognizer.enabled = YES;
        self.dragState = MGLAnnotationViewDragStateNone;
    }
    else if (dragState == MGLAnnotationViewDragStateEnding)
    {
        if ([self.annotation respondsToSelector:@selector(setCoordinate:)])
        {
            CLLocationCoordinate2D coordinate = [self.mapView convertPoint:self.center toCoordinateFromView:self.mapView];
            [(NSObject *)self.annotation setValue:[NSValue valueWithMGLCoordinate:coordinate] forKey:@"coordinate"];
        }
    }
}

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer
{
    BOOL isDragging = self.dragState == MGLAnnotationViewDragStateDragging;

    if (gestureRecognizer == _panGestureRecognizer && !(isDragging))
    {
        return NO;
    }

    return YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    return otherGestureRecognizer == _longPressRecognizer || otherGestureRecognizer == _panGestureRecognizer;
}

#pragma mark UIAccessibility methods

- (BOOL)isAccessibilityElement {
    return !self.hidden;
}

- (UIAccessibilityTraits)accessibilityTraits {
    return UIAccessibilityTraitButton | UIAccessibilityTraitAdjustable;
}

- (NSString *)accessibilityLabel {
    return [self.annotation respondsToSelector:@selector(title)] ? self.annotation.title : super.accessibilityLabel;
}

- (NSString *)accessibilityValue {
    return [self.annotation respondsToSelector:@selector(subtitle)] ? self.annotation.subtitle : super.accessibilityValue;
}

- (NSString *)accessibilityHint {
    return NSLocalizedStringWithDefaultValue(@"ANNOTATION_A11Y_HINT", nil, nil, @"Shows more info", @"Accessibility hint");
}

- (CGRect)accessibilityFrame {
    CGRect accessibilityFrame = self.frame;
    CGRect minimumFrame = CGRectInset({ self.center, CGSizeZero },
                                      -MGLAnnotationAccessibilityElementMinimumSize.width / 2,
                                      -MGLAnnotationAccessibilityElementMinimumSize.height / 2);
    accessibilityFrame = CGRectUnion(accessibilityFrame, minimumFrame);
    return accessibilityFrame;
}

- (void)accessibilityIncrement {
    [self.superview accessibilityIncrement];
}

- (void)accessibilityDecrement {
    [self.superview accessibilityDecrement];
}

@end
