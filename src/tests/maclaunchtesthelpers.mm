// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#import <AppKit/AppKit.h>

bool dispatchMacReopenEvent()
{
    @autoreleasepool {
        NSAppleEventDescriptor* event = [NSAppleEventDescriptor
            appleEventWithEventClass:kCoreEventClass eventID:kAEReopenApplication
            targetDescriptor:nil returnID:kAutoGenerateReturnID transactionID:kAnyTransactionID];
        AppleEvent reply = {typeNull, nullptr};
        const OSErr result = [[NSAppleEventManager sharedAppleEventManager]
            dispatchRawAppleEvent:event.aeDesc withRawReply:&reply handlerRefCon:&reply];
        AEDisposeDesc(&reply);
        return result == noErr;
    }
}
