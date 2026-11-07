# Script tự động đóng gói và ký gói MSIX cho AgeOfShapes
$ErrorActionPreference = "Stop"

$workspaceRoot = $PSScriptRoot
$clientDir = Join-Path $workspaceRoot "Client"
$outputMsix = Join-Path $workspaceRoot "AgeOfShapes.msix"
$outputVersionedMsix = Join-Path $workspaceRoot "AgeOfShapes_1.1.8.0_x64.msix"

$makeAppx = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.28000.0\x64\makeappx.exe"
$signTool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.28000.0\x64\signtool.exe"
$certThumbprint = "95E0A52C8244F8D2809FD72D11459FB52847222A"

Write-Host "==> Đang đóng gói MSIX từ: $clientDir" -ForegroundColor Cyan
& $makeAppx pack /v /o /d $clientDir /p $outputMsix

Write-Host "==> Đang ký số gói MSIX với chứng chỉ: $certThumbprint" -ForegroundColor Cyan
& $signTool sign /fd SHA256 /sha1 $certThumbprint $outputMsix

Copy-Item $outputMsix $outputVersionedMsix -Force

Write-Host "==> Hoàn tất! File đã tạo:" -ForegroundColor Green
Write-Host " - $outputMsix"
Write-Host " - $outputVersionedMsix"
