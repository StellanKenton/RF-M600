param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectPath,

    [string]$Target = "Development",

    [ValidateSet("Build", "Rebuild")]
    [string]$Mode = "Build",

    [int]$TimeoutSeconds = 120
)

$resolvedProjectPath = (Resolve-Path $ProjectPath).Path
$projectDir = Split-Path -Parent $resolvedProjectPath
$buildLogPath = Join-Path $projectDir "build_log.txt"

$uv4Candidates = @(
    $env:KEIL_UV4,
    "C:\Users\senki\AppData\Local\Keil_v5\UV4\UV4.exe",
    "C:\Keil_v5\UV4\UV4.exe"
) | Where-Object { $_ }

$uv4Path = $uv4Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $uv4Path) {
    Write-Error "UV4.exe not found. Install Keil MDK or set the KEIL_UV4 environment variable."
    exit 1
}

$buildSwitch = if ($Mode -eq "Rebuild") { "-r" } else { "-b" }
$arguments = @(
    $buildSwitch,
    $resolvedProjectPath,
    "-t",
    $Target,
    "-j0",
    "-o",
    $buildLogPath
)

Write-Host "Using Keil:" $uv4Path
Write-Host "Project:" $resolvedProjectPath
Write-Host "Target:" $Target
Write-Host "Mode:" $Mode

Push-Location $projectDir
try {
    if (Test-Path $buildLogPath) {
        Remove-Item $buildLogPath -Force
    }

    $process = Start-Process -FilePath $uv4Path -ArgumentList $arguments -PassThru
    if (-not $process) {
        Write-Error "Failed to start UV4.exe."
        exit 1
    }

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $buildOutput = $null

    while ((Get-Date) -lt $deadline) {
        if (Test-Path $buildLogPath) {
            $buildOutput = Get-Content $buildLogPath -Raw
            if (
                $buildOutput -match 'Build Time Elapsed:' -or
                $buildOutput -match 'Build aborted\.' -or
                $buildOutput -match '"\.\\Objects\\.*"\s+-\s+\d+\s+Error\(s\),\s+\d+\s+Warning\(s\)\.'
            ) {
                break
            }
        }

        Start-Sleep -Milliseconds 500
    }

    if (-not $buildOutput) {
        Write-Error "Keil build log was not generated."
        exit 1
    }

    Write-Host ""
    Write-Output $buildOutput.TrimEnd()

    if ($buildOutput -match 'Build aborted\.') {
        exit 1
    }

    $summaryMatch = [regex]::Match($buildOutput, '(\d+)\s+Error\(s\),\s+(\d+)\s+Warning\(s\)\.')
    if (-not $summaryMatch.Success) {
        Write-Error "Keil build completed without a recognizable summary line."
        exit 1
    }

    $errorCount = [int]$summaryMatch.Groups[1].Value
    if ($errorCount -ne 0) {
        exit 1
    }
}
finally {
    Pop-Location
}