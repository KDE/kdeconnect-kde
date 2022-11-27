Writing a plugin for KDE Connect
=================================

For the desktop client (this project):
--------------------------------------

1. Enter the "plugins" directory.
2. Copy the "ping" under a different name ("findmyphone" in this example).
3. Add "add_subdirectory(findmyphone)" to CMakeLists.txt after the others "add_subdirectory".
4. Enter the new "findmyphone" directory.
5. Edit CMakeLists.txt by replacing "ping" with "findmyphone".
6. Rename other files in this directory by replacing "ping" with "findmyphone"
7. Write a description of your plugin into "README"
8. Edit findmyphoneplugin.cpp and findmyphoneplugin.h.
  A. Change license header.
  B. Replace (case sensitive) "ping" with "findmyphone", "PingPlugin" with "FindMyPhonePlugin" and "PING" with "FINDMYPHONE".
9. Edit kdeconnect_findmyphone.json file:
  A. Replace "ping" with "findmyphone".
  B. Change name, description, icon, author, email, version, website, license info.
  C. Remove all the translations
  D. Set X-KDEConnect-SupportedPacketType and X-KDEConnect-OutgoingPacketType to the packet type your plugin will receive
     and send, respectively. In this example this is "kdeconnect.findmyphone". Make sure that this matches what is defined in
     the findmyphoneplugin.h file (in the line "#define PACKET_TYPE_..."), and also in Android.
10. Now you have an empty skeleton to implement your new plugin logic.

For Android (project kdeconnect-android):
-----------------------------------------

1. Change directory to src/org/kde/kdeconnect/Plugins.
2. Copy "PingPlugin" under a different name ("FindMyPhonePlugin" in this example).
3. Enter the new "FindMyPhonePlugin" directory.
4. Rename "PingPlugin.java" to "FindMyPhonePlugin.java"
5. Edit it. Replace (case sensitive) "Ping" with "FindMyPhone", "ping" with "findmyphone", "PING" with "FINDMYPHONE"
   and "plugin_ping" with "plugin_findmyphone".
6. Open res/values/strings.xml. Find and copy the lines "pref_plugin_ping_desc" and "pref_plugin_ping" replacing "ping"
   with "findmyphone" and edit the plugin name and description between <string> </string>).
7. Now you have an empty skeleton to implement your new plugin logic.
