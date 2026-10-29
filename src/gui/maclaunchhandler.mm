// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "launchcontroller.h"
#include "platformactivation.h"

#import <AppKit/AppKit.h>

@interface SpeedCrunchReopenHandler : NSObject {
    QPointer<LaunchController> m_controller;
}
- (instancetype)initWithController:(LaunchController*)controller;
- (void)install;
- (void)applicationWillFinishLaunching:(NSNotification*)notification;
- (void)handleReopen:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)reply;
@end

@implementation SpeedCrunchReopenHandler
- (instancetype)initWithController:(LaunchController*)controller
{
    self = [super init];
    if (self) {
        m_controller = controller;
        // AppKit installs its default handlers while launching. Install ours
        // at the documented point, after those defaults are in place.
        [self install];
        if (![NSApp isRunning]) {
            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillFinishLaunching:)
                name:NSApplicationWillFinishLaunchingNotification object:nil];
        }
    }
    return self;
}
- (void)applicationWillFinishLaunching:(NSNotification*)notification
{
    Q_UNUSED(notification);
    [self install];
}
- (void)install
{
    [[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
        andSelector:@selector(handleReopen:withReplyEvent:)
        forEventClass:kCoreEventClass andEventID:kAEReopenApplication];
}
- (void)handleReopen:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)reply
{
    Q_UNUSED(event);
    Q_UNUSED(reply);
    // Reopen is distinct from becoming active, so Command-Tab and ordinary
    // window clicks never create sessions. Initial launches use a different event.
    if (m_controller)
        m_controller->request();
}
- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [[NSAppleEventManager sharedAppleEventManager] removeEventHandlerForEventClass:kCoreEventClass
        andEventID:kAEReopenApplication];
    [super dealloc];
}
@end

void* installMacReopenHandler(LaunchController* controller)
{
    return [[SpeedCrunchReopenHandler alloc] initWithController:controller];
}

void removeMacReopenHandler(void* handler)
{
    [[NSAppleEventManager sharedAppleEventManager] removeEventHandlerForEventClass:kCoreEventClass
        andEventID:kAEReopenApplication];
    [static_cast<SpeedCrunchReopenHandler*>(handler) release];
}

void activateMacApplication()
{
    [NSApp activateIgnoringOtherApps:YES];
}
