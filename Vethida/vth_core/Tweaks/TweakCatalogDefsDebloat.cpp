#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsDebloat(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        // The id says "removal" but the ops only set service Start values, with
        // restore values, so nothing is uninstalled. The id stays because saved tweak
        // state is keyed on it. WebView2 is a separate runtime and isn't touched.
        t.id = QStringLiteral("deb.apps.edge_removal");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Disable Microsoft Edge update and elevation services");
        t.description = QStringLiteral("Disables Microsoft Edge's three background update and elevation services (edgeupdate, edgeupdatem, MicrosoftEdgeElevationService) so Edge stops auto-updating and running its elevation helper. Does not uninstall Edge, remove file associations, or delete install directories.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Edge stops receiving security updates while this is applied; revert before browsing with it"),
            QStringLiteral("A Windows feature update or an Edge repair can re-enable these services"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MicrosoftEdgeElevationService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\edgeupdate"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\edgeupdatem"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.background.apps");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Background apps");
        t.name = QStringLiteral("Disable background access for Edge, settings, SecHealth and store");
        t.description = QStringLiteral("Disables background access (Disabled=1, DisabledByUser=1) for Microsoft Edge, Settings (immersivecontrolpanel), Windows Security (SecHealthUI), and Microsoft Store.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Windows Security background tasks and Defender definition refreshes may be delayed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\Microsoft.MicrosoftEdge_8wekyb3d8bbwe"), QStringLiteral("Disabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\Microsoft.MicrosoftEdge_8wekyb3d8bbwe"), QStringLiteral("DisabledByUser"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\windows.immersivecontrolpanel_cw5n1h2txyewy"), QStringLiteral("Disabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\windows.immersivecontrolpanel_cw5n1h2txyewy"), QStringLiteral("DisabledByUser"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\Microsoft.Windows.SecHealthUI_cw5n1h2txyewy"), QStringLiteral("Disabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\Microsoft.Windows.SecHealthUI_cw5n1h2txyewy"), QStringLiteral("DisabledByUser"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\Microsoft.WindowsStore_8wekyb3d8bbwe"), QStringLiteral("Disabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\Microsoft.WindowsStore_8wekyb3d8bbwe"), QStringLiteral("DisabledByUser"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.cortana");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Disable Cortana");
        t.description = QStringLiteral("Disables Cortana via the AllowCortana group policy and per-user search flags. Cortana is deprecated on modern Windows 11, so effect is mostly cosmetic on current builds.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Voice activation and Cortana search results stop working until this is reverted"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("AllowCortana"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowCortana"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("CanCortanaBeEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("CortanaCapabilities"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("CortanaEnabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.app_run_config");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Context menu");
        t.name = QStringLiteral("Add application run configuration menu");
        t.description = QStringLiteral("Adds a context menu for EXE files with: Run with Priority (Realtime, High, Above Normal, Normal, Below Normal, Low), Configure Internet Access (Block/Restore firewall rules), Set GPU usage (Dedicated/Integrated/Reset), and Run as TrustedInstaller.");
        t.iconName = QStringLiteral("menu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.registryOps = {
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig"), QStringLiteral("MUIVerb"), QStringLiteral("Config Application Run"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig"), QStringLiteral("Icon"), QStringLiteral("taskmgr.exe,1"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio"), QStringLiteral("MUIVerb"), QStringLiteral("Run with Priority"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio"), QStringLiteral("Icon"), QStringLiteral("taskmgr.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\001flyout"), QStringLiteral(""), QStringLiteral("Realtime"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\001flyout\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /c start \"\" /Realtime \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\002flyout"), QStringLiteral(""), QStringLiteral("High"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\002flyout\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /c start \"\" /High \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\003flyout"), QStringLiteral(""), QStringLiteral("Above normal"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\003flyout\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /c start \"\" /AboveNormal \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\004flyout"), QStringLiteral(""), QStringLiteral("Normal"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\004flyout\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /c start \"\" /Normal \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\005flyout"), QStringLiteral(""), QStringLiteral("Below normal"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\005flyout\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /c start \"\" /BelowNormal \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\006flyout"), QStringLiteral(""), QStringLiteral("Low"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\RunPrio\\shell\\006flyout\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /c start \"\" /Low \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig\\shell\\WindowsFirewall"), QStringLiteral("MUIVerb"), QStringLiteral("Configure Internet Access..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig\\shell\\WindowsFirewall"), QStringLiteral("icon"), QStringLiteral("%SystemRoot%\\system32\\FirewallControlPanel.dll,0"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig\\shell\\WindowsFirewall"), QStringLiteral("subcommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig\\shell\\WindowsFirewall\\Shell\\block"), QStringLiteral("MUIVerb"), QStringLiteral("Block internet access"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig\\shell\\WindowsFirewall\\Shell\\block"), QStringLiteral("icon"), QStringLiteral("%SystemRoot%\\system32\\imageres.dll,100"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig\\shell\\WindowsFirewall\\Shell\\block\\command"), QStringLiteral(""), QStringLiteral("\"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe\" -Executionpolicy ByPass -WindowStyle Hidden -NoLogo -Command \"start powershell -Verb runas -ArgumentList \\\"-NoLogo -WindowStyle Hidden -command `\\\"New-NetFirewallRule -DisplayName ([System.IO.Path]::GetFilenameWithoutExtension('%1')) -Name '%1' -Enabled True -Direction Outbound -Action Block -Program '%1'`\\\"\\\"\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig\\shell\\WindowsFirewall\\Shell\\Remove"), QStringLiteral("MUIVerb"), QStringLiteral("Restore the internet access"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig\\shell\\WindowsFirewall\\Shell\\Remove"), QStringLiteral("icon"), QStringLiteral("%SystemRoot%\\system32\\imageres.dll,101"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfig\\shell\\WindowsFirewall\\Shell\\Remove\\command"), QStringLiteral(""), QStringLiteral("\"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe\" -Executionpolicy ByPass -WindowStyle Hidden -NoLogo -Command \"start powershell -Verb runas -ArgumentList \\\"-NoLogo -WindowStyle Hidden -command `\\\"Remove-NetFirewallRule -Name '%1'`\\\"\\\"\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\GPU"), QStringLiteral("MUIVerb"), QStringLiteral("Set GPU usage..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\GPU"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\GPU"), QStringLiteral("Icon"), QStringLiteral("dxdiag.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\GPU\\shell\\001flyout"), QStringLiteral(""), QStringLiteral("Set to Dedicated GPU"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\GPU\\shell\\001flyout\\command"), QStringLiteral(""), QStringLiteral("C:\\Windows\\System32\\REG.exe ADD HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\DirectX\\UserGpuPreferences /f /v \"%1\" /d GpuPreference=2;"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\GPU\\shell\\002flyout"), QStringLiteral(""), QStringLiteral("Set to Integrated GPU"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\GPU\\shell\\002flyout\\command"), QStringLiteral(""), QStringLiteral("C:\\Windows\\System32\\REG.exe ADD HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\DirectX\\UserGpuPreferences /f /v \"%1\" /d GpuPreference=1;"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\GPU\\shell\\003flyout"), QStringLiteral(""), QStringLiteral("Reset GPU usage..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfig\\shell\\GPU\\shell\\003flyout\\command"), QStringLiteral(""), QStringLiteral("C:\\Windows\\System32\\REG.exe DELETE HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\DirectX\\UserGpuPreferences /v \"%1\" /f"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfigAs"), QStringLiteral("MUIVerb"), QStringLiteral("Run App As..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfigAs"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\shell\\AppRunConfigAs"), QStringLiteral("Icon"), QStringLiteral("taskmgr.exe,1"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfigAs\\shell\\TrusterdIns"), QStringLiteral("MUIVerb"), QStringLiteral("Run as trustedinstaller..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfigAs\\shell\\TrusterdIns"), QStringLiteral("HasLUAShield"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\exefile\\Shell\\AppRunConfigAs\\shell\\TrusterdIns"), QStringLiteral("Icon"), QStringLiteral("powershell.exe,0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.dll_msi_utils");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Context menu");
        t.name = QStringLiteral("Add register DLL/OCX and extract MSI");
        t.description = QStringLiteral("Adds Register and Unregister context menu options for DLL and OCX files (using regsvr32), an Extract option for MSI packages, and a TXT file option in the New menu.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Adds shell verbs that invoke regsvr32 and msiexec; right-clicking an untrusted DLL can register malicious code"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCR\\dllfile\\shell\\Register\\Command"), QStringLiteral(""), QStringLiteral("regsvr32.exe \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\dllfile\\shell\\Unregister\\Command"), QStringLiteral(""), QStringLiteral("regsvr32.exe /u \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\ocxfile\\shell\\Register\\Command"), QStringLiteral(""), QStringLiteral("regsvr32.exe \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\ocxfile\\shell\\Unregister\\Command"), QStringLiteral(""), QStringLiteral("regsvr32.exe /u \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Msi.Package\\shell\\Extract\\command"), QStringLiteral(""), QStringLiteral("msiexec.exe /a \"%1\" /qb TARGETDIR=\"%1 Contents\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\.txt\\ShellNew"), QStringLiteral("NullFile"), QStringLiteral(""), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.file_management");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Context menu");
        t.name = QStringLiteral("Add file and folder management menu");
        t.description = QStringLiteral("Adds a File Management submenu with: permanent delete, hash calculation (SHA1/SHA256/SHA384/SHA512/MD5/RIPEMD160), copy content to clipboard, and file visibility toggle. Adds a Folder Management submenu with: empty folder and permanent delete.");
        t.iconName = QStringLiteral("menu");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.registryOps = {
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement"), QStringLiteral("Icon"), QStringLiteral("shell32.dll,0"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement"), QStringLiteral("MUIVerb"), QStringLiteral("File Management"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement"), QStringLiteral("Position"), QStringLiteral("Middle"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\Windows.PermanentDelete"), QStringLiteral("CommandStateSync"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\Windows.PermanentDelete"), QStringLiteral("ExplorerCommandHandler"), QStringLiteral("{E9571AB2-AD92-4ec6-8924-4E5AD33790F5}"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\Windows.PermanentDelete"), QStringLiteral("Icon"), QStringLiteral("shell32.dll,-240"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\Windows.PermanentDelete"), QStringLiteral("Position"), QStringLiteral("Bottom"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\GetFileHash"), QStringLiteral("MUIVerb"), QStringLiteral("Calculate Hash"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\GetFileHash"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\GetFileHash\\shell\\01SHA1"), QStringLiteral("MUIVerb"), QStringLiteral("SHA1"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\GetFileHash\\shell\\01SHA1\\command"), QStringLiteral(""), QStringLiteral("powershell.exe -noexit get-filehash -literalpath '%1' -algorithm SHA1 | format-list"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\GetFileHash\\shell\\02SHA256"), QStringLiteral("MUIVerb"), QStringLiteral("SHA256"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\GetFileHash\\shell\\02SHA256\\command"), QStringLiteral(""), QStringLiteral("powershell.exe -noexit get-filehash -literalpath '%1' -algorithm SHA256 | format-list"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\GetFileHash\\shell\\06MD5"), QStringLiteral("MUIVerb"), QStringLiteral("MD5"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\GetFileHash\\shell\\06MD5\\command"), QStringLiteral(""), QStringLiteral("powershell.exe -noexit get-filehash -literalpath '%1' -algorithm MD5 | format-list"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\Copy Content to Clipboard"), QStringLiteral("MUIVerb"), QStringLiteral("Copy Content to Clipboard"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\Copy Content to Clipboard"), QStringLiteral("Icon"), QStringLiteral("DxpTaskSync.dll,-52"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\Copy Content to Clipboard\\Command"), QStringLiteral(""), QStringLiteral("cmd /c clip < \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\HiddenAttribute"), QStringLiteral("MUIVerb"), QStringLiteral("File Visibility"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\HiddenAttribute"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\HiddenAttribute"), QStringLiteral("Icon"), QStringLiteral("imageres.dll,-5314"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\HiddenAttribute\\Shell\\Item0"), QStringLiteral("MUIVerb"), QStringLiteral("Not Hidden"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\HiddenAttribute\\Shell\\Item0\\Command"), QStringLiteral(""), QStringLiteral("attrib -s -h \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\HiddenAttribute\\Shell\\Item1"), QStringLiteral("MUIVerb"), QStringLiteral("Hidden"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\HiddenAttribute\\Shell\\Item1\\Command"), QStringLiteral(""), QStringLiteral("attrib -s +h \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\HiddenAttribute\\Shell\\Item2"), QStringLiteral("MUIVerb"), QStringLiteral("System Hidden"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\HiddenAttribute\\Shell\\Item2\\Command"), QStringLiteral(""), QStringLiteral("attrib +s +h \"%1\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement"), QStringLiteral("Icon"), QStringLiteral("explorer.exe,0"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement"), QStringLiteral("MUIVerb"), QStringLiteral("Folder Management"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement"), QStringLiteral("Position"), QStringLiteral("Middle"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement\\shell\\Windows.PermanentDelete"), QStringLiteral("CommandStateSync"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement\\shell\\Windows.PermanentDelete"), QStringLiteral("ExplorerCommandHandler"), QStringLiteral("{E9571AB2-AD92-4ec6-8924-4E5AD33790F5}"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement\\shell\\Windows.PermanentDelete"), QStringLiteral("Icon"), QStringLiteral("shell32.dll,-240"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement\\shell\\Windows.PermanentDelete"), QStringLiteral("Position"), QStringLiteral("Bottom"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.runas_admin");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Context menu");
        t.name = QStringLiteral("Add run as administrator for MSI/PS1/VBS");
        t.description = QStringLiteral("Adds Run as Administrator context menu option for MSI installer packages, PowerShell scripts (.ps1), and VBScript files (.vbs).");
        t.iconName = QStringLiteral("menu");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.registryOps = {
            stringOp(QStringLiteral("HKCR\\Msi.Package\\Shell\\runas"), QStringLiteral("HasLUAShield"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\SystemFileAssociations\\.ps1\\Shell\\runas"), QStringLiteral("HasLUAShield"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\SystemFileAssociations\\.ps1\\Shell\\runas\\command"), QStringLiteral(""), QStringLiteral("powershell.exe \"-Command\" \"if((Get-ExecutionPolicy ) -ne 'AllSigned') { Set-ExecutionPolicy -Scope Process Bypass }; & '%1'\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\VBSFile\\Shell\\runas"), QStringLiteral("HasLUAShield"), QStringLiteral(""), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.shutdown_restart");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Context menu");
        t.name = QStringLiteral("Add shutdown and restart desktop menu");
        t.description = QStringLiteral("Adds a desktop right-click menu with: Shut Down PC, Restart PC, Restart and reopen apps, Restart to Advanced Startup, Restart to UEFI/BIOS, Safe Mode options, Turn off display, Kill not responding tasks, Restart/Pause File Explorer, and Devices & Printers shortcut.");
        t.iconName = QStringLiteral("menu");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.registryOps = {
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB"), QStringLiteral("MUIVerb"), QStringLiteral("Device Shutdown"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB"), QStringLiteral("Icon"), QStringLiteral("shell32.dll,-16739"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB"), QStringLiteral("Position"), QStringLiteral("Bottom"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB\\shell\\001flyout"), QStringLiteral("MUIVerb"), QStringLiteral("Shut Down PC"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB\\shell\\001flyout\\command"), QStringLiteral(""), QStringLiteral("shutdown /s /f /t 0"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB\\shell\\002flyout"), QStringLiteral("MUIVerb"), QStringLiteral("Restart PC"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB\\shell\\002flyout\\command"), QStringLiteral(""), QStringLiteral("shutdown /r /t 0"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB\\shell\\004flyout"), QStringLiteral("MUIVerb"), QStringLiteral("Restart to Advanced Startup Options"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB\\shell\\004flyout"), QStringLiteral("HasLUAShield"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB\\shell\\004flyout\\command"), QStringLiteral(""), QStringLiteral("shutdown /r /o /f /t 0"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB\\shell\\005flyout"), QStringLiteral("MUIVerb"), QStringLiteral("Restart to UEFI/BIOS"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB\\shell\\005flyout"), QStringLiteral("HasLUAShield"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartB\\shell\\005flyout\\command"), QStringLiteral(""), QStringLiteral("shutdown /r /fw /f /t 0"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA"), QStringLiteral("MUIVerb"), QStringLiteral("Restart/Refresh System Components "), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA"), QStringLiteral("Position"), QStringLiteral("Bottom"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA"), QStringLiteral("Icon"), QStringLiteral("taskmgr.exe,2"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA"), QStringLiteral("Subcommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\KillNRTasks"), QStringLiteral("icon"), QStringLiteral("taskmgr.exe,-30651"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\KillNRTasks"), QStringLiteral("MUIverb"), QStringLiteral("Kill all not responding tasks"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\KillNRTasks"), QStringLiteral("Position"), QStringLiteral("Top"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\KillNRTasks\\command"), QStringLiteral(""), QStringLiteral("CMD.exe /C taskkill.exe /f /fi \"status eq Not Responding\" & Pause"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\Restart Explorer"), QStringLiteral("icon"), QStringLiteral("explorer.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\Restart Explorer"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\Restart Explorer"), QStringLiteral("MUIVerb"), QStringLiteral("Restart/Pause File Explorer"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\Restart Explorer"), QStringLiteral("Position"), QStringLiteral("Center"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\Restart Explorer\\shell\\01menu"), QStringLiteral("MUIVerb"), QStringLiteral("Restart File Explorer"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\Restart Explorer\\shell\\01menu"), QStringLiteral("icon"), QStringLiteral("explorer.exe,1"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\Restart Explorer\\shell\\01menu\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /c taskkill /f /im explorer.exe & start explorer.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\Restart Explorer\\shell\\02menu"), QStringLiteral("MUIVerb"), QStringLiteral("Pause File Explorer"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\Restart Explorer\\shell\\02menu"), QStringLiteral("icon"), QStringLiteral("explorer.exe,5"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\RestartA\\shell\\Restart Explorer\\shell\\02menu\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /c @echo off & echo. & echo Stopping explorer.exe process . . . & echo. & taskkill /f /im explorer.exe & echo. & echo. & echo Waiting to start explorer.exe process when you are ready . . . & pause && start explorer.exe && exit"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\DevicesAndPrinters"), QStringLiteral("MUIVerb"), QStringLiteral("Devices and Printers"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\DevicesAndPrinters"), QStringLiteral("Icon"), QStringLiteral("%systemroot%\\system32\\DeviceCenter.dll,-1"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\DesktopBackground\\Shell\\DevicesAndPrinters\\Command"), QStringLiteral(""), QStringLiteral("explorer.exe shell:::{A8A91A66-3A7D-4424-8D24-04E180695C7A}"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.take_ownership");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Context menu");
        t.name = QStringLiteral("Add take ownership context menu");
        t.description = QStringLiteral("Adds a Take Ownership option to files, folders, and drives context menus. Uses takeown and icacls to grant full control to the current user.");
        t.iconName = QStringLiteral("menu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.registryOps = {
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\0TakeOwnership"), QStringLiteral(""), QStringLiteral("Take Ownership"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\0TakeOwnership"), QStringLiteral("HasLUAShield"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\0TakeOwnership"), QStringLiteral("NoWorkingDirectory"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\0TakeOwnership"), QStringLiteral("NeverDefault"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\*\\shell\\FileManagement\\shell\\0TakeOwnership\\command"), QStringLiteral(""), QStringLiteral("powershell -windowstyle hidden -command \"Start-Process cmd -ArgumentList '/c takeown /f \\\"%1\\\" && icacls \\\"%1\\\" /grant *S-1-3-4:F /t /c /l' -Verb runAs\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement\\shell\\TakeOwnership"), QStringLiteral(""), QStringLiteral("Take Ownership"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement\\shell\\TakeOwnership"), QStringLiteral("HasLUAShield"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement\\shell\\TakeOwnership"), QStringLiteral("NoWorkingDirectory"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement\\shell\\TakeOwnership"), QStringLiteral("Position"), QStringLiteral("middle"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\FolderManagement\\shell\\TakeOwnership\\command"), QStringLiteral(""), QStringLiteral("powershell -windowstyle hidden -command \"Start-Process cmd -ArgumentList '/c takeown /f \\\"%1\\\" /r /d y && icacls \\\"%1\\\" /grant *S-1-3-4:F /t /c /l /q' -Verb runAs\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\runas"), QStringLiteral(""), QStringLiteral("Take Ownership"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\runas"), QStringLiteral("HasLUAShield"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\runas"), QStringLiteral("NoWorkingDirectory"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\runas"), QStringLiteral("Position"), QStringLiteral("middle"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\runas\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /c takeown /f \"%1\\\" /r /d y && icacls \"%1\\\" /grant *S-1-3-4:F /t /c"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.terminal_in_place");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Context menu");
        t.name = QStringLiteral("Add terminal in place context menu");
        t.description = QStringLiteral("Replaces default terminal context menus with a unified 'Open terminal in place' submenu containing PowerShell 5 and CMD options. Available on drives, directories, and directory backgrounds. Blocks the default Windows Terminal UWP entry.");
        t.iconName = QStringLiteral("menu");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Blocked"), QStringLiteral("{9F156763-7844-4DC4-B2B1-901F640F5155}"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\GalleryTerminalBehaivor"), QStringLiteral("MUIVerb"), QStringLiteral("Open terminal in place..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\GalleryTerminalBehaivor"), QStringLiteral("Icon"), QStringLiteral("cmd.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\GalleryTerminalBehaivor"), QStringLiteral("Position"), QStringLiteral("Middle"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\GalleryTerminalBehaivor"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\GalleryTerminalBehaivor\\shell\\Powershell"), QStringLiteral(""), QStringLiteral("Open PowerShell 5 Window here..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\GalleryTerminalBehaivor\\shell\\Powershell"), QStringLiteral("Icon"), QStringLiteral("powershell.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\GalleryTerminalBehaivor\\shell\\Powershell\\command"), QStringLiteral(""), QStringLiteral("powershell.exe -noexit -command Set-Location -literalPath '%V'"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\GalleryTerminalBehaivor\\shell\\cmd2"), QStringLiteral(""), QStringLiteral("Open CMD Window here..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\GalleryTerminalBehaivor\\shell\\cmd2"), QStringLiteral("Icon"), QStringLiteral("imageres.dll,-5323"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Drive\\shell\\GalleryTerminalBehaivor\\shell\\cmd2\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /s /k pushd \"%V\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\GalleryTerminalBehaivor"), QStringLiteral("MUIVerb"), QStringLiteral("Open terminal in place..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\GalleryTerminalBehaivor"), QStringLiteral("Icon"), QStringLiteral("cmd.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\GalleryTerminalBehaivor"), QStringLiteral("Position"), QStringLiteral("Middle"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\GalleryTerminalBehaivor"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\GalleryTerminalBehaivor\\shell\\Powershell"), QStringLiteral(""), QStringLiteral("Open PowerShell 5 Window here..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\GalleryTerminalBehaivor\\shell\\Powershell"), QStringLiteral("Icon"), QStringLiteral("powershell.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\GalleryTerminalBehaivor\\shell\\Powershell\\command"), QStringLiteral(""), QStringLiteral("powershell.exe -noexit -command Set-Location -literalPath '%V'"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\GalleryTerminalBehaivor\\shell\\cmd2"), QStringLiteral(""), QStringLiteral("Open CMD Window here..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\GalleryTerminalBehaivor\\shell\\cmd2"), QStringLiteral("Icon"), QStringLiteral("imageres.dll,-5323"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\shell\\GalleryTerminalBehaivor\\shell\\cmd2\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /s /k pushd \"%V\""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\Background\\shell\\GalleryTerminalBehaivor"), QStringLiteral("MUIVerb"), QStringLiteral("Open terminal in place..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\Background\\shell\\GalleryTerminalBehaivor"), QStringLiteral("Icon"), QStringLiteral("cmd.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\Background\\shell\\GalleryTerminalBehaivor"), QStringLiteral("Position"), QStringLiteral("Middle"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\Background\\shell\\GalleryTerminalBehaivor"), QStringLiteral("SubCommands"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\Background\\shell\\GalleryTerminalBehaivor\\shell\\Powershell"), QStringLiteral(""), QStringLiteral("Open PowerShell 5 Window here..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\Background\\shell\\GalleryTerminalBehaivor\\shell\\Powershell"), QStringLiteral("Icon"), QStringLiteral("powershell.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\Background\\shell\\GalleryTerminalBehaivor\\shell\\Powershell\\command"), QStringLiteral(""), QStringLiteral("powershell.exe -noexit -command Set-Location -literalPath '%V'"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\Background\\shell\\GalleryTerminalBehaivor\\shell\\cmd2"), QStringLiteral(""), QStringLiteral("Open CMD Window here..."), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\Background\\shell\\GalleryTerminalBehaivor\\shell\\cmd2"), QStringLiteral("Icon"), QStringLiteral("imageres.dll,-5323"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Directory\\Background\\shell\\GalleryTerminalBehaivor\\shell\\cmd2\\command"), QStringLiteral(""), QStringLiteral("cmd.exe /s /k pushd \"%V\""), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.appv_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable App-V client");
        t.description = QStringLiteral("Disables the Microsoft Application Virtualization client service.");
        t.iconName = QStringLiteral("package");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Virtualised App-V packages published by enterprise IT will stop launching"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AppVClient"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.apps.background_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Background apps");
        t.name = QStringLiteral("Disable all background apps");
        t.description = QStringLiteral("Globally disables all UWP/Store background app activity, freeing CPU, RAM, and network resources.");
        t.iconName = QStringLiteral("app-window");
        t.impact = Impact::Medium;
        t.warnings = {
            QStringLiteral("Store app notifications and live tiles will stop working"),
            QStringLiteral("Some Store apps may not sync data in the background"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications"), QStringLiteral("GlobalUserDisabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications"), QStringLiteral("BackgroundAppGlobalToggle"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("BackgroundAppGlobalToggle"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.fax_print");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable legacy device services");
        t.description = QStringLiteral("Disables legacy services: Fax, Smart Card, Touch Keyboard, Geolocation, and Infrared Monitor.");
        t.iconName = QStringLiteral("printer");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Smart-card logon, certificate-based VPN and some corporate sign-in flows will stop working"),
            QStringLiteral("Touch keyboard, handwriting, fax and geolocation features become unavailable"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Fax"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SCardSvr"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ScDeviceEnum"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SCPolicySvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TabletInputService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\lfsvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\irmon"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.features.feeds");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("News & Interests");
        t.name = QStringLiteral("Disable news feeds and interests");
        t.description = QStringLiteral("Disables the Windows taskbar news feed, weather widget, and interests panel.");
        t.iconName = QStringLiteral("rss");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("The taskbar weather/news widget disappears and cannot be re-enabled without reverting the policy"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Feeds"), QStringLiteral("EnableFeeds"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\NewsAndInterests\\AllowNewsAndInterests"), QStringLiteral("value"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Dsh"), QStringLiteral("AllowNewsAndInterests"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.hidden");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable hidden background services");
        t.description = QStringLiteral("Disables 8 hidden/rarely-used services: Maps, Phone, Retail Demo, Insider, WMP, Parental Controls, Payments, Mixed Reality.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Your Phone linking, offline Maps, WMP sharing and Mixed Reality features stop functioning"),
            QStringLiteral("Parental Controls and NFC payments become unavailable if they were in use"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MapsBroker"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PhoneSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RetailDemo"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\wisvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WMPNetworkSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WpcMonSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SEMgrSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SharedRealitySvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.drivers.legacy");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Drivers");
        t.name = QStringLiteral("Disable legacy drivers");
        t.description = QStringLiteral("Disables the legacy Beep and cdrom drivers that are rarely needed on modern systems.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("CD/DVD drives will not work if physically present"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Beep"), QStringLiteral("Start"), 3u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\cdrom"), QStringLiteral("Start"), 3u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.remote");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Remote access");
        t.name = QStringLiteral("Disable remote access services");
        t.description = QStringLiteral("Disables remote access services: Remote Registry, Remote Desktop, Routing, and Link Tracking.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Remote Desktop will not work until reverted"),
            QStringLiteral("Remote registry access will be blocked"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RemoteRegistry"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RemoteAccess"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SessionEnv"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TermService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\UmRdpService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TrkWks"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SharedAccess"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WinRM"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.sync");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable sync host service");
        t.description = QStringLiteral("Disables the Sync Host service that synchronizes mail, contacts, calendar, and other data.");
        t.iconName = QStringLiteral("refresh-cw");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Built-in Mail, Calendar, and People apps will not sync"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\OneSyncSvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\CDPUserSvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NetTcpPortSharing"), QStringLiteral("Start"), 3u, 4u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.telemetry");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable telemetry services");
        t.description = QStringLiteral("Demotes the DiagTrack (Connected User Experiences and Telemetry), DPS (Diagnostic Policy Service), Ndu and MapsBroker services to manual start, reducing background telemetry and diagnostics. Some troubleshooters/Feedback Hub may be affected.");
        t.iconName = QStringLiteral("activity");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Feedback Hub, Insider builds and some MDM push scenarios stop working while telemetry services are disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DiagTrack"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DPS"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ndu"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MapsBroker"), QStringLiteral("Start"), 3u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.xbox");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Xbox & Game Bar");
        t.name = QStringLiteral("Disable Xbox services");
        t.description = QStringLiteral("Disables all Xbox-related background services (Auth, Game Save, GIP, NetApi).");
        t.iconName = QStringLiteral("gamepad-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Xbox Game Pass and Xbox Live features will not work"),
            QStringLiteral("Some games may fail to launch if they require Xbox services"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\XblAuthManager"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\XblGameSave"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\XboxGipSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\XboxNetApiSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WMPNetworkSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.features.sudo");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Sudo");
        t.name = QStringLiteral("Enable Windows SUDO");
        t.description = QStringLiteral("Enables the Windows SUDO feature (inline mode) for running elevated commands from a non-admin terminal.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("SUDO feature enabled for all users"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Sudo"), QStringLiteral("Enabled"), 3u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.notifications.startup_toast");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Notifications");
        t.name = QStringLiteral("Disable startup app notification toast");
        t.description = QStringLiteral("Sets Enabled=0 for Windows.SystemToast.StartupApp to suppress the 'new startup app' notification toast.");
        t.iconName = QStringLiteral("activity");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Startup app change notifications suppressed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\Windows.SystemToast.StartupApp"), QStringLiteral("Enabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.process.devicecensus");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Block DeviceCensus.exe");
        t.description = QStringLiteral("Blocks DeviceCensus.exe from running by setting an IFEO debugger redirect, preventing Windows device census telemetry.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Device census telemetry blocked"),
            QStringLiteral("May affect Windows Update compatibility checks"),
        };
        t.registryOps = {
            // Deleted on revert: a stock install has no IFEO entry for DeviceCensus.exe,
            // and an empty Debugger value isn't the same as none.
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\DeviceCensus.exe"), QStringLiteral("Debugger"), QStringLiteral("%windir%\\System32\\taskkill.exe"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.ai_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable AI Fabric service");
        t.description = QStringLiteral("Disables the Windows AI Fabric Service (WSAIFabricSvc) by setting its start type to Disabled (value 4).");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("AI Fabric service will not start at all, on demand or otherwise."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WSAIFabricSvc"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.applocker");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable AppLocker services");
        t.description = QStringLiteral("Disables AppLocker application-whitelisting infrastructure. Enterprise-only; no-op for home users.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("AppLocker application whitelisting disabled."),
            QStringLiteral("AppID enforcement disabled."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AppID"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AppIDSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\applockerfltr"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.app_updaters");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Updaters");
        t.name = QStringLiteral("Disable 3rd-party app updater services");
        t.description = QStringLiteral("Disables auto-update services from Chrome (gupdate/gupdatem), Firefox (MozillaMaintenance), Edge (edgeupdate/edgeupdatem), Adobe (ARM + UpdateService), Hola, and Overwolf. You'll need to update these apps manually.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Chrome auto-update disabled."),
            QStringLiteral("Firefox auto-update disabled."),
            QStringLiteral("Edge auto-update disabled."),
            QStringLiteral("Adobe Reader auto-update disabled."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\gupdate"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\gupdatem"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MozillaMaintenance"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\edgeupdate"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\edgeupdatem"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AdobeARMservice"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AdobeUpdateService"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Hola Unblocker"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\OverwolfUpdater"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AzureAttestService"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.asus_oem");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("OEM bloatware");
        t.name = QStringLiteral("Disable ASUS OEM bloatware services");
        t.description = QStringLiteral("Disables 7 ASUS OEM prebuilt telemetry/updater/software-management services. Safe no-op on non-ASUS systems (SetStartMode skips missing services).");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("ASUS system utilities features removed."),
            QStringLiteral("ASUS software manager disabled."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ASUSSystemAnalysis"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ASUSSystemDiagnosis"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ASUSLinkNear"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ASUSLinkRemote"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ASUSSwitch"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AsusAppService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ASUSSoftwareManager"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.chrome_elevation");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Updaters");
        t.name = QStringLiteral("Disable Chrome elevation services");
        t.description = QStringLiteral("Disables Chrome's elevation helper services that let Chrome update itself silently without UAC prompts.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Chrome silent updates may fail without prompting."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\GoogleChromeElevationService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\GoogleChromeBetaElevationService"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.dotnet_opt");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable .NET NGEN optimization services");
        t.description = QStringLiteral("Disables the .NET background compiler services that run after Windows updates and new .NET installs.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral(".NET assembly precompilation disabled."),
            QStringLiteral("Slightly slower .NET cold starts."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\clr_optimization_v4.0.30319_64"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\clr_optimization_v4.0.30319_32"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\clr_optimization_v2.0.50727_64"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\clr_optimization_v2.0.50727_32"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.hyperv");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Virtualization");
        t.name = QStringLiteral("Disable Hyper-V integration services");
        t.description = QStringLiteral("Disables Hyper-V integration daemons that load whenever Hyper-V is enabled. Skip if you use Hyper-V VMs or WSL2.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Hyper-V VMs will lose integration features."),
            QStringLiteral("WSL2 networking may be affected."),
            QStringLiteral("Shared clipboard, time sync, and guest shutdown with VMs disabled."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\vmickvpexchange"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\vmicguestinterface"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\vmicshutdown"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\vmicheartbeat"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\vmicvmsession"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\vmicrdv"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\vmictimesync"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\vmicvss"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\HvHost"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.misc_manual");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Set misc services to manual (DoSvc, BITS, sppsvc, etc.)");
        t.description = QStringLiteral("Demotes Delivery Optimization (DoSvc), BITS, and other chatty auto-start services to Manual; enables FDResPub for better network discovery.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Delivery Optimization requires manual start."),
            QStringLiteral("BITS requires manual start (affects other downloads)."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DoSvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\BITS"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\sppsvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\StorSvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\cbdhsvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AviraPhantomVPN"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DispBrokerDesktopSvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DisplayEnhancementService"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\FDResPub"), QStringLiteral("Start"), 2u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.mixed_reality");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable Windows Mixed Reality services");
        t.description = QStringLiteral("Disables Windows Mixed Reality services. The WMR platform is discontinued; these only run if you use a WMR headset.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Mixed Reality headsets non-functional."),
            QStringLiteral("Spatial tracking removed."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MixedRealityOpenXRSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\perceptionsimulation"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SharedRealitySvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\spectrum"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SpatialGraphFilter"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.mssql_manual");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Set MSSQL and MySQL services to manual");
        t.description = QStringLiteral("Sets SQL Server Express and MySQL services to Manual so dev databases do not auto-start on every boot.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("SQL Server instances require manual start."),
            QStringLiteral("MySQL requires manual start."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SQLWriter"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SQLTELEMETRY$SQLEXPRESS"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SQLTELEMETRY"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SQLBrowser"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SQLAgent$SQLEXPRESS"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SQLSERVERAGENT"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MSSQL$SQLEXPRESS"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MySQL80"), QStringLiteral("Start"), 3u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.p2p");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Networking");
        t.name = QStringLiteral("Disable peer-to-peer networking services");
        t.description = QStringLiteral("Disables legacy peer-to-peer networking services (PNRP and BranchCache peer distribution). Rarely used on a home network.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Peer Name Resolution Protocol removed."),
            QStringLiteral("BranchCache peer distribution removed."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PNRPAutoReg"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\p2psvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\p2pimsvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PNRPsvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PeerDistSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.pen");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable pen and stylus services");
        t.description = QStringLiteral("Disables Wacom pen driver and the Windows pen service. Safe on non-pen devices.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Wacom pen input unavailable."),
            QStringLiteral("Windows pen service unavailable."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WacomPen"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PenService"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.printing");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable printing and biometric services");
        t.description = QStringLiteral("Disables the Print Spooler service, Print Notify, and the Windows Biometric Service. Safe on workstations that don't print and don't use Windows Hello fingerprint/face.");
        t.iconName = QStringLiteral("printer");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("All printing broken."),
            QStringLiteral("Windows Hello biometric login disabled."),
            QStringLiteral("Fingerprint reader non-functional."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Spooler"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PrintNotify"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WbioSrvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.stisvc");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable Windows Image Acquisition (stisvc)");
        t.description = QStringLiteral("Disables the Still Image Acquisition service used by scanners and WIA-based cameras. Safe on desktops without imaging hardware.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Scanner functionality broken."),
            QStringLiteral("WIA camera import disabled."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stisvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.troubleshooting");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable troubleshooting and error reporting services");
        t.description = QStringLiteral("Disables Windows Error Reporting (WerSvc), its control panel applet, automatic troubleshooting, and the Application Experience lookup service. All run ambient compat/error telemetry in the background.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Windows Error Reporting disabled."),
            QStringLiteral("Automatic troubleshooting disabled."),
            QStringLiteral("Application Experience data collection disabled."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TroubleshootingSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\wercplsupport"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WerSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AeLookupSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.uev");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable UE-V and App-V services");
        t.description = QStringLiteral("Disables UE-V (roaming settings) and App-V (application streaming) services. Enterprise features, unused on home PCs.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("UE-V roaming settings disabled."),
            QStringLiteral("App-V application streaming disabled."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\UevAgentDriver"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AppvStrm"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AppvVermgr"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AppvVfs"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AppVClient"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.visualstudio");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable Visual Studio services");
        t.description = QStringLiteral("Disables Visual Studio's installer elevation helper and diagnostic collector services.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("VS installer elevation service disabled."),
            QStringLiteral("VS diagnostic collector disabled."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\VSInstallerElevationService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\VSStandardCollectorService150"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.vmware_manual");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Set VMware services to manual");
        t.description = QStringLiteral("Sets VMware Workstation helper services to Manual so they only start when VMware is launched.");
        t.iconName = QStringLiteral("server");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("VMware VMs will not auto-start."),
            QStringLiteral("Shared VMs will not auto-start."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\VMAuthdService"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\VMware NAT Service"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\VMwareHostd"), QStringLiteral("Start"), 3u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.workfolders");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable Work Folders services");
        t.description = QStringLiteral("Disables enterprise file-sync (Work Folders) services. No effect on home users.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Work Folders enterprise sync unavailable."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\workfoldersvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\workfolderssvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.xsplit_updater");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Updaters");
        t.name = QStringLiteral("Disable XSplit VCam updater");
        t.description = QStringLiteral("Disables the XSplit VCam background updater service.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("XSplit VCam will not auto-update."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\XSplit_VCam_Updater"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.sync.setting_sync");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Sync");
        t.name = QStringLiteral("Disable setting sync across devices");
        t.description = QStringLiteral("Disables all setting sync policies: DisableSettingSync=2, plus DisableApplicationSettingSync, DisableAppSyncSettingSync, DisableCredentialsSettingSync, DisableDesktopThemeSettingSync, DisablePersonalizationSettingSync, DisableWebBrowserSettingSync (all set to 2).");
        t.iconName = QStringLiteral("palette");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("All cross-device setting sync disabled"),
            QStringLiteral("Application settings sync disabled"),
            QStringLiteral("Credentials sync disabled"),
            QStringLiteral("Theme and personalization sync disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync"), QStringLiteral("DisableSettingSync"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync"), QStringLiteral("DisableApplicationSettingSync"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync"), QStringLiteral("DisableAppSyncSettingSync"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync"), QStringLiteral("DisableCredentialsSettingSync"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync"), QStringLiteral("DisableDesktopThemeSettingSync"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync"), QStringLiteral("DisablePersonalizationSettingSync"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync"), QStringLiteral("DisableWebBrowserSettingSync"), 2u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.apps.copilot");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Copilot & AI");
        t.name = QStringLiteral("Disable Windows Copilot");
        t.description = QStringLiteral("Disables Windows Copilot AI assistant and removes it from the taskbar.");
        t.iconName = QStringLiteral("bot");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Windows Copilot, Click-to-Do and related AI shortcuts become unavailable for every user account"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Get-AppxPackage *copilot* | Remove-AppxPackage -AllUsers -ErrorAction SilentlyContinue; winget uninstall --id 9NHT9RB2F4HD --silent --accept-source-agreements; winget uninstall --id 9PJVPMSB6GVH --silent --accept-source-agreements") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.apps.cortana");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Disable Cortana");
        t.description = QStringLiteral("Disables Cortana and prevents it from appearing on the lock screen.");
        t.iconName = QStringLiteral("microphone-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Voice commands, reminders and lock-screen voice activation relying on Cortana stop working"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Get-AppxPackage *Microsoft.549981C3F5F10* | Remove-AppxPackage -AllUsers -ErrorAction SilentlyContinue; winget uninstall --id Microsoft.549981C3F5F10 --silent --accept-source-agreements; winget uninstall --id 9NFFX4SZZ23L --silent --accept-source-agreements") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.features.recall");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Recall");
        t.name = QStringLiteral("Disable Windows Recall");
        t.description = QStringLiteral("Disables Windows Recall AI data analysis that periodically screenshots and indexes your activity.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Recall timeline and on-device AI search across past activity are turned off for every user"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Disable-WindowsOptionalFeature -Online -FeatureName Recall -Remove -NoRestart -ErrorAction SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Enable-WindowsOptionalFeature -Online -FeatureName Recall -NoRestart -ErrorAction SilentlyContinue") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.maintenance.auto");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Scheduled tasks");
        t.name = QStringLiteral("Disable automatic maintenance (registry policy)");
        t.description = QStringLiteral("Disables Windows automatic idle-time maintenance via registry policy. Prevents overnight disk cleanup, defrag, and scheduled diagnostics from waking the system.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        CommandOp to0;
        to0.program = QStringLiteral("schtasks");
        to0.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\TaskScheduler\\Maintenance Configurator"), QStringLiteral("/Disable") };
        to0.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\TaskScheduler\\Maintenance Configurator"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to0);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.maintenance.cache_cleanup");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Cleanup");
        t.name = QStringLiteral("Clean application caches");
        t.description = QStringLiteral("Clears cached data from Discord, Spotify, browser caches, and Windows temp directories. Frees disk space and can fix performance issues caused by bloated cache files.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Open applications may need to re-download cached data"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Remove-Item -Path '%APPDATA%\\discord\\Cache\\Cache_Data\\*' -Recurse -Force -EA SilentlyContinue;Remove-Item -Path '%APPDATA%\\discord\\Code Cache\\*' -Recurse -Force -EA SilentlyContinue;Remove-Item -Path '%LOCALAPPDATA%\\Spotify\\Storage\\*' -Recurse -Force -EA SilentlyContinue;Remove-Item -Path '%TEMP%\\*' -Recurse -Force -EA SilentlyContinue;Remove-Item -Path '%LOCALAPPDATA%\\Temp\\*' -Recurse -Force -EA SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.maintenance.dx_cache_clear");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Cleanup");
        t.name = QStringLiteral("Clear DirectX shader cache");
        t.description = QStringLiteral("Deletes compiled shaders and DirectX cache files. Forces the GPU driver to recompile shaders fresh, which can fix rendering issues and reclaim disk space. One-shot action.");
        t.iconName = QStringLiteral("eraser");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes cached files/logs permanently; cannot be undone."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Remove-Item -Path '%LOCALAPPDATA%\\D3DSCache\\*' -Recurse -Force -EA SilentlyContinue;Remove-Item -Path '%LOCALAPPDATA%\\NVIDIA\\DXCache\\*' -Recurse -Force -EA SilentlyContinue;Remove-Item -Path '%LOCALAPPDATA%\\NVIDIA\\GLCache\\*' -Recurse -Force -EA SilentlyContinue;Remove-Item -Path '%LOCALAPPDATA%\\AMD\\DXCache\\*' -Recurse -Force -EA SilentlyContinue;Remove-Item -Path '%LOCALAPPDATA%\\AMD\\GLCache\\*' -Recurse -Force -EA SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.power.idle_disable");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Core parking");
        t.name = QStringLiteral("Disable processor idle states and core parking");
        t.description = QStringLiteral("Disables processor idle states (C-states) and core parking on the active power plan via powercfg. Sets IDLEDISABLE=1, minimum processor state to 100% (PROCTHROTTLEMIN=100), disables core parking (CPMINCORES=100), and disables processor performance boost mode (PERFBOOSTMODE=0). Applies to AC power only. Removes the wake-up delay before a parked core runs your game, and the frame drops that come with it, at a sustained power and heat cost.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("CPU stays at 100% minimum state with no core parking; power draw, heat and laptop fan noise rise sharply"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg -setacvalueindex SCHEME_CURRENT SUB_PROCESSOR IDLEDISABLE 1; powercfg -setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMIN 100; powercfg -setacvalueindex SCHEME_CURRENT SUB_PROCESSOR CPMINCORES 100; powercfg -setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE 0; powercfg -setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFINCTHRESHOLD 10; powercfg -setactive SCHEME_CURRENT") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg -setacvalueindex SCHEME_CURRENT SUB_PROCESSOR IDLEDISABLE 0; powercfg -setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMIN 5; powercfg -setacvalueindex SCHEME_CURRENT SUB_PROCESSOR CPMINCORES 0; powercfg -setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE 2; powercfg -setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFINCTHRESHOLD 30; powercfg -setactive SCHEME_CURRENT") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }


    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.spectre_meltdown");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Disable Spectre and Meltdown mitigations");
        t.description = QStringLiteral("Disables CPU vulnerability mitigations (Spectre, Meltdown) for maximum performance. Sets FeatureSettings, FeatureSettingsOverride, and FeatureSettingsOverrideMask.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("CPU vulnerable to Spectre/Meltdown attacks"),
            QStringLiteral("Security significantly reduced"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("FeatureSettings"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("FeatureSettingsOverride"), 3u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("FeatureSettingsOverrideMask"), 3u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsMitigation"), QStringLiteral("UserPreference"), 2u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.ps1_association");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("PowerShell script file association");
        t.description = QStringLiteral("Associates .ps1 files with a custom PowerShell handler that sets the execution policy to unrestricted and runs the script directly.");
        t.iconName = QStringLiteral("menu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.registryOps = {
            stringOp(QStringLiteral("HKCR\\.ps1"), QStringLiteral(""), QStringLiteral("Gallery.Inc.Code.PowerShell"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Gallery.Inc.Code.Powershell"), QStringLiteral(""), QStringLiteral("PowerShell Script"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Gallery.Inc.Code.Powershell\\DefaultIcon"), QStringLiteral(""), QStringLiteral("\"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe,0"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\Gallery.Inc.Code.Powershell\\shell\\open\\command"), QStringLiteral(""), QStringLiteral("\"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe\" -noLogo -ExecutionPolicy unrestricted -file \"%1\""), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.process.affinity_presets");
        t.category = QStringLiteral("debloating");
        // Persistent priority bias, opt-in only.
        t.concern = QStringLiteral("persistent per-process priority bias; opt-in only, never recommended.");
        t.name = QStringLiteral("CPU affinity presets for hybrid architectures");
        t.description = QStringLiteral("Configures process affinity hints for Intel 12th gen+ hybrid CPUs (P-cores and E-cores). Sets Thread Director hints via registry: pins known high-priority processes (games, renderers) to P-core preferred scheduling, and background/telemetry processes to E-core preferred scheduling. Uses HKLM\\\\SOFTWARE\\\\Microsoft\\\\Windows NT\\\\CurrentVersion\\\\Image File Execution Options per-process PerfOptions to bias scheduling. Applies CpuPriorityClass=1 (background) to telemetry and update processes. Does not set hard affinity masks uses scheduling priority hints compatible with all CPU architectures.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Search indexing, TrustedInstaller servicing and Defender scans run at background priority and take noticeably longer"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\DiagTrack.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\CompatTelRunner.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\WerFault.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\WerFaultSecure.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\usocoreworker.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\UsoClient.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\TiWorker.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\TrustedInstaller.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\SearchIndexer.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\SearchProtocolHost.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\SearchFilterHost.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\OneDrive.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.extended_aggressive");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Disable extended services (aggressive, full)");
        t.description = QStringLiteral("Disables the aggressive extended service set (networking, WMI, filtering, NLA, gaming, biometrics, per-user sync templates). EXPERT ONLY - includes services many features depend on.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Disables Base Filtering Engine (firewall), DHCP/DNS cache, WMI and NLA - networking, the firewall and WMI-based features will break"),
            QStringLiteral("Disables credential/biometric (Ngc) and gaming services; sign-in and Game Pass may fail. Restore before relying on these."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\BFE"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Backupper"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\CredentialEnrollmentManagerUserSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DevQueryBroker"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DeviceAssociationService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DevicesFlowUserSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dhcp"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DialogBlockingService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DsSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DsmSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DusmSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\EFS"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\FontCache"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\GamingServices"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\GamingServicesNet"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\IKEEXT"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MessagingService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MicrosoftAccountCloudUserAuthentication"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NPSMSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Netman"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NgcCtnrSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NgcSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NvLst"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\OneSyncSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PcaSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PimIndexMaintenanceSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PushToInstall"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RasMan"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\UserManager"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Wcmsvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WebClient"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WinHttpAutoProxySvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Winmgmt"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WlanSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WpdBusEnum"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WpnUserService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WudfSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\dLauncherLoopback"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\fdPHost"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\fdphost"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\fdrespub"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\netprofm"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\uhssvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\upfc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.extended_system");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Disable 32 extended system services (tier 2 debloat)");
        t.description = QStringLiteral("Disables 32 less-common Windows services (including 9 per-user service templates) to reclaim background RAM/CPU. Advanced users only - touches telephony, sensors, font cache v3, Netlogon, IIS, themes, clipboard/messaging/user-data per-user services.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Touch keyboard unavailable."),
            QStringLiteral("Windows themes disabled."),
            QStringLiteral("Image acquisition broken."),
            QStringLiteral("Mobile hotspot disabled."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TapiSrv"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\FontCache3.0.0.0"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WEPHOSTSVC"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Wecsvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SensorDataService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SensrSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\StiSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\autotimesvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MicrosoftEdgeElevationService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\icssvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NfsClnt"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\wisvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\defragsvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AppReadiness"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\FrameServer"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\InstallService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\OSRSS"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\sedsvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TabletInputService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Themes"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Netlogon"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ssh-agent"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\W3SVC"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ConsentUxUserSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DevicePickerUserSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\UnistoreSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DevicesFlowUserSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MessagingService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\UserDataSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DeviceAssociationBrokerSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\cbdhsvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\CaptureService"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.comprehensive_disable");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Comprehensive service disable (aggressive)");
        t.description = QStringLiteral("Demotes 38 Windows services to manual start in one operation, across networking, storage, printing, remote access, diagnostics, Windows Update delivery and Hyper-V. An aggressive superset of the individual service-disable tweaks.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Demotes 38 services to manual start, including Windows Update, printing, file sharing and Hyper-V; anything that depends on one starting by itself stops working"),
            QStringLiteral("High risk of leaving the system unable to boot cleanly or receive security updates until reverted"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\luafv"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MsLldp"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Ndu"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PEAUTH"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\rspndr"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\tcpipreg"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\lltdio"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\afunix"), QStringLiteral("Start"), 3u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NetBIOS"), QStringLiteral("Start"), 3u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NetBT"), QStringLiteral("Start"), 3u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\wanarp"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\wcifs"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\dosvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\UsoSvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WindowsTrustedRTProxy"), QStringLiteral("Start"), 3u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Wof"), QStringLiteral("Start"), 3u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\edgeupdate"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Themes"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\StorSvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RemoteRegistry"), QStringLiteral("Start"), 3u, 4u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DPS"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SENS"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MapsBroker"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MsKeyboardFilter"), QStringLiteral("Start"), 3u, 4u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NetTcpPortSharing"), QStringLiteral("Start"), 3u, 4u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ssh-agent"), QStringLiteral("Start"), 3u, 4u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Spooler"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RemoteAccess"), QStringLiteral("Start"), 3u, 4u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\shpamsvc"), QStringLiteral("Start"), 3u, 4u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ShellHWDetection"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SysMain"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\OneSyncSvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WpnUserService"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\CDPSvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\CDPUserSvc"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\vmms"), QStringLiteral("Start"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\tzautoupdate"), QStringLiteral("Start"), 3u, 4u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.smartscreen");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Disable SmartScreen");
        t.description = QStringLiteral("Disables Windows SmartScreen filter for files, browser, and shell. Also disables Store auto-download at the policy level.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("SmartScreen no longer warns about unrecognized downloads; phishing and malware from untrusted sources run unchallenged"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SmartScreen"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("EnableSmartScreen"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System\\Browser"), QStringLiteral("AllowSmartScreen"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System\\Browser"), QStringLiteral("PreventSmartScreenPromptOverrideForFiles"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System\\Browser"), QStringLiteral("EnableSmartScreenInShell"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System\\Browser"), QStringLiteral("PreventOverrideForFilesInShell"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System\\Browser"), QStringLiteral("PreventSmartScreenPromptOverride"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System\\SmartScreen"), QStringLiteral("AllowSmartScreen"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System\\SmartScreen"), QStringLiteral("PreventSmartScreenPromptOverrideForFiles"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System\\SmartScreen"), QStringLiteral("EnableSmartScreenInShell"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System\\SmartScreen"), QStringLiteral("PreventOverrideForFilesInShell"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System\\SmartScreen"), QStringLiteral("PreventSmartScreenPromptOverride"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.apps.bloatware");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove UWP bloatware");
        t.description = QStringLiteral("Removes a curated list of pre-installed UWP/AppX packages - Bing apps, Xbox apps, Teams, Skype, Zune, 3DViewer, Solitaire, YourPhone, Clipchamp and the rest of the standard bloat surface.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Removed apps cannot be reinstalled without the Microsoft Store."),
            QStringLiteral("Xbox Game Pass / Clipchamp / OneNote integration may break."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("takeown.exe /F \"C:\\Program Files\\WindowsApps\" /A; icacls.exe \"C:\\Program Files\\WindowsApps\" /grant:r \"$env:USERNAME:(OI)(CI)F\"") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.apps.hello");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Windows Hello and Quick Assist");
        t.description = QStringLiteral("Removes Windows Hello face recognition and Quick Assist remote help via Windows package removal.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Face and fingerprint sign-in stop working; only PIN or password can unlock the device afterwards"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Get-WindowsPackage -Online | Where-Object PackageName -like '*Hello-Face*' | Remove-WindowsPackage -Online -NoRestart -ErrorAction SilentlyContinue; Get-WindowsPackage -Online | Where-Object PackageName -like '*QuickAssist*' | Remove-WindowsPackage -Online -NoRestart -ErrorAction SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.apps.onedrive");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Uninstall OneDrive");
        t.description = QStringLiteral("Completely uninstalls OneDrive via the system OneDriveSetup uninstaller.");
        t.iconName = QStringLiteral("download");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Unsynced local OneDrive files and folder redirection are lost if uninstalled before a full cloud sync completes"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Start-Process -Wait -FilePath \"$env:SystemRoot\\System32\\OneDriveSetup.exe\" -ArgumentList '/uninstall' -ErrorAction SilentlyContinue; Start-Process -Wait -FilePath \"$env:SystemRoot\\SysWOW64\\OneDriveSetup.exe\" -ArgumentList '/uninstall' -ErrorAction SilentlyContinue; Remove-Item -Path 'C:\\OneDriveTemp' -Recurse -Force -ErrorAction SilentlyContinue; Remove-Item -Path \"$env:USERPROFILE\\OneDrive\" -Recurse -Force -ErrorAction SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.feeds");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Windows Feeds manager");
        t.description = QStringLiteral("Removes Feeds Manager, social info provider integration, and related CLSID entries.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{5F6C276C-EA40-4C9C-B1F0-D41B9B835B94}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.file_history");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove File History feature");
        t.description = QStringLiteral("Removes File History shell integration, CLSID registrations, and offline files command entries.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{a9a5b8d2-7c09-4971-b100-6132e0e7d426}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{B98A2BEA-7D42-4558-8BD1-832F41BAC6FD}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{62BE5D10-60EB-11CE-9F53-0020AF0BA770}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.help_pane");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Windows help system");
        t.description = QStringLiteral("Removes Windows Help Pane, MSHelp protocol handler, and associated shell extensions.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\ms-help','Registry::HKEY_CLASSES_ROOT\\CLSID\\{8cec58ae-07a1-11d9-b15e-000d56bfe6ee}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.homegroup");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Windows HomeGroup");
        t.description = QStringLiteral("Removes HomeGroup shell integration, CLSID entries, namespace registrations, and HomeGroup-related services.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{B4FB3F98-C1EA-428d-A78A-D1F5659CBA93}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace\\{B4FB3F98-C1EA-428d-A78A-D1F5659CBA93}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.ink_workspace");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Windows Ink Workspace");
        t.description = QStringLiteral("Removes Windows Ink Workspace and handwriting-related CLSID entries.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{D710F620-6778-48b8-B981-D78ED9E09527}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.legacy_components");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove legacy Windows components");
        t.description = QStringLiteral("Removes AllJoyn, Disk Quota UI, PLA (Performance Logs), WinSAT, Tablet PC control panel, TTS control panel, Phone/Modem control panel, Mobility Center, GodMode, and other unused system component registrations.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{5ea4f148-308c-46d7-98a9-49041b1dd468}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{ED7BA470-8E54-465E-825C-99712043E01C}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{87D66A43-7B11-4A28-9811-C86EE395ACF7}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.media_codecs");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove unnecessary media components");
        t.description = QStringLiteral("Removes unused video/audio codecs, Windows DVR support, TV Tuner support, and legacy media-related CLSID entries.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{B8C12012-2B70-4e60-8AA3-02A6EF2F0089}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.meet_now");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Meet Now feature");
        t.description = QStringLiteral("Removes Meet Now protocol handlers and runtime class entries.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\ms-meetnow','Registry::HKEY_CLASSES_ROOT\\ms-meetnowflyout')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.photo_viewer");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Windows Photo Viewer");
        t.description = QStringLiteral("Removes legacy Windows Photo Viewer file associations, CLSID entries, and image format handler registrations.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{FFE2A43C-56B9-4bf5-9A79-CC6D4285608A}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{76FDA0B0-6E6B-11d1-A9E0-00C04FD7085A}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.previous_versions");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove previous versions feature");
        t.description = QStringLiteral("Removes Previous Versions/Shadow Copy shell integration and property sheet handlers.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{596AB062-B4D2-4215-9F74-E9109B0A8153}','Registry::HKEY_CLASSES_ROOT\\AllFilesystemObjects\\shellex\\PropertySheetHandlers\\{596AB062-B4D2-4215-9F74-E9109B0A8153}','Registry::HKEY_CLASSES_ROOT\\Directory\\shellex\\PropertySheetHandlers\\{596AB062-B4D2-4215-9F74-E9109B0A8153}','Registry::HKEY_CLASSES_ROOT\\Drive\\shellex\\PropertySheetHandlers\\{596AB062-B4D2-4215-9F74-E9109B0A8153}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.retail_demo");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Retail Demo mode");
        t.description = QStringLiteral("Removes Retail Demo mode registry entries, protocols, AppX packages, and scheduled task references.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RetailDemo')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.search_shell");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Windows Search shell components");
        t.description = QStringLiteral("Removes Search App, SearchFolder, auto-suggestion CLSIDs, and related shell extension registrations.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{04731B67-D933-450a-90E6-4ACD2E9408FE}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{1F3427C8-5C10-4210-AA03-2EE45287D668}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.shell_features");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove unused shell features");
        t.description = QStringLiteral("Removes various unused shell features: SmartScreen protocol, SpellCheck host, Error Reporting, Scheduled Diagnostics, Windows To Go, Workplace wizards, Fax, Archive/ZIP handler, Remote Desktop protocol, Gallery folder, and other legacy Windows components.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{e88865ea-0e1c-4e20-9aa6-edcd0212c87c}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{AD182E17-4754-4742-8529-C11EEEF0C299}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{FFCDB781-D71C-4D10-BD5F-0492EAFFD90A}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.skype");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Skype service and integration");
        t.description = QStringLiteral("Removes Skype COM objects, Office integration points, Windows integration CLSIDs, and Skype service registrations.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{4CAC6328-B9B0-11D1-A1CA-00C04FD85AB4}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{9B7A0B56-FC0D-44A7-8497-E7E46AA4DBF7}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.sync_center");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Sync Center and Offline Files");
        t.description = QStringLiteral("Removes Sync Center control panel integration, cloud sync CLSID entries, and offline file management components.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{9C73F5E5-7AE7-4E32-A8E8-8D23B85255BF}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{BB06C0E4-D293-4f75-8A90-CB05B6477EEE}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SyncRootManager')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.telemetry_components");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove telemetry runtime components");
        t.description = QStringLiteral("Removes Iris telemetry, Windows Gaming telemetry, Windows Feedback, and Windows Timeline runtime class entries.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\WindowsRuntime\\ActivatableClassId\\WindowsUdk.Services.Iris.IrisService','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\WindowsRuntime\\ActivatableClassId\\Windows.Gaming.GameBar.PresenceServer.Internal.PresenceWriter')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.work_folders");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Work Folders and workplace join");
        t.description = QStringLiteral("Removes Work Folders context menu integration, Workplace Join wizards, and enterprise file sync components.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\CLSID\\{A75A0100-B76D-4D2B-B3F4-CDC1E9FDA4C6}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.xbox_gamebar");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Xbox Game Bar and GameExplorer");
        t.description = QStringLiteral("Removes Xbox Game Bar runtime contracts, GameExplorer folder registrations, and gaming-related CLSID entries.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\WindowsRuntime\\WellKnownContracts\\Windows.Media.Capture.GameBarContract','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\WindowsRuntime\\WellKnownContracts\\Windows.Media.AppBroadcasting.AppBroadcastingContract','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\WindowsRuntime\\WellKnownContracts\\Windows.Media.AppRecording.AppRecordingContract','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\WindowsRuntime\\WellKnownContracts\\Windows.Media.Capture.AppCaptureContract','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\WindowsRuntime\\WellKnownContracts\\Windows.Media.Capture.AppCaptureMetadataContract','Registry::HKEY_CLASSES_ROOT\\CLSID\\{9A5EA990-3034-4D6F-9128-01F3C61022BC}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{DBC85A2C-C0DC-4961-B6E2-D28B62C11AD4}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.remove_security_menus");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove security and encryption context menus");
        t.description = QStringLiteral("Removes BitLocker drive context menu entries, Windows Encryption EPP menus, Controlled Folder Access menu, and EFS encryption context menu handler.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\Drive\\shell\\change-passphrase','Registry::HKEY_CLASSES_ROOT\\Drive\\shell\\change-pin','Registry::HKEY_CLASSES_ROOT\\Drive\\shell\\encrypt-bde','Registry::HKEY_CLASSES_ROOT\\Drive\\shell\\encrypt-bde-elev','Registry::HKEY_CLASSES_ROOT\\Drive\\shell\\manage-bde','Registry::HKEY_CLASSES_ROOT\\Drive\\shell\\resume-bde','Registry::HKEY_CLASSES_ROOT\\Drive\\shell\\resume-bde-elev','Registry::HKEY_CLASSES_ROOT\\Drive\\shell\\unlock-bde','Registry::HKEY_CLASSES_ROOT\\*\\shell\\UpdateEncryptionSettingsWork','Registry::HKEY_CLASSES_ROOT\\Directory\\shell\\UpdateEncryptionSettings','Registry::HKEY_CLASSES_ROOT\\*\\shellex\\ContextMenuHandlers\\EPP','Registry::HKEY_CLASSES_ROOT\\Directory\\shellex\\ContextMenuHandlers\\EPP','Registry::HKEY_CLASSES_ROOT\\Drive\\shellex\\ContextMenuHandlers\\EPP','Registry::HKEY_CLASSES_ROOT\\DesktopBackground\\Shell\\ControlledFolderAccess','Registry::HKEY_CLASSES_ROOT\\CLSID\\{A470F8CF-A1E8-4f65-8335-227475AA5C46}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{A470F8CF-A1E8-4f65-8335-227475AA5C46}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Directory\\shellex\\ContextMenuHandlers\\EncryptionMenu')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.remove_sharing_menus");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove sharing and sync context menus");
        t.description = QStringLiteral("Removes OneDrive sync menu, PlayTo/Cast to Device menu, Play menu entries, Send To handler, CopyTo/MoveTo menus, and Open as Portable Device.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\Directory\\shellex\\ContextMenuHandlers\\ FileSyncEx','Registry::HKEY_CLASSES_ROOT\\*\\shellex\\ContextMenuHandlers\\ FileSyncEx','Registry::HKEY_CLASSES_ROOT\\lnkfile\\shellex\\ContextMenuHandlers\\ FileSyncEx','Registry::HKEY_CLASSES_ROOT\\CLSID\\{7AD84985-87B4-4a16-BE58-8B72A5B390F7}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{7AD84985-87B4-4a16-BE58-8B72A5B390F7}','Registry::HKEY_CLASSES_ROOT\\SystemFileAssociations\\Directory.Audio\\shellex\\ContextMenuHandlers\\PlayTo','Registry::HKEY_CLASSES_ROOT\\SystemFileAssociations\\Directory.Image\\shellex\\ContextMenuHandlers\\PlayTo','Registry::HKEY_CLASSES_ROOT\\SystemFileAssociations\\Directory.Video\\shellex\\ContextMenuHandlers\\PlayTo','Registry::HKEY_CLASSES_ROOT\\SystemFileAssociations\\audio\\shell\\Play','Registry::HKEY_CLASSES_ROOT\\SystemFileAssociations\\Directory.Audio\\shell\\Play','Registry::HKEY_CLASSES_ROOT\\SystemFileAssociations\\Directory.Image\\shell\\Play','Registry::HKEY_CLASSES_ROOT\\Stack.Audio\\shell\\Play','Registry::HKEY_CLASSES_ROOT\\Stack.Image\\shell\\Play','Registry::HKEY_CLASSES_ROOT\\AppX43hnxtbyyps62jhe9sqpdzxn1790zetc\\Shell','Registry::HKEY_CLASSES_ROOT\\AppXk0g4vb8gvt7b93tg50ybcy892pge6jmt\\Shell','Registry::HKEY_CLASSES_ROOT\\MediaCenter.WTVFile\\shell','Registry::HKEY_CLASSES_ROOT\\SystemFileAssociations\\audio\\shell','Registry::HKEY_CLASSES_ROOT\\CLSID\\{7BA4C740-9E81-11CF-99D3-00AA004AE837}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{7BA4C740-9E81-11CF-99D3-00AA004AE837}','Registry::HKEY_CLASSES_ROOT\\AllFilesystemObjects\\shellex\\ContextMenuHandlers\\SendTo','Registry::HKEY_CLASSES_ROOT\\CLSID\\{3852C2E2-4A16-4b11-8E71-F8904C37EC3D}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{AF65E2EA-3739-4e57-9C5F-7F43C949CE5E}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{AF65E2EA-3739-4e57-9C5F-7F43C949CE5E}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{3852C2E2-4A16-4b11-8E71-F8904C37EC3D}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{A0202464-B4B4-4b85-9628-CCD46DF16942}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{F60C3E02-214A-462b-9B07-56EA38545A13}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{A0202464-B4B4-4b85-9628-CCD46DF16942}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{F60C3E02-214A-462b-9B07-56EA38545A13}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Drive\\shellex\\ContextMenuHandlers\\{D6791A63-E7E2-4fee-BF52-5DED8E86E9B8}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{D6791A63-E7E2-4fee-BF52-5DED8E86E9B8}','Registry::HKEY_CLASSES_ROOT\\Drive\\shell\\Optimize using PerfectDisk')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.remove_shell_clutter");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove shell integration clutter");
        t.description = QStringLiteral("Removes unnecessary context menu entries: Edit for text/reg files, Paint 3D Edit for image files, Windows Photos shell entries, Edit with Clipchamp, Windows Media Player 11 shell menus, and NVIDIA link context menus.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\SystemFileAssociations\\text\\shell\\edit','Registry::HKEY_CLASSES_ROOT\\regfile\\shell\\edit','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.bmp\\Shell\\3D Edit','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.jpeg\\Shell\\3D Edit','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.jpe\\Shell\\3D Edit','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.jpg\\Shell\\3D Edit','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.png\\Shell\\3D Edit','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.gif\\Shell\\3D Edit','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.tif\\Shell\\3D Edit','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.tiff\\Shell\\3D Edit','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.bmp\\Shell','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.jpeg\\Shell','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.jpe\\Shell','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.jpg\\Shell','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.png\\Shell','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.gif\\Shell','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.tif\\Shell','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\SystemFileAssociations\\.tiff\\Shell','Registry::HKEY_CLASSES_ROOT\\AppX43hnxtbyyps62jhe9sqpdzxn1790zetc\\Shell\\ShellCreateVideo','Registry::HKEY_CLASSES_ROOT\\AppXk0g4vb8gvt7b93tg50ybcy892pge6jmt\\Shell\\ShellCreateVideo','Registry::HKEY_CLASSES_ROOT\\AppX43hnxtbyyps62jhe9sqpdzxn1790zetc\\Shell\\ShellEdit','Registry::HKEY_CLASSES_ROOT\\CLSID\\{AF679D58-078E-48D4-A1E3-474F10DF76B8}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{8AB635F8-9A67-4698-AB99-784AD929F3B4}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{8AB635F8-9A67-4698-AB99-784AD929F3B4}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{AF679D58-078E-48D4-A1E3-474F10DF76B8}','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.3G2\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.3GP\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.ADTS\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.AIFF\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.ASF\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.ASX\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.AU\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.AVI\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.FLAC\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.M2TS\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.m3u\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.M4A\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.MIDI\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.MK3D\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.MKA\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.MKV\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.MOV\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.MP3\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.MP4\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.MPEG\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.TTS\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.WAV\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.WAX\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.WMA\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.WMV\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.WPL\\shell','Registry::HKEY_CLASSES_ROOT\\WMP11.AssocFile.WVX\\shell','Registry::HKEY_CLASSES_ROOT\\WMP.DVR-MSFile\\shell','Registry::HKEY_CLASSES_ROOT\\WMP.WTVFile\\shell','Registry::HKEY_CLASSES_ROOT\\lnkfile\\shellex\\ContextMenuHandlers\\NvAppShExt','Registry::HKEY_CLASSES_ROOT\\lnkfile\\shellex\\ContextMenuHandlers\\OpenGLShExt')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.ctx.remove_system_menus");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove system context menus");
        t.description = QStringLiteral("Removes Print from file context menus, Previous Versions tab and context menu, Desktop Search Box handler, Enhanced Storage handler, WSL menu entries, Windows Terminal UWP menu, Troubleshoot Compatibility handler, and Burn ISO Image menu.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Deletes registry keys permanently; this cannot be undone by toggling off."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CLASSES_ROOT\\SystemFileAssociations\\image\\shell\\print','Registry::HKEY_CLASSES_ROOT\\batfile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\cmdfile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\docxfile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\fonfile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\htmlfile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\inffile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\inifile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\JSEFile\\Shell\\Print','Registry::HKEY_CLASSES_ROOT\\otffile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\pfmfile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\regfile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\rtffile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\ttcfile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\ttffile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\txtfile\\shell\\print','Registry::HKEY_CLASSES_ROOT\\VBEFile\\Shell\\Print','Registry::HKEY_CLASSES_ROOT\\VBSFile\\Shell\\Print','Registry::HKEY_CLASSES_ROOT\\WSFFile\\Shell\\Print','Registry::HKEY_CLASSES_ROOT\\CLSID\\{596AB062-B4D2-4215-9F74-E9109B0A8153}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{596AB062-B4D2-4215-9F74-E9109B0A8153}','Registry::HKEY_CLASSES_ROOT\\AllFilesystemObjects\\shellex\\PropertySheetHandlers\\{596AB062-B4D2-4215-9F74-E9109B0A8153}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}\\shellex\\PropertySheetHandlers\\{596AB062-B4D2-4215-9F74-E9109B0A8153}','Registry::HKEY_CLASSES_ROOT\\Directory\\shellex\\PropertySheetHandlers\\{596AB062-B4D2-4215-9F74-E9109B0A8153}','Registry::HKEY_CLASSES_ROOT\\Drive\\shellex\\PropertySheetHandlers\\{596AB062-B4D2-4215-9F74-E9109B0A8153}','Registry::HKEY_CLASSES_ROOT\\Directory\\shellex\\PropertySheetHandlers\\Offline Files','Registry::HKEY_CLASSES_ROOT\\Folder\\shellex\\PropertySheetHandlers\\Offline Files','Registry::HKEY_CLASSES_ROOT\\CLSID\\{69AD30A4-B8B0-43C0-82CA-2B7AFE9AA1B7}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{69AD30A4-B8B0-43C0-82CA-2B7AFE9AA1B7}','Registry::HKEY_CLASSES_ROOT\\CLSID\\{2854F705-3548-414C-A113-93E27C808C85}','Registry::HKEY_CLASSES_ROOT\\WOW6432Node\\CLSID\\{2854F705-3548-414C-A113-93E27C808C85}','Registry::HKEY_CLASSES_ROOT\\Directory\\Background\\shell\\WSL','Registry::HKEY_CLASSES_ROOT\\Directory\\shell\\WSL','Registry::HKEY_CLASSES_ROOT\\Drive\\shell\\WSL','Registry::HKEY_CLASSES_ROOT\\PackagedCom\\ClassIndex\\{9F156763-7844-4DC4-B2B1-901F640F5155}','Registry::HKEY_CLASSES_ROOT\\Msi.Package\\shellex\\ContextMenuHandlers\\Compatibility','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\cmdfile\\shellex\\ContextMenuHandlers\\Compatibility','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\exefile\\shellex\\ContextMenuHandlers\\Compatibility','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\batfile\\shellex\\ContextMenuHandlers\\Compatibility','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\{1d27f844-3a1f-4410-85ac-14651078412d}','Registry::HKEY_CLASSES_ROOT\\MSILink\\shellex\\ContextMenuHandlers\\{1d27f844-3a1f-4410-85ac-14651078412d}','Registry::HKEY_CLASSES_ROOT\\Windows.IsoFile\\shell\\burn')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.features.optional_disable");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Disable unnecessary Windows features");
        t.description = QStringLiteral("Disables optional Windows features including TFTP, TIFF filter, Containers, SMB Direct, printing services (XPS, PDF, Foundation), PowerShell v2, Telnet, Windows Search components, Remote Desktop Connection, and Defender default definitions. Re-enables DirectPlay and Legacy Components for older game compatibility.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Removed features cannot be restored without Windows source media"),
            QStringLiteral("Remote Desktop Connection and Windows Search components removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Disable-WindowsOptionalFeature -Online -FeatureName  -Remove -NoRestart -ErrorAction SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.maintenance.tempdata");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Clear temporary data");
        t.description = QStringLiteral("Clears Prefetch cache, user/system temp folders, Windows Error Reporting queue, and all Application/System/Security event logs.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Application, System and Security event logs are cleared; recent crash or audit evidence is lost"),
            QStringLiteral("Pending Windows Error Reporting uploads and prefetch history are deleted permanently"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Remove-Item -Path '%temp%\\*' -Recurse -Force -EA SilentlyContinue;Remove-Item -Path 'C:\\Windows\\Temp\\*' -Recurse -Force -EA SilentlyContinue;Remove-Item -Path 'C:\\Windows\\Prefetch\\*' -Recurse -Force -EA SilentlyContinue;Remove-Item -Path '%LOCALAPPDATA%\\Microsoft\\Windows\\WER\\ReportQueue\\*' -Recurse -Force -EA SilentlyContinue;wevtutil cl '%temp%' 2>$null;wevtutil cl 'C:\\Windows\\Temp' 2>$null;wevtutil cl 'C:\\Windows\\Prefetch' 2>$null;wevtutil cl '%LOCALAPPDATA%\\Microsoft\\Windows\\WER\\ReportQueue' 2>$null") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.app_services");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove Chrome elevation services");
        t.description = QStringLiteral("Removes Google Chrome and Chrome Beta elevation services used for browser auto-updates with elevated privileges.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Chrome silent updates may fail"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Services\\GoogleChromeBetaElevationService','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Services\\GoogleChromeElevationService')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.capture_dvr");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove capture and DVR service");
        t.description = QStringLiteral("Removes the BcastDVRUserService used for Game Bar screen recording and broadcasting.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Game Bar screen recording removed"),
            QStringLiteral("Game DVR broadcasting removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\BcastDVRUserService','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost\\BcastDVRUserService')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.ndu_remove");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove network data usage service");
        t.description = QStringLiteral("Completely removes the Ndu (Network Data Usage) monitoring driver from the system.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Network data usage monitoring permanently removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Ndu')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.system_services");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove unnecessary Windows system services");
        t.description = QStringLiteral("Removes 30+ Windows system services including DusmSvc, TermService, RpcLocator, smphost, BDESVC (BitLocker), AxInstSV, AJRouter, SysMain, bam, CldFlt, dam, RetailDemo, ALG, tzautoupdate, SmsRouter, QWAVE, CscService, svsvc, MSiSCSI, LMS, bttflt, MsKeyboardFilter, wcncsvc, fhsvc, WPDBusEnum, SEMgrSvc, PhoneSvc, SessionEnv, shpamsvc, embeddedmode, CloudBackupRestoreSvc, AppleSSD, SENS, PushToInstall, and more.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("BitLocker encryption removed (BDESVC)"),
            QStringLiteral("Remote Desktop removed (TermService)"),
            QStringLiteral("Superfetch/SysMain removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\DusmSvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\TermService','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\RpcLocator','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\smphost','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\BDESVC','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\AxInstSV','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\AJRouter','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\SysMain','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\bam','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\CldFlt','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\dam','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\hvcrash','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\RetailDemo','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\ALG','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\tzautoupdate','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\SmsRouter','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\QWAVE','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\CscService','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\svsvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\MSiSCSI','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\QWAVEdrv','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LMS','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\bttflt','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\MsKeyboardFilter','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\wcncsvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\fhsvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\WPDBusEnum','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\SEMgrSvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\PhoneSvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\SessionEnv','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\amdwps','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\shpamsvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\embeddedmode','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\CloudBackupRestoreSvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\AppleSSD','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\SENS','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\PushToInstall')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.telemetry_extended");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove extended telemetry and tracking services");
        t.description = QStringLiteral("Removes 30+ telemetry-related services including Sense (Defender ATP), lmhosts, UevAgentService, dcsvc, DialogBlockingService, Psched, gencounter, Filetrace, NDKPerf, NDKPing, storqosflt, NdisCap, PcaSvc, InventorySvc, PerfHost, UmRdpService, WdNisSvc, GpuEnergyDrv, luafv, PktMon, pla, GraphicsPerfSvc, SystemUsageReportSvc, IObitUnlocker, RasAuto, WinDefend, refsdedupsvc, lltdsvc, IpxlatCfgSvc, cloudidsvc, AMD External Events Utility, and more.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Windows Defender disabled (WinDefend)"),
            QStringLiteral("Defender ATP removed (Sense)"),
            QStringLiteral("Program Compatibility Assistant removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Sense','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\lmhosts','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\UevAgentService','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\dcsvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\DialogBlockingService','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Psched','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\gencounter','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Filetrace','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NDKPerf','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NDKPing','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\storqosflt','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NdisCap','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\PcaSvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\InventorySvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\ss_conn_service','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\ss_conn_service2','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\PerfHost','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\UmRdpService','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\WdNisSvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\GpuEnergyDrv','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\luafv','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\PktMon','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\pla','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\GraphicsPerfSvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\AMD External Events Utility','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\SystemUsageReportSvc_QUEENCREEK','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\VSStandardCollectorService150','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\IObitUnlocker','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\RasAuto','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\WinDefend','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\refsdedupsvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\lltdsvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\IpxlatCfgSvc','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\cloudidsvc','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\AMDLOG','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost\\GraphicsPerfSvcGroup')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.ucpd");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove UCPD and error device services");
        t.description = QStringLiteral("Removes UCPD (User Choice Protection Driver) which prevents changing default browser, and ErrDev error logging device.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("UCPD default browser protection removed"),
            QStringLiteral("ErrDev error logging device removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\UCPD','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\ErrDev')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.unwanted_drivers");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove unwanted driver services");
        t.description = QStringLiteral("Removes ZTDNS (Zero Trust DNS), CSC (Client-Side Caching), and AppvVemgr (App-V virtual environment manager) driver services.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Offline files (CSC) unavailable"),
            QStringLiteral("Zero Trust DNS filtering removed"),
            QStringLiteral("App-V virtual environment manager removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\ZTDNS','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\CSC','Registry::HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\AppvVemgr')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.tasks.comprehensive_disable");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Disable comprehensive Windows scheduled tasks");
        // The description names the task groups below without a count, so adding a
        // task doesn't make it wrong.
        t.description = QStringLiteral("Disables the scheduled tasks behind Windows telemetry, maintenance, updates and settings sync, using schtasks /Change /DISABLE rather than deleting the task cache. Covers App-V publishing, Data Integrity Scan, scheduled defrag, Scheduled Diagnosis, SilentCleanup, Flighting, InstallService SmartRetry, MicrosoftEdge update tasks, NetTrace, PushToInstall, SettingSync, Speech, TaskScheduler maintenance, UpdateOrchestrator, WaaSMedic, Windows Defender scan and maintenance, and WindowsUpdate.");
        t.iconName = QStringLiteral("clock-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        CommandOp to0;
        to0.program = QStringLiteral("schtasks");
        to0.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\AppV\\MonikerCache"), QStringLiteral("/Disable") };
        to0.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\AppV\\MonikerCache"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to0);
        CommandOp to1;
        to1.program = QStringLiteral("schtasks");
        to1.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\AppV\\PublishingRefreshBoot"), QStringLiteral("/Disable") };
        to1.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\AppV\\PublishingRefreshBoot"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to1);
        CommandOp to2;
        to2.program = QStringLiteral("schtasks");
        to2.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\AppV\\PublishingRefreshSchedule"), QStringLiteral("/Disable") };
        to2.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\AppV\\PublishingRefreshSchedule"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to2);
        CommandOp to3;
        to3.program = QStringLiteral("schtasks");
        to3.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\AppV\\SyncAppvPublishingServer"), QStringLiteral("/Disable") };
        to3.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\AppV\\SyncAppvPublishingServer"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to3);
        CommandOp to4;
        to4.program = QStringLiteral("schtasks");
        to4.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Data Integrity Scan\\Data Integrity Scan"), QStringLiteral("/Disable") };
        to4.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Data Integrity Scan\\Data Integrity Scan"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to4);
        CommandOp to5;
        to5.program = QStringLiteral("schtasks");
        to5.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Data Integrity Scan\\Data Integrity Scan for Crash Recovery"), QStringLiteral("/Disable") };
        to5.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Data Integrity Scan\\Data Integrity Scan for Crash Recovery"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to5);
        CommandOp to6;
        to6.program = QStringLiteral("schtasks");
        to6.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Defrag\\ScheduledDefrag"), QStringLiteral("/Disable") };
        to6.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Defrag\\ScheduledDefrag"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to6);
        CommandOp to7;
        to7.program = QStringLiteral("schtasks");
        to7.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Diagnosis\\Scheduled"), QStringLiteral("/Disable") };
        to7.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Diagnosis\\Scheduled"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to7);
        CommandOp to8;
        to8.program = QStringLiteral("schtasks");
        to8.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\DiskCleanup\\SilentCleanup"), QStringLiteral("/Disable") };
        to8.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\DiskCleanup\\SilentCleanup"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to8);
        CommandOp to9;
        to9.program = QStringLiteral("schtasks");
        to9.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Flighting\\FeatureConfig\\ReconcileFeatures"), QStringLiteral("/Disable") };
        to9.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Flighting\\FeatureConfig\\ReconcileFeatures"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to9);
        CommandOp to10;
        to10.program = QStringLiteral("schtasks");
        to10.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Flighting\\OneSettings\\RefreshCache"), QStringLiteral("/Disable") };
        to10.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Flighting\\OneSettings\\RefreshCache"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to10);
        CommandOp to11;
        to11.program = QStringLiteral("schtasks");
        to11.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\InstallService\\SmartRetry"), QStringLiteral("/Disable") };
        to11.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\InstallService\\SmartRetry"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to11);
        CommandOp to12;
        to12.program = QStringLiteral("schtasks");
        to12.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\MicrosoftEdge\\MicrosoftEdgeShadowStackRollback"), QStringLiteral("/Disable") };
        to12.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\MicrosoftEdge\\MicrosoftEdgeShadowStackRollback"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to12);
        CommandOp to13;
        to13.program = QStringLiteral("schtasks");
        to13.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\MicrosoftEdge\\MicrosoftEdgeUpdateBrowserReplacementTask"), QStringLiteral("/Disable") };
        to13.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\MicrosoftEdge\\MicrosoftEdgeUpdateBrowserReplacementTask"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to13);
        CommandOp to14;
        to14.program = QStringLiteral("schtasks");
        to14.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\MicrosoftEdge\\MicrosoftEdgeUpdateTaskMachineCore"), QStringLiteral("/Disable") };
        to14.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\MicrosoftEdge\\MicrosoftEdgeUpdateTaskMachineCore"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to14);
        CommandOp to15;
        to15.program = QStringLiteral("schtasks");
        to15.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\MicrosoftEdge\\MicrosoftEdgeUpdateTaskMachineUA"), QStringLiteral("/Disable") };
        to15.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\MicrosoftEdge\\MicrosoftEdgeUpdateTaskMachineUA"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to15);
        CommandOp to16;
        to16.program = QStringLiteral("schtasks");
        to16.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\NetTrace\\GatherNetworkInfo"), QStringLiteral("/Disable") };
        to16.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\NetTrace\\GatherNetworkInfo"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to16);
        CommandOp to17;
        to17.program = QStringLiteral("schtasks");
        to17.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\PushToInstall\\LoginCheck"), QStringLiteral("/Disable") };
        to17.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\PushToInstall\\LoginCheck"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to17);
        CommandOp to18;
        to18.program = QStringLiteral("schtasks");
        to18.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\PushToInstall\\Registration"), QStringLiteral("/Disable") };
        to18.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\PushToInstall\\Registration"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to18);
        CommandOp to19;
        to19.program = QStringLiteral("schtasks");
        to19.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\SettingSync\\BackgroundUploadTask"), QStringLiteral("/Disable") };
        to19.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\SettingSync\\BackgroundUploadTask"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to19);
        CommandOp to20;
        to20.program = QStringLiteral("schtasks");
        to20.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\SettingSync\\NetworkStateChangeTask"), QStringLiteral("/Disable") };
        to20.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\SettingSync\\NetworkStateChangeTask"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to20);
        CommandOp to21;
        to21.program = QStringLiteral("schtasks");
        to21.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Speech\\HeadsetButtonPressNLP"), QStringLiteral("/Disable") };
        to21.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Speech\\HeadsetButtonPressNLP"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to21);
        CommandOp to22;
        to22.program = QStringLiteral("schtasks");
        to22.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Speech\\SpeechModelDownloadTask"), QStringLiteral("/Disable") };
        to22.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Speech\\SpeechModelDownloadTask"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to22);
        CommandOp to23;
        to23.program = QStringLiteral("schtasks");
        to23.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\TaskScheduler\\Maintenance Configurator"), QStringLiteral("/Disable") };
        to23.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\TaskScheduler\\Maintenance Configurator"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to23);
        CommandOp to24;
        to24.program = QStringLiteral("schtasks");
        to24.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\TaskScheduler\\Regular Maintenance"), QStringLiteral("/Disable") };
        to24.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\TaskScheduler\\Regular Maintenance"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to24);
        CommandOp to25;
        to25.program = QStringLiteral("schtasks");
        to25.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\Reboot"), QStringLiteral("/Disable") };
        to25.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\Reboot"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to25);
        CommandOp to26;
        to26.program = QStringLiteral("schtasks");
        to26.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\Reboot_Battery"), QStringLiteral("/Disable") };
        to26.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\Reboot_Battery"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to26);
        CommandOp to27;
        to27.program = QStringLiteral("schtasks");
        to27.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\Report policies"), QStringLiteral("/Disable") };
        to27.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\Report policies"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to27);
        CommandOp to28;
        to28.program = QStringLiteral("schtasks");
        to28.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\Schedule Scan"), QStringLiteral("/Disable") };
        to28.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\Schedule Scan"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to28);
        CommandOp to29;
        to29.program = QStringLiteral("schtasks");
        to29.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\Schedule Scan Static Task"), QStringLiteral("/Disable") };
        to29.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\Schedule Scan Static Task"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to29);
        CommandOp to30;
        to30.program = QStringLiteral("schtasks");
        to30.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\StartInstall"), QStringLiteral("/Disable") };
        to30.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\StartInstall"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to30);
        CommandOp to31;
        to31.program = QStringLiteral("schtasks");
        to31.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\USO_UxBroker"), QStringLiteral("/Disable") };
        to31.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\UpdateOrchestrator\\USO_UxBroker"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to31);
        CommandOp to32;
        to32.program = QStringLiteral("schtasks");
        to32.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\WaaSMedic\\PlugScheduler"), QStringLiteral("/Disable") };
        to32.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\WaaSMedic\\PlugScheduler"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to32);
        CommandOp to33;
        to33.program = QStringLiteral("schtasks");
        to33.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Windows Defender\\Windows Defender Cache Maintenance"), QStringLiteral("/Disable") };
        to33.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Windows Defender\\Windows Defender Cache Maintenance"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to33);
        CommandOp to34;
        to34.program = QStringLiteral("schtasks");
        to34.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Windows Defender\\Windows Defender Cleanup"), QStringLiteral("/Disable") };
        to34.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Windows Defender\\Windows Defender Cleanup"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to34);
        CommandOp to35;
        to35.program = QStringLiteral("schtasks");
        to35.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Windows Defender\\Windows Defender Scheduled Scan"), QStringLiteral("/Disable") };
        to35.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Windows Defender\\Windows Defender Scheduled Scan"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to35);
        CommandOp to36;
        to36.program = QStringLiteral("schtasks");
        to36.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Windows Defender\\Windows Defender Verification"), QStringLiteral("/Disable") };
        to36.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\Windows Defender\\Windows Defender Verification"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to36);
        CommandOp to37;
        to37.program = QStringLiteral("schtasks");
        to37.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\WindowsUpdate\\Scheduled Start"), QStringLiteral("/Disable") };
        to37.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\WindowsUpdate\\Scheduled Start"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to37);
        CommandOp to38;
        to38.program = QStringLiteral("schtasks");
        to38.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\WindowsUpdate\\sih"), QStringLiteral("/Disable") };
        to38.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\WindowsUpdate\\sih"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to38);
        CommandOp to39;
        to39.program = QStringLiteral("schtasks");
        to39.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\WindowsUpdate\\sihboot"), QStringLiteral("/Disable") };
        to39.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Windows\\WindowsUpdate\\sihboot"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to39);
        CommandOp to40;
        to40.program = QStringLiteral("schtasks");
        to40.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\MicrosoftEdgeUpdateTaskMachineCore"), QStringLiteral("/Disable") };
        to40.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\MicrosoftEdgeUpdateTaskMachineCore"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to40);
        CommandOp to41;
        to41.program = QStringLiteral("schtasks");
        to41.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\MicrosoftEdgeUpdateTaskMachineUA"), QStringLiteral("/Disable") };
        to41.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\MicrosoftEdgeUpdateTaskMachineUA"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to41);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.tasks.enterprise");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove enterprise and management task cache");
        t.description = QStringLiteral("Removes task cache entries for Active Directory RMS Client, EnterpriseMgmt, EDP, SharedPC, Work Folders, Workplace Join, Management, and Plug and Play Sysprep tasks.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Active Directory RMS tasks removed"),
            QStringLiteral("Enterprise management tasks removed"),
            QStringLiteral("Domain workplace join tasks removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Active Directory Rights Management Services Client','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\EnterpriseMgmt','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\EDP','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\SharedPC','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Work Folders','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Workplace Join','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Management','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Plug and Play\\Sysprep Generalize Drivers')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.tasks.infrastructure");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove system infrastructure task cache");
        t.description = QStringLiteral("Removes task cache entries for SoftwareProtectionPlatform, ReFsDedupSvc, capabilityaccessmanager, CloudRestore, Containers, StateRepository, LanguageComponentsInstaller, .NET Framework, Wininet, Chkdsk, Autochk, DirectX, Printing, RecoveryEnvironment, SpacePort, and Sustainability tasks.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Disk check tasks removed"),
            QStringLiteral("Printing scheduled tasks removed"),
            QStringLiteral("Recovery environment tasks removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\SoftwareProtectionPlatform','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\SoftwareProtectionPlatform\\SvcRestartTaskLogon','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\SoftwareProtectionPlatform\\SvcRestartTaskNetwork','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\ReFsDedupSvc','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\capabilityaccessmanager','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\CloudRestore','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Containers','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\StateRepository','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\LanguageComponentsInstaller','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\.NET Framework','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Wininet','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Chkdsk','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Autochk','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\DirectX','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Printing','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\RecoveryEnvironment','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\SpacePort','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Sustainability')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.tasks.maintenance");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove maintenance and system task cache");
        t.description = QStringLiteral("Removes task cache entries for Windows Maintenance, DiskCleanup, WindowsBackup, Data Integrity Scan, Power Efficiency Diagnostics, Setup, WOF, USB, Storage Tiers, Boot cache, AppxDeploymentClient, Live, Defrag, Sysmain, and SystemRestore tasks.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Automatic maintenance tasks removed"),
            QStringLiteral("Disk cleanup tasks removed"),
            QStringLiteral("Windows Backup tasks removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Maintenance','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\TaskScheduler\\Maintenance Configurator','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskStateFlags','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Maintenance','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\DiskCleanup','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\WindowsBackup','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Data Integrity Scan','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Power Efficiency Diagnostics','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Setup','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\WOF','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\USB','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Storage Tiers Management','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Boot','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\AppxDeploymentClient\\Pre-staged app cleanup','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Live','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Defrag','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Sysmain','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\SystemRestore')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.tasks.misc");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove miscellaneous Windows task cache");
        t.description = QStringLiteral("Removes task cache entries for PushToInstall, OneCore, AppID, ApplicationData, NetTrace, Offline Files, PI, PLA, Kernel, AppListBackup, File Classification Infrastructure, FileHistory, InstallService, AppxDeploymentClient (UDP), UNP, Pluton, UPnP, User Profile Service, Windows Error Reporting, Windows Media Sharing, Windows Registry, Remote Desktop/Assistance, SyncCenter, Time Synchronization, Time Zone, Location, Maps, WSL, Xbox, and Power tasks.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("File History tasks removed"),
            QStringLiteral("Error reporting tasks removed"),
            QStringLiteral("Location tasks removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\PushToInstall','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\OneCore','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\AppID','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\ApplicationData','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\NetTrace','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Offline Files','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\PI','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\PLA','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Kernel','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\AppListBackup','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\File Classification Infrastructure','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\FileHistory','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tasks\\{E61FEADD-31CB-4052-8A16-1F4336764D10}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Logon\\{E61FEADD-31CB-4052-8A16-1F4336764D10}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Plain\\{E61FEADD-31CB-4052-8A16-1F4336764D10}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\InstallService','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\AppxDeploymentClient','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\UNP','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Pluton','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\UPnP','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\User Profile Service','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Windows Error Reporting','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Windows Media Sharing','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Registry','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\RemoteApp and Desktop Connections Update','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\RemoteAssistance','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\SyncCenter','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Time Synchronization\\SynchronizeTime','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Time Zone','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Location','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Maps','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Windows Subsystem For Linux','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\XblGameSave','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Power')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.tasks.networking");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove networking and connectivity task cache");
        t.description = QStringLiteral("Removes task cache entries for Network Connectivity Status Indicator, NlaSvc, WCM, WlanSvc, WwanSvc, Bluetooth, DUSM, Ras, BrokerInfrastructure, and PCRNF/PCRPF/NetworkExperimentation.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Network connectivity status tasks removed"),
            QStringLiteral("WiFi/WLAN tasks removed"),
            QStringLiteral("Mobile broadband (WWAN) tasks removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Network Connectivity Status Indicator','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\NlaSvc','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\WCM','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\WlanSvc','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\WwanSvc','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Bluetooth','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\DUSM','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Ras','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\BrokerInfrastructure','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\PCRNF','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\PCRPF','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\NetworkExperimentation')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.tasks.security");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove security and Defender task cache");
        t.description = QStringLiteral("Removes task cache entries for Windows Defender, Exploit Guard, Windows AI, Sense (Defender ATP), AccountHealth, Security, BitLocker, and TPM tasks.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Windows Defender scheduled scans removed"),
            QStringLiteral("Exploit Guard tasks removed"),
            QStringLiteral("BitLocker tasks removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Windows Defender','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\ExploitGuard','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\WindowsAI','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Sense','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\AccountHealth','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Security','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\BitLocker','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\TPM')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.tasks.shell");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove shell and UI task cache");
        t.description = QStringLiteral("Removes task cache entries for Shell (FamilySafetyMonitor, FamilySafetyRefreshTask, IndexerAutomaticMaintenance, ThemeAssetTask, CrawlStartPages), ConsentUX, WS, Windows Activation Technologies, TaskScheduler, SettingSync, CloudExperienceHost, Input, WindowsColorSystem, Speech, TextServicesFramework, and Task Manager.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Family Safety monitoring tasks removed"),
            QStringLiteral("Search indexer maintenance removed"),
            QStringLiteral("Theme sync tasks removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Shell\\FamilySafetyMonitor','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Shell\\FamilySafetyRefreshTask','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Shell\\IndexerAutomaticMaintenance','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Shell\\ThemeAssetTask_SyncFODState','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Shell\\CrawlStartPages','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\ConsentUX','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\WS','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Windows Activation Technologies','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\TaskScheduler','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\SettingSync','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\CloudExperienceHost','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Input','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\WindowsColorSystem','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Speech','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\TextServicesFramework','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Task Manager')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.tasks.telemetry");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove telemetry and diagnostics task cache");
        t.description = QStringLiteral("Removes task cache entries for Customer Experience Improvement Program, Device Setup, DeviceDirectoryClient, Device Information, Feedback, WDI, Servicing, Management Provisioning, DiskDiagnostic, DiskFootprint, PerformanceTrace, Diagnosis, and Application Experience.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("CEIP telemetry tasks removed"),
            QStringLiteral("Device diagnostics tasks removed"),
            QStringLiteral("Feedback collection tasks removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Customer Experience Improvement Program','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Device Setup','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\DeviceDirectoryClient','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Device Information','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Feedback','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\WDI','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Servicing','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Management\\Provisioning','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\DiskDiagnostic','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\DiskFootprint','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\PerformanceTrace','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Diagnosis','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\Application Experience','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Windows\\MemoryDiagnostic')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.tasks.thirdparty");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove third-party and Office task cache");
        t.description = QStringLiteral("Removes task cache entries for Aiseesoft Studio, VisualStudio, Microsoft Office (Feature Updates, Performance Monitor), StartAllBack Update, HidHide Updater, and ViGEmBus Updater.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Office auto-update tasks removed"),
            QStringLiteral("Third-party update tasks removed"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Aiseesoft Studio','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\VisualStudio','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Office','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Office\\Office Feature Updates','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Office\\Office Feature Updates Logon','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\Microsoft\\Office\\Office Performance Monitor','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\StartAllBack Update','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\HidHide_Updater','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree\\ViGEmBus_Updater')){Remove-Item -LiteralPath $k -Recurse -Force}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
