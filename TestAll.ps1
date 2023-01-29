<#  
.SYNOPSIS
    Run the TAEF dlls generated by WinAppSDK

.DESCRIPTION
    The TestAll script will take the folder input and look for subfolders containing a .testmd file. WinAppSDK
    components define a testmd with the following schema and runs the TAEF dll in the subfolder.
    
    Example:
    {
        "Tests": [
            {
                "Description": "Enabled: This module tests the push notifications component in WinAppSDK.",
                "Filename": "PushNotificationTests.dll",
                "Parameters": "",
                "Architectures": ["x86", "x64", "arm64"],
                "Status": "Enabled" 
            }
        ]
    }

.PARAMETER OutputFolder
    Set the base folder for the script to look for testmds

.PARAMETER Platform
    Only run tests for the selected platform(s)

.PARAMETER Configuration
    Only run tests the selected configuration(s)
#>

param(
        [Parameter(Mandatory=$true)]
        [string]$OutputFolder,
        [Parameter(Mandatory=$true)]
        [string]$Platform,
        [Parameter(Mandatory=$true)]
        [string]$Configuration
)

$StartTime = Get-Date
$lastexitcode = 0
Set-StrictMode -Version 3.0

$configPlat = Join-Path $Configuration $Platform
$outputFolderPath = Join-Path $OutputFolder $configPlat 
foreach ($testmd in (Get-ChildItem -Recurse -Filter "*.testmd" $outputFolderPath))
{
    $testJson = Get-Content -Raw $testmd.FullName | ConvertFrom-Json
    foreach ($testConfig in $testJson.Tests)
    {
        Write-Host $testConfig.Description
        $validPlatform = $testConfig.Architectures.Contains($Platform)
        $testEnabled = $testConfig.Status -eq "Enabled"
        if ($validPlatform -and $testEnabled)
        {
            $testFolder = Split-Path -parent $testmd.FullName
            $tePath = Join-Path $testFolder "te.exe"
            $dllFile = Join-Path $testFolder $testConfig.Filename
            & $tePath $dllFile
        }
        elseif (-not($validPlatform))
        {
            Write-Host "$Platform not listed in supported architectures."
        }
        elseif (-not($testEnabled))
        {
            Write-Host "Test is disabled. Not running."
        }
    }
}

$TotalTime = (Get-Date)-$StartTime
$TotalMinutes = $TotalTime.Minutes
$TotalSeconds = $TotalTime.Seconds
Write-Host "Total Running Time: $TotalMinutes minutes and $TotalSeconds seconds"
