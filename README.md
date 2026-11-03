# QQ 音乐参数 EQ

通过 QQ 音乐自带的音效插件接口加载 10 段参数 EQ。正常启动原 QQ 音乐即可，无需启动器、后台控制器或播放器注入。

独占输出由 QQ 音乐原生提供，本插件负责参数 EQ。两者可以一起使用。

[下载安装包](https://github.com/tutomato/QQMusicParametricEQ/releases/tag/v0.1.0-preview.1) · [全部发布](https://github.com/tutomato/QQMusicParametricEQ/releases) · [反馈问题](https://github.com/tutomato/QQMusicParametricEQ/issues)

## 安装与启用

1. 退出 QQ 音乐，运行 `QQMusicParametricEQ-…-windows-x86-setup.exe`。
2. 打开原 QQ 音乐，进入 **主菜单 → 设置 → 音效插件 → 添加插件**。
3. 选择 `%LOCALAPPDATA%\QQMusicParametricEQ` 文件夹。首次需要选择一次；以后更新同目录 DLL 即可。
4. 勾选 **QQMusic Parametric EQ**，选中该行并点击 **配置插件**。

已有其他音效插件时，可将 DLL 放进原有插件目录，保留 QQ 的原目录选择。

ZIP 中的 DLL 也可以解压到固定目录，再按上述方式添加该文件夹。宿主需要支持 Winamp DSP 插件接口；本包为 x86，用于 32 位 QQ 音乐。

## 独占输出

进入 **设置 → 音频设备**：

1. 输出设备选择你的 **WASAPI: DAC / 声卡名称**。
2. 输出格式选择原始比特，频率转换选择原始频率，声道选择原始声道数。
3. 向下滚动，选择 **独占模式**。切歌或重新播放后生效。

也可使用 QQ 音乐原生 ASIO。独占期间其他播放器不能同时占用同一设备；先暂停其他播放器，必要时退出它以释放设备。

## 参数 EQ

- 10 个频段，每段可选择钟形、低音搁架、高音搁架。
- 频率：20–20000 Hz；增益：−18～+18 dB；Q：0.2–20；总增益：−30～0 dB。
- **启用参数 EQ**：立即改变启用状态，不提交调参草稿。
- **应用调参**：提交参数修改，保持启用状态。
- **一键重置**：立即恢复默认频率、Q=1、全部增益 0 dB，保持启用状态。
- 新音频格式从第一帧使用目标 EQ；播放中调参采用短交叉过渡。没有开头淡入，也不改变采样率或帧数。
- 设置保存在 `%LOCALAPPDATA%\QQMusicParametricEQ\equalizer.ini`，更新与卸载默认保留。

每次切换实际采样率或声道数都会重新计算滤波系数。窗口底部显示宿主最近送入的格式及处理块数。

## 已验证与限制

真实接入基线：QQ 音乐 22.76、Windows x64、32 位 QQ 音乐、YULONG Aurora。已验证原生插件识别、44.1 kHz / 16 位双声道回调，以及 WASAPI 独占初始化返回成功。

DSP 支持宿主传入的 16/24/32 位整数 PCM、1–8 声道；36 组自动测试覆盖 44.1/48/96 kHz、16/24/32 位、单声道/双声道的频率响应，并检查缓冲边界、首帧增益及关闭旁路。**宿主送入的位深与采样率不等于音乐文件原始格式**；当前真实回调仅确认了 16 位，其他格式尚不能宣称已在 QQ 全链路验证。

不处理原生 DSD / DoP 直通、不提供 VST 宿主、不保证逐位直通。请避免同时启用其他均衡器或音效。

## 更新与卸载

更新前完全退出 QQ 音乐，再运行新版安装包。安装目录和配置保持不变。

卸载前在 QQ 音乐内取消勾选插件并退出 QQ 音乐，然后在 Windows **设置 → 应用**卸载 **QQ 音乐参数 EQ**。不会修改或删除 QQ 音乐主程序、歌单或音乐。QQ 的插件目录选择由 QQ 自行保存，卸载后可在 QQ 内重新选择其他插件目录。

## 构建

需要 Windows、Zig 0.13.0，可选 Inno Setup 6：

```powershell
.\scripts\build.ps1 -ZigCompiler C:\Tools\zig\zig.exe -InnoCompiler C:\Tools\InnoSetup\ISCC.exe
```

代码使用 MIT 许可，见 LICENSE。RBJ 滤波公式参考 W3C Audio EQ Cookbook，沿用 NcmWasapiExtension 的参数定义。本项目不是腾讯官方产品，不包含 QQ 音乐客户端及用户音乐。
