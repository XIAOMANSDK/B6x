# 可选参数：Path_lnp（默认 ./output/）
Param(
    [Parameter(Position=0)]
    [string]$Path_lnp_param = '.\output\'
)

# 用户自定义ARMCC路径
$ArmCC_Path = 'C:\Keil_v5\ARM\ARMCC\bin'
# 用户自定义7-Zip路径
$SevenZip_Custom_Path = 'C:\Program Files\7-Zip\7z.exe'

$Bank_A   = '_0x04000'
$Bank_B   = '_0x20000'
$Ext_lnp  = '.lnp'
$Ext_axf  = '.axf'
$Ext_hex  = '.hex'
$Ext_bin  = '.bin'
$Ext_asm  = '.asm'

function Normalize-PathInput {
    param([string]$p)
    if (-not $p) { return $null }
    # 修剪空白字符和周围引号，展开环境变量
    $s = $p.Trim().Trim('"', "'")
    $s = [Environment]::ExpandEnvironmentVariables($s)

    # 尝试解析为提供程序路径；如果失败则使用原始字符串
    try {
        $resolved = Resolve-Path -LiteralPath $s -ErrorAction Stop
        $s = $resolved.ProviderPath
    } catch {
        # 如果解析失败，保持 $s 不变
    }

    # 如果路径指向文件，则使用其目录
    if (Test-Path -LiteralPath $s) {
        $it = Get-Item -LiteralPath $s -ErrorAction SilentlyContinue
        if ($it -and -not $it.PSIsContainer) { $s = Split-Path $s -Parent }
    }

    # 去除所有末尾反斜杠，然后添加一个
    $s = $s.TrimEnd('\\')
    if ($s) { $s += '\' }
    return $s
}

# 规范化并验证传入的路径
$Path_lnp = Normalize-PathInput $Path_lnp_param
if (-not $Path_lnp) {
    Write-Error "Invalid Path_lnp: '$Path_lnp_param'"
    exit 1
}

# 确保目录存在
if (-not (Test-Path -LiteralPath $Path_lnp)) {
    Write-Error "Path does not exist: $Path_lnp"
    exit 1
}

# 提取基目录：去掉末尾的 "output\" 或 "output"
$baseDir = $Path_lnp
if ($baseDir -match '\\output\\$' -or $baseDir -match '\\output$') {
    $baseDir = Split-Path -Parent $baseDir
}
$baseDir = $baseDir.TrimEnd('\') + '\'

$ARM_LINK = 'armlink.exe'
$ARM_FROM = 'fromelf.exe'

# 判断系统环境变量中 armlink 是否存在
# 如果不存在，则使用用户自定义路径
$Found_Armlink=[bool] (Get-Command -ErrorAction SilentlyContinue  -Type Application $ARM_LINK)
if (!$Found_Armlink)
{
    # 将用户自定义路径添加到系统环境变量
    $env:path+=";$ArmCC_Path;"

    $Found_Armlink=[bool](Get-Command -ErrorAction SilentlyContinue  -Type Application $ARM_LINK)
    # 用户自定义路径有误
    if (!$Found_Armlink)
    {
        Write-Output "=================="
        Write-Output " ARMCC Path Error "
        Write-Output "=================="
        exit
    }
}

# 获取最新的lnp文件(eg:\projects\bleOTA\mdk\output\bleOTA.lnp)
$lnpFile = Get-ChildItem -Path $Path_lnp -Recurse -ErrorAction SilentlyContinue -Filter '*.lnp' |
    Sort-Object -Property LastWriteTime -Descending |
    Select-Object -First 1
if (-not $lnpFile) {
    Write-Error "No .lnp file found in: $Path_lnp"
    exit 1
}

$full_path = $lnpFile.FullName

# 获取不带路径且不带后缀的 lnp 文件名
$Bank_A_File_Name = [System.IO.Path]::GetFileNameWithoutExtension($full_path)
$Bank_B_File_Name = $Bank_A_File_Name + $Bank_B

# 带路径不带后缀的名称(例如: \projects\bleOTA\mdk\output\bleOTA)
$outDir = [System.IO.Path]::GetDirectoryName($full_path)
$Bank_A_name = Join-Path -Path $outDir -ChildPath $Bank_A_File_Name
$Bank_B_name = Join-Path -Path $outDir -ChildPath $Bank_B_File_Name

# 拼成带路径的原始文件
$Org_lnp = $Bank_A_name + $Ext_lnp
$Org_axf = $Bank_A_name + $Ext_axf
$Org_hex = $Bank_A_name + '_0x04000' + $Ext_hex
$Org_bin = $Bank_A_name + '_0x04000' + $Ext_bin
$Org_asm = $Bank_A_name + '_0x04000' + $Ext_asm

# 拼成带路径的目标文件
$Obj_lnp = $Bank_B_name + $Ext_lnp
$Obj_axf = $Bank_B_name + $Ext_axf
$Obj_hex = $Bank_B_name + $Ext_hex
$Obj_bin = $Bank_B_name + $Ext_bin
$Obj_asm = $Bank_B_name + $Ext_asm

# 获取包含路径的 sct 名称
if (-not (Test-Path $Org_lnp)) {
    Write-Error "Linker script not found: $Org_lnp"
    exit 1
}

$Org_sct = (Get-Content $Org_lnp -ErrorAction SilentlyContinue | Select-String -Pattern '\.sct' | ForEach-Object { $_.Line.Split('"')[1] } | Select-Object -First 1)
if (-not $Org_sct) {
    Write-Error "No .sct file reference found in: $Org_lnp"
    exit 1
}

# 修剪提取的路径中的空白字符（可能包含空格或换行符）
$Org_sct = $Org_sct.Trim()

# 尝试多种策略来解析 .sct 文件路径
$foundSct = $null

# 策略 1：如果是绝对路径，直接使用
if ([System.IO.Path]::IsPathRooted($Org_sct)) {
    if (Test-Path -LiteralPath $Org_sct) {
        $foundSct = $Org_sct
    }
} else {
    # 相对于输出目录（.lnp 文件所在目录）
    $outDirNoTrail = $outDir.TrimEnd('\')
    $sctRelativePath = Join-Path -Path $outDirNoTrail -ChildPath $Org_sct
    $resolvedPath = [System.IO.Path]::GetFullPath($sctRelativePath)
    if (Test-Path -LiteralPath $resolvedPath) {
        $foundSct = $resolvedPath
    }

    # 策略 3：相对于基目录（去掉 output 后的 mdk 目录）
    if (-not $foundSct) {
        $baseDirNoTrail = $baseDir.TrimEnd('\')
        $sctRelativePath3 = Join-Path -Path $baseDirNoTrail -ChildPath $Org_sct
        $resolvedPath3 = [System.IO.Path]::GetFullPath($sctRelativePath3)
        if (Test-Path -LiteralPath $resolvedPath3) {
            $foundSct = $resolvedPath3
        }
    }
}

if (-not $foundSct) {
    Write-Error "找不到源 .sct 文件: $Org_sct"
    exit 1
}

$Org_sct = $foundSct

# 仅获取 sct 文件名，用于后续文本替换
$Org_sct_name = [System.IO.Path]::GetFileNameWithoutExtension($Org_sct)
$Obj_sct = [System.IO.Path]::Combine([System.IO.Path]::GetDirectoryName($Org_sct), ($Org_sct_name + $Bank_B + '.sct'))
$Obj_sct_name = $Org_sct_name + $Bank_B

# 判断目标 sct 文件是否不存在
if ( !(Test-Path $Obj_sct) )
{
    # 一次性替换两个地址
    (Get-Content $Org_sct) -Replace "0x18020000", "0x18040000" -Replace "0x18004000", "0x18020000" | Set-Content $Obj_sct
}

# 在原始 lnp 文件基础上替换 sct 文件名和 Bank 文件名，并保存为目标 lnp 文件
# (例如: link_xip_ble_0x04000 替换为 link_xip_ble_0x20000, bleOTA 替换为 bleOTA_0x20000)
(Get-Content $Org_lnp) -Replace $Org_sct_name, $Obj_sct_name -Replace $Bank_A_File_Name, $Bank_B_File_Name | Set-Content $Obj_lnp

# 链接生成高地址 Bank
# 切换到基目录，使 armlink 能正确解析 .lnp 中的相对路径
$originalDir = Get-Location
Set-Location -Path $baseDir -ErrorAction Stop

try {
    # 链接生成目标文件(axf, map)
    & $ARM_LINK --Via $Obj_lnp
} finally {
    # 始终恢复原始目录
    Set-Location -Path $originalDir
}

# 删除目标 lnp，防止下次读取时存在多个 lnp 导致的报错
Remove-Item $Obj_lnp -ErrorAction SilentlyContinue

# 生成 HEX 文件
& $ARM_FROM $Org_axf --i32combined --output $Org_hex
& $ARM_FROM $Obj_axf --i32combined --output $Obj_hex

# 生成 BIN 文件
& $ARM_FROM $Org_axf --bin --output $Org_bin
& $ARM_FROM $Obj_axf --bin --output $Obj_bin

# 生成 ASM 文件
& $ARM_FROM $Org_axf --text -a -c --output $Org_asm
& $ARM_FROM $Obj_axf --text -a -c --output $Obj_asm

# 删除旧的带CRC32的bin文件（保留最新的）
$oldCRCFiles = Get-ChildItem -Path (Split-Path -Parent $Org_bin) -Filter "*0x*_0x*.bin" -ErrorAction SilentlyContinue
if ($oldCRCFiles) {
   Remove-Item $oldCRCFiles.FullName -Force
}
# 计算CRC32并创建带CRC32的bin文件
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public class Win32Api {
    [DllImport("ntdll.dll")]
    public static extern uint RtlComputeCrc32(uint dwInitial, byte[] pData, int iLen);
}
"@

function Get-CRC32 {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory=$true,ValueFromPipeline = $true)]
        [string]$InputFile
    )

    # 读取文件的字节
    $fileBytes = [System.IO.File]::ReadAllBytes($InputFile)

    # 使用 Win32 API 计算 CRC32 校验和
    $crc32 = [Win32Api]::RtlComputeCrc32(0, $fileBytes, $fileBytes.Length)

    # 将 CRC32 值转换为十六进制字符串
    $crc32String = $crc32.ToString("X8")

    # 显示 CRC32 校验和
    Write-Output $crc32String
}

# 计算 Bank A 的 CRC32
$CRC32_A = Get-CRC32 $Org_bin
$Bank_A_with_crc = Split-Path -Leaf ($Org_bin -replace '\.bin$', "_0x$CRC32_A.bin")

# 根据 CRC32 创建新的文件名并复制文件
$Org_bin_with_crc = $Org_bin -replace '\.bin$', "_0x$CRC32_A.bin"
Copy-Item -Path $Org_bin -Destination $Org_bin_with_crc -Force
# 删除原始的 bin 文件
#Remove-Item -Path $Org_bin -Force
# 更新 Org_bin 指向带 CRC32 的文件
$Org_bin = $Org_bin_with_crc

# 计算 Bank B 的 CRC32
$CRC32_B = Get-CRC32 $Obj_bin
$Bank_B_with_crc = Split-Path -Leaf ($Obj_bin -replace '\.bin$', "_0x$CRC32_B.bin")

# 根据 CRC32 创建新的文件名并复制文件
$Obj_bin_with_crc = $Obj_bin -replace '\.bin$', "_0x$CRC32_B.bin"
Copy-Item -Path $Obj_bin -Destination $Obj_bin_with_crc -Force
# 删除原始的 bin 文件
#Remove-Item -Path $Obj_bin -Force
# 更新 Obj_bin 指向带 CRC32 的文件
$Obj_bin = $Obj_bin_with_crc

# 根据最长文件名长度生成分隔线
$maxLen = [Math]::Max($Bank_A_with_crc.Length, $Bank_B_with_crc.Length)
$separator = "=" * ($maxLen + 8)

Write-Output $separator
Write-Output "bankA: $Bank_A_with_crc"
Write-Output "bankB: $Bank_B_with_crc"
Write-Output $separator

$SevenZip_64bit = "C:\Program Files\7-Zip\7z.exe"  # Standard 64-bit installation
$SevenZip_32bit = 'C:\Program Files (x86)\7-Zip\7z.exe'  # Standard 32-bit installation on 64-bit Windows

# First try to find 7-Zip in PATH
$SevenZip = $SevenZip_64bit
$Found_7Zip = [bool](Get-Command -ErrorAction SilentlyContinue -Type Application $SevenZip)

# If not found in PATH, try common installation locations
if (!$Found_7Zip) {
    $SevenZipPaths = @(
        $SevenZip_32bit,
        $SevenZip_Custom_Path
    )

    foreach ($path in $SevenZipPaths) {
        if (Test-Path $path) {
            $SevenZip = $path
            $Found_7Zip = $true
            break
        }
    }
}

if (!$Found_7Zip) {
    Write-Error "找不到 7-Zip 安装。请确保已安装 7-Zip。"
    Write-Error "下载地址: https://www.7-zip.org/"
    Write-Error "已检查的路径："
    Write-Error "  - 64 位: $SevenZip_64bit"
    Write-Error "  - 32 位: $SevenZip_32bit"
    Write-Error "  - 自定义: $SevenZip_Custom_Path"
    exit 1
}

# 创建 manifest.json 内容
$manifestHashTable = [ordered]@{
    "bankA" = Split-Path -Path $Org_bin -Leaf
    "bankB" = Split-Path -Path $Obj_bin -Leaf
}
$manifest = $manifestHashTable | ConvertTo-Json -Depth 3

# 将 manifest.json 写入输出目录
$manifestPath = Join-Path (Split-Path -Parent $Org_bin) "manifest.json"
Set-Content -Path $manifestPath -Value $manifest

# 创建 ZIP 文件名
$zipFileName = Join-Path (Split-Path -Parent $Org_bin) ($Bank_A_File_Name + "_OTA_FW.zip")

# 先删除已存在的 zip 文件，确保以新创建模式而非追加模式打包
if (Test-Path $zipFileName) {
    Remove-Item $zipFileName -Force
}

# 使用 7-Zip 创建包含两个 bin 文件和 manifest.json 的存档
$arguments = @(
    "a",           # 向存档添加文件
    "-tzip",       # 指定 zip 格式
    "-bso0",       # 设置输出为无（无进度信息）
    "-bsp0",       # 不显示进度百分比
    $zipFileName,  # 输出存档名称
    $Org_bin,     # Bank A bin 文件
    $Obj_bin,     # Bank B bin 文件
    $manifestPath # Manifest 文件
)

# 使用 7-Zip 执行，隐藏输出
$zipResult = & $SevenZip @arguments 2>&1