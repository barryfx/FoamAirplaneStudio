param(
  [string]$Version = '0.1.0',
  [string]$QtBin = 'C:/Qt/6.11.1/msvc2022_64/bin',
  [string]$VisualStudio = 'C:/Program Files/Microsoft Visual Studio/18/Community',
  [string]$Iscc = "$env:LOCALAPPDATA/Programs/Inno Setup 6/ISCC.exe"
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$release = Join-Path $repo 'build/release/Release'
$output = Join-Path $repo 'dist'
$work = Join-Path $repo ('build/installer/' + [Guid]::NewGuid().ToString('N'))
$stage = Join-Path $work 'app'
New-Item -ItemType Directory -Force $stage,$output | Out-Null
foreach ($entry in (Get-Content "$repo/licenses/MANIFEST.json" -Raw | ConvertFrom-Json)) {
  if ((Get-FileHash -LiteralPath (Join-Path "$repo/licenses" $entry.file) -Algorithm SHA256).Hash -ne $entry.sha256) {
    throw "License source hash mismatch: $($entry.file)"
  }
}
if ((Get-FileHash "$repo/LICENSE").Hash -ne (Get-FileHash "$repo/licenses/GPL-3.0.txt").Hash) { throw 'Application license mismatch' }
Copy-Item -LiteralPath "$release/foamairplanestudio.exe","$release/Qt6Pdf.dll" -Destination $stage
# Stage fresh deployment files, not stale test binaries or SDK/debug artifacts.
& "$QtBin/windeployqt.exe" --release --no-translations --no-compiler-runtime --no-system-d3d-compiler --no-system-dxc-compiler "$stage/foamairplanestudio.exe" "$stage/Qt6Pdf.dll"
if ($LASTEXITCODE) { throw 'Qt deployment failed' }
Get-ChildItem "$release/TK*.dll" | Copy-Item -Destination $stage
Copy-Item -LiteralPath "$release/freetype.dll" -Destination $stage
Copy-Item -LiteralPath "$repo/resources/help","$repo/licenses" -Destination $stage -Recurse
Copy-Item -LiteralPath "$repo/THIRD_PARTY_NOTICES.md" -Destination "$stage/licenses"
Copy-Item -LiteralPath (Join-Path (Split-Path $Iscc) 'license.txt') -Destination "$stage/licenses/INNO-SETUP.txt"
$redist = Get-ChildItem "$VisualStudio/VC/Redist/MSVC" -Directory | Where-Object { $_.Name -match '^\d+\.' } | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$crt = Get-ChildItem "$($redist.FullName)/x64" -Directory -Filter 'Microsoft.VC*.CRT' | Select-Object -First 1
if (!$crt) { throw 'Release app-local CRT not found' }
Get-ChildItem -LiteralPath $crt.FullName -Filter '*.dll' | Copy-Item -Destination $stage
$toolset = Get-ChildItem "$VisualStudio/VC/Tools/MSVC" -Directory | Sort-Object Name -Descending | Select-Object -First 1
$dumpbin = Join-Path $toolset.FullName 'bin/Hostx64/x64/dumpbin.exe'
foreach ($binary in (Get-ChildItem $stage -Recurse -File | Where-Object { $_.Extension -in '.exe','.dll' })) {
  if ($binary.Name -match '(?i)(Qt6.*d|vcruntime\d+d|msvcp\d+d|ucrtbased)\.dll$') { throw "Debug runtime in package: $($binary.Name)" }
  $imports = & $dumpbin /dependents $binary.FullName
  if ($LASTEXITCODE) { throw "Cannot inspect dependencies: $($binary.Name)" }
  foreach ($line in $imports) {
    if ($line -match '^\s+([\w.-]+\.dll)\s*$') {
      $dll = $Matches[1]
      if ($dll -match '^(api-ms-|ext-ms-)') { continue }
      if (!(Test-Path (Join-Path $stage $dll)) -and !(Test-Path (Join-Path "$env:WINDIR/System32" $dll))) {
        throw "Unresolved dependency $dll in $($binary.Name)"
      }
    }
  }
}
$msLicenses = New-Item -ItemType Directory -Force "$stage/licenses/microsoft/visual-studio-runtime"
Copy-Item -LiteralPath "$VisualStudio/Licenses/1033/Redist.txt","$VisualStudio/Licenses/1033/ThirdPartyNotices.txt" -Destination $msLicenses
@'
Microsoft Visual C++ runtime DLLs are distributed unmodified from Visual Studio's
VC/Redist/MSVC/x64/Microsoft.VC*.CRT directory. They retain Microsoft's terms;
they are not relicensed under the application's GPL.
https://learn.microsoft.com/en-us/visualstudio/releases/2026/redistribution
https://visualstudio.microsoft.com/license-terms/
App-local deployment avoids installing a machine-wide runtime or requiring admin.
'@ | Set-Content "$msLicenses/README.txt" -Encoding utf8
# All license texts are available for review on the installer's acceptance page.
$review = @('FoamAirplaneStudio - application license and third-party notices',
  'Application: GPL-3.0-only. Dependencies retain their own terms.',
  'The full license collection is installed in the licenses folder.', '',
  (Get-Content "$repo/LICENSE" -Raw), (Get-Content "$repo/THIRD_PARTY_NOTICES.md" -Raw))
foreach ($file in (Get-ChildItem "$stage/licenses" -Recurse -File -Filter '*.txt' | Sort-Object FullName)) {
  $review += "`r`n--- $($file.Name) ---`r`n" + (Get-Content -LiteralPath $file.FullName -Raw)
}
$review -join "`r`n" | Set-Content "$stage/licenses/INSTALLER-LICENSES.txt" -Encoding utf8
$checks = foreach ($file in (Get-ChildItem "$stage/licenses" -Recurse -File | Sort-Object FullName)) {
  $relative = [IO.Path]::GetRelativePath("$stage/licenses",$file.FullName).Replace("'","''")
  "VerifyLicense('$relative', '$((Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant())');"
}
$checks | Set-Content "$work/license-checks.iss" -Encoding utf8
$inventory = foreach ($file in (Get-ChildItem $stage -Recurse -File | Sort-Object FullName)) {
  [ordered]@{file=[IO.Path]::GetRelativePath($stage,$file.FullName);sha256=(Get-FileHash -LiteralPath $file.FullName).Hash.ToLowerInvariant();bytes=$file.Length}
}
$inventory | ConvertTo-Json | Set-Content "$stage/package-manifest.json" -Encoding utf8
Copy-Item -LiteralPath "$PSScriptRoot/installer.iss" -Destination $work
& $Iscc "/DStageDir=$stage" "/DOutputDir=$output" "/DAppVersion=$Version" "$work/installer.iss"
if ($LASTEXITCODE) { throw 'Installer compilation failed' }
$installer = Join-Path $output "FoamAirplaneStudio-$Version-Windows-x64-Setup.exe"
Get-FileHash -LiteralPath $installer | Format-List
[ordered]@{installer=$installer;stage=$stage;work=$work;version=$Version} | ConvertTo-Json | Set-Content "$output/installer-build.json" -Encoding utf8
Write-Output "Installer: $installer"
