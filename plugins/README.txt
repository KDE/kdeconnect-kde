Writting a plugin for KDE Connect
=================================

For the desktop client (this project):
--------------------------------------

1. Enter the "plugins" directory.
2. Copy the "ping" under a different name ("findmyphone" in this example).
3. Add "add_subdirectory(findmyphone)" to CMakeLists.txt after the others "add_subdirectory".
1. Enter the new "findmyphone" directory.
5. Edit CMakeLists.txt by replacing "ping" with "findmyphone".
6. Rename other files in this directory by replacing "ping" with "findmyphone"
7. Write a description of your plugin into "README"
8. Edit findmyphoneplugin.cpp and findmyphoneplugin.h.
  A. Change license header.
  B. Replace (case sensitive) "ping" with "findmyphone", "PingPlugin" with "FindMyPhonePlugin" and "PING" with "FINDMYPHONE".
9. Edit kdeconnect_findmyphone.desktop file:
  A. Replace "ping" with "findmyphone".
  B. Change name, description, icon, author, email, version, website, license info.
  C. Remove all the translations
  D. Set X-KDEConnect-SupportedPackageType and X-KDEConnect-OutgoingPackageType to the package type your plugin will receive
     and send, respectively. In this example this is "kdeconnect.findmyphone". Make sure that this matches what is defined in
     the findmyplugin.h file (in the line "#define PACKAGE_TYPE_..."), and also in Android.
10. Now you have an empty skeleton to implement your new plugin logic.

For Android (project kdeconnect-android):
-----------------------------------------

1. Change directory to src/org/kde/kdeconnect/Plugins.
2. Copy "PingPlugin" under a different name ("FindMyPhonePlugin" in this example).
1. Enter the new "FindMyPhonePlugin" directory.
4. Rename "PingPlugin.java" to "FindMyPhonePlugin.java"
5. Edit it. Replace (case sensitive) "Ping" with "FindMyPhone", "ping" with "findmyphone", "PING" with "FINDMYPHONE"
   and "plugin_ping" with "plugin_findmyphone".
6. Open res/values/strings.xml. Find and copy the lines "pref_plugin_ping_desc" and "pref_plugin_ping" replacing "ping"
   with "findmyphone" and edit the plugin name and description between <string> </string>).
7. Open src/org/kde/kdeconnect/Plugins/PluginFactory.java.
  A. Copy "import … PingPlugin" line with replacing "PingPlugin" with "FindMyPhonePlugin".
  B. Copy "PluginFactory.registerPlugin(PingPlugin.class);" line with replacing "PingPlugin" with "FindMyPhonePlugin".
8. Open src/org/kde/kdeconnect/NetworkPackage.java. Copy a "public final static String PACKAGE_TYPE_PING = …" line
   replacing "PING" with the package type you will be using (should match the desktop client).
9. Now you have an empty skeleton to implement your new plugin logic.
