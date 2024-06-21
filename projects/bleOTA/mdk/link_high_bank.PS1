$Bank_A = '_0x04000'
$Bank_B = '_0x20000'
$Ext_lnp = '.lnp'
$Ext_axf = '.axf'
$Ext_hex = '.hex'
$Ext_bin = '.bin'
$Ext_asm = '.asm'
$Path_lnp = '.\output\'

# user defined
$Path_cmd = 'C:\Keil_v5\ARM\ARMCC\bin'
$env:path+=";$Path_cmd;"

# 获取最新的lnp文件(eg:\projects\bleOTA\mdk\output\bleOTA.lnp)
$full_path = Get-ChildItem -Path $Path_lnp -Recurse -ErrorAction SilentlyContinue -Filter *.lnp | Where-Object { $_.Extension -eq '.lnp' } | Where-Object {$_.LastWriteTime} | Select-Object -last 1 | Select-Object -ExpandProperty FullName

# 以.为分隔符, 分割带路径的文件名(eg:\projects\bleOTA\mdk\output\bleOTA)和后缀名(eg:lnp)
# 去掉.lnp的长度
$full_paht_len = $full_path.length - 4
#$Pname, $Ext = $full_path.Split(".")
$Pname = $full_path.Substring(0, $full_paht_len)

# 带路径不带后缀的名称
# (eg:\projects\bleOTA\mdk\output\bleOTA)
# (eg:\projects\bleOTA\mdk\output\bleOTA_0x20000)
$Bank_A_name = $Pname
$Bank_B_name = $Pname+$Bank_B

# 获取不带路径且不带后缀的lnp文件名
$Bank_A_File_Name = (Get-Item $full_path).BaseName
$Bank_B_File_Name = $Bank_A_File_Name+$Bank_B

# 拼成带路径的原始文件
# (eg:\projects\bleOTA\mdk\output\bleOTA.lnp)
# (eg:\projects\bleOTA\mdk\output\bleOTA.axf)
# (eg:\projects\bleOTA\mdk\output\bleOTA_0x4000.hex)
# (eg:\projects\bleOTA\mdk\output\bleOTA_0x4000.bin)
# (eg:\projects\bleOTA\mdk\output\bleOTA_0x4000.asm)
$Org_lnp = $Bank_A_name+$Ext_lnp
$Org_axf = $Bank_A_name+$Ext_axf
$Org_hex = $Bank_A_name+'_0x4000'+$Ext_hex
$Org_bin = $Bank_A_name+'_0x4000'+$Ext_bin
$Org_asm = $Bank_A_name+'_0x4000'+$Ext_asm

# 拼成带路径的目标文件
# (eg:\projects\bleOTA\mdk\output\bleOTA_0x20000.lnp)
# (eg:\projects\bleOTA\mdk\output\bleOTA_0x20000.axf)
# (eg:\projects\bleOTA\mdk\output\bleOTA_0x20000.hex)
# (eg:\projects\bleOTA\mdk\output\bleOTA_0x20000.bin)
# (eg:\projects\bleOTA\mdk\output\bleOTA_0x20000.asm)
$Obj_lnp = $Bank_B_name+$Ext_lnp
$Obj_axf = $Bank_B_name+$Ext_axf
$Obj_hex = $Bank_B_name+$Ext_hex
$Obj_bin = $Bank_B_name+$Ext_bin
$Obj_asm = $Bank_B_name+$Ext_asm


# 在原始lnp文件基础上替换sct文件, 并保存为目标lnp文件
# (eg:link_xip_ble_0x04000替换为link_xip_ble_0x20000)
(Get-Content $Org_lnp) -Replace $Bank_A, $Bank_B | Set-Content $Obj_lnp

# 替换目标lnp文件中的文件名
# (eg:bleOTA替换为bleOTA_0x20000）
(Get-Content $Obj_lnp) -Replace $Bank_A_File_Name, $Bank_B_File_Name | Set-Content $Obj_lnp

Write-Output "**************** Linking High Bank ... ****************"
# 链接生成目标文件(axf, map)
armlink.exe --Via $Obj_lnp

# 删除目标lnp, 防止下次读取时存在多个lnp导致的报错
Remove-Item $Obj_lnp

# 生成hex
fromelf.exe $Org_axf --i32combined --output $Org_hex
fromelf.exe $Obj_axf --i32combined --output $Obj_hex

# 生成bin
fromelf.exe $Org_axf --bin --output $Org_bin
fromelf.exe $Obj_axf --bin --output $Obj_bin

# 生成asm
fromelf.exe $Org_axf --text -a -c --output $Org_asm
fromelf.exe $Obj_axf --text -a -c --output $Obj_asm

# 计算CRC32
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

    #Write-host $InputFile

    # Read the file as bytes
    $fileBytes = [System.IO.File]::ReadAllBytes($InputFile)

    # Calculate the CRC32 checksum using the Win32 API
    $crc32 = [Win32Api]::RtlComputeCrc32(0, $fileBytes, $fileBytes.Length)

    # Convert the CRC32 value to hexadecimal string
    $crc32String = $crc32.ToString("X8")

    # Display the CRC32 checksum
    Write-Output $crc32String
}

$Bank_A = "File : " + $Bank_A_File_Name +  '_0x4000' + $Ext_bin
$Bank_B = "File : " + $Bank_B_File_Name + $Ext_bin

$CRC32 = Get-CRC32 $Org_bin
Write-Output "================================="
Write-Output $Bank_A
$CRC32 =  "CRC32: " + "0x" +$CRC32
Write-Output $CRC32
Write-Output "================================="

$CRC32 = Get-CRC32 $Obj_bin
Write-Output $Bank_B
$CRC32 =  "CRC32: " + "0x" +$CRC32
Write-Output $CRC32
Write-Output "================================="

#pause