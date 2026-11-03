#ifndef AppVersion
#define AppVersion "0.1.0-preview.1"
#endif
#ifndef ProjectDir
#error ProjectDir is required
#endif
[Setup]
AppId={{D4B0C202-86A7-4FCA-823C-3D61308E0015}
AppName=QQ 音乐参数 EQ
AppVersion={#AppVersion}
AppPublisher=QQMusicParametricEQ contributors
DefaultDirName={localappdata}\QQMusicParametricEQ
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir={#ProjectDir}\dist
OutputBaseFilename=QQMusicParametricEQ-{#AppVersion}-windows-x86-setup
Compression=lzma2
SolidCompression=yes
UninstallDisplayName=QQ 音乐参数 EQ
LicenseFile={#ProjectDir}\LICENSE
CloseApplications=yes
RestartApplications=no
[Files]
Source: "{#ProjectDir}\dist\dsp_qqparametric.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProjectDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion isreadme
Source: "{#ProjectDir}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
[Code]
procedure CurPageChanged(CurPageID: Integer);
begin
 if CurPageID <> wpFinished then Exit;
 WizardForm.FinishedLabel.Caption := '插件已安装。打开 QQ 音乐的 设置 → 音效插件 → 添加插件，选择：' + #13#10 + ExpandConstant('{app}') + #13#10 + '勾选 QQMusic Parametric EQ，然后点击 配置插件。独占输出使用 QQ 音乐原生 设置 → 音频设备 → WASAPI → 独占模式。';
end;
