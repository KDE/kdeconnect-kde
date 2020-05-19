/*
 * Copyright 2020 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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