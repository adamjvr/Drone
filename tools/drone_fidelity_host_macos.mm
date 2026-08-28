#include <drone/fidelity/indexed_framebuffer.hpp>
#include <drone/formats/jba.hpp>

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>

#include <iostream>
#include <stdexcept>
#include <vector>

@interface DroneView : NSView
@property(nonatomic) std::vector<unsigned char>* rgba;
@end

@implementation DroneView
- (BOOL)isFlipped { return YES; }
- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    if (!_rgba || _rgba->empty()) return;
    CGDataProviderRef provider = CGDataProviderCreateWithData(nullptr, _rgba->data(), _rgba->size(), nullptr);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGImageRef image = CGImageCreate(320, 200, 8, 32, 320 * 4, colorSpace,
                                     kCGImageAlphaLast | kCGBitmapByteOrderDefault,
                                     provider, nullptr, false, kCGRenderingIntentDefault);
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
    CGContextDrawImage(ctx, NSRectToCGRect(self.bounds), image);
    CGImageRelease(image);
    CGColorSpaceRelease(colorSpace);
    CGDataProviderRelease(provider);
}
- (BOOL)acceptsFirstResponder { return YES; }
- (void)keyDown:(NSEvent*)event {
    NSString* chars = [event charactersIgnoringModifiers];
    if ([event keyCode] == 53 || [chars caseInsensitiveCompare:@"q"] == NSOrderedSame) {
        [NSApp terminate:nil];
    } else {
        [super keyDown:event];
    }
}
@end

int main(int argc, char** argv) try {
    @autoreleasepool {
        if (argc < 2 || argc > 3) {
            std::cerr << "Usage: drone_fidelity_host <image-or-sheet.jba> [integer-scale]\n";
            return 2;
        }
        int scale = argc == 3 ? std::stoi(argv[2]) : 3;
        if (scale < 1 || scale > 8) throw std::runtime_error("scale must be between 1 and 8");

        drone::fidelity::IndexedFramebuffer framebuffer;
        framebuffer.load(drone::formats::load_jba_320x200(argv[1]));
        auto rgba = framebuffer.rgba8();

        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSRect rect = NSMakeRect(0, 0, 320 * scale, 200 * scale);
        NSWindow* window = [[NSWindow alloc]
            initWithContentRect:rect
                      styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable)
                        backing:NSBackingStoreBuffered
                          defer:NO];
        [window setTitle:@"Drone Fidelity Host"];
        DroneView* view = [[DroneView alloc] initWithFrame:rect];
        view.rgba = &rgba;
        [window setContentView:view];
        [window makeFirstResponder:view];
        [window center];
        [window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
} catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
}
