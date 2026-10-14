#pragma once

// 跨平台开机自启动
//   Windows : HKCU\Software\Microsoft\Windows\CurrentVersion\Run
//   macOS   : ~/Library/LaunchAgents/<id>.plist
//   Linux   : ~/.config/autostart/<id>.desktop
namespace AutoStart {
bool isSupported();
bool isEnabled();
bool setEnabled(bool on);
}
