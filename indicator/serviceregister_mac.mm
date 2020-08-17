/*
 * SPDX-FileCopyrightText: 2020 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#include <QFile>
#include <QProcess>
#include <QApplication>

@interface KDEConnectSendFileService : NSObject
- (void)sendViaKDEConnect:(NSPasteboard *)pboard userData:(NSString *)userData error:(NSString **)error;
@end

static KDEConnectSendFileService *sendFileService = nil;

static void cleanup_service()
{
    if (sendFileService != nil)
        [sendFileService release];

    sendFileService = nil;
}

void registerServices() {
    NSLog(@"Registering KDE Connect Send File Service");
    KDEConnectSendFileService *sendFileService = [[KDEConnectSendFileService alloc] init]; 
    qAddPostRoutine(cleanup_service);   // Remove after quit

    NSRegisterServicesProvider(sendFileService, @"SendViaKDEConnect");

    NSLog(@"KDE Connect Send File Service registered");
}

@implementation KDEConnectSendFileService

- (void)sendViaKDEConnect:(NSPasteboard *)pboard userData:(NSString *)userData error:(NSString **)error {
    Q_UNUSED(userData);
    Q_UNUSED(error);

    NSAlert *alert = nil;
    // NSPasteboardTypeFileURL is first introduced in macOS 13, avoid to use it now
    NSArray *filePathArray = [pboard propertyListForType:NSFilenamesPboardType];
    for (NSString *path in filePathArray) {
        BOOL isDirectory = NO;
        if ([[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDirectory]) {
            if (!isDirectory) {
                if (QFile::exists(QCoreApplication::applicationDirPath() + QStringLiteral("/kdeconnect-handler"))) {
                    QProcess kdeconnect_handler;
                    kdeconnect_handler.setProgram(QCoreApplication::applicationDirPath() + QStringLiteral("/kdeconnect-handler"));
                    kdeconnect_handler.setArguments({QString::fromNSString(path)});
                    kdeconnect_handler.startDetached();
                } else {
                    alert = [[NSAlert alloc] init];
                    [alert setInformativeText:@"Cannot find kdeconnect-handler"];
                }
            } else {
                alert = [[NSAlert alloc] init];
                [alert setInformativeText:@"Cannot share a directory"];
            }
        }
        break;  // Now we only support single file sharing
    }

    if ([filePathArray count] < 1) {
        alert = [[NSAlert alloc] init];
        [alert setInformativeText:@"Cannot share selected item"];
    }

    if (alert) {
        [alert setMessageText:@"Share file failed"];
        [alert addButtonWithTitle:@"OK"];
        [alert setAlertStyle:NSAlertStyleCritical];
        [alert runModal];
        [alert release];
    }
}

@end