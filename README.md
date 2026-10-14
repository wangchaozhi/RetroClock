# RetroClock

一个用 Qt 6 编写的桌面时钟，灵感来自老版 Windows 自带的时钟。没有标题栏，背景透明，支持 Windows、macOS 和 Linux。

## 功能

- **指针表盘**：金属表圈，平滑扫动的秒针，表盘上带日期窗口
- **数字时钟**：大号时间，秒数用强调色，下方显示日期
- **深色 / 浅色**两套主题，不透明度可以在 50%–100% 之间调
- **系统托盘**：单击图标显示或隐藏时钟，右键打开完整设置菜单
- **开机自启动**：Windows 写注册表 Run 键，macOS 用 LaunchAgent，Linux 用 XDG autostart
- 总在最前、是否在任务栏显示、只允许运行一个实例，所有设置自动保存

## 操作

| 操作 | 作用 |
| --- | --- |
| 左键拖动 | 移动窗口 |
| 滚轮 | 缩放 |
| 双击 | 在指针表盘和数字时钟之间切换 |
| 右键（时钟或托盘图标） | 设置菜单 |
| 单击托盘图标 | 显示 / 隐藏 |
| Ctrl+Q | 退出 |

## 编译

需要 Qt 6（Widgets 模块）和 CMake 3.16 及以上版本。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<Qt 安装路径>
cmake --build build --config Release
```

也可以直接用 Qt Creator 打开 `CMakeLists.txt`。

## 下载

每次推送都会通过 GitHub Actions 编译三个平台的版本，可以在 Actions 页面的 Artifacts 里下载。推送 `v*` 标签（例如 `v2.1.0`）会自动发布 Release：

- Windows：`RetroClock-windows-x64.zip`
- macOS：`RetroClock-macos.dmg`（没有签名，第一次打开要右键选“打开”）
- Linux：`RetroClock-linux-x86_64.AppImage`

> Linux 下透明背景需要桌面开启窗口合成（compositor）；GNOME 需要装 AppIndicator 扩展才能显示托盘图标。
