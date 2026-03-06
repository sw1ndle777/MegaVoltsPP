<#
.SYNOPSIS
    Rebuild
.EXAMPLE
    .\rebuild.ps1               # Defaults to prod, all services
    .\rebuild.ps1 nginx         # Defaults to prod, only nginx
    .\rebuild.ps1 dev           # Dev env, all services
    .\rebuild.ps1 dev nginx     # Dev env, only nginx
#>
param(
    [string]$Arg1 = "",
    [string]$Arg2 = ""
)

$ErrorActionPreference = "Continue"

$Env = "prod"
$Service = ""

if ($Arg1 -eq "dev" -or $Arg1 -eq "prod") {
    $Env = $Arg1
    $Service = $Arg2
} elseif (-not [string]::IsNullOrWhiteSpace($Arg1)) {
    $Env = "prod"
    $Service = $Arg1
} else {
    $Env = "prod"
    $Service = ""
}

if ($Env -eq "dev") {
    $ComposeFile = "docker-compose.dev.yml"
} else {
    if (Test-Path "docker-compose.prod.yml") {
        $ComposeFile = "docker-compose.prod.yml"
    } else {
        Write-Warning "Could not find docker-compose.prod.yml. Using docker-compose.yml"
        $ComposeFile = "docker-compose.yml"
    }
}

Write-Host "------------------------------------------------"
Write-Host "Environment : $Env"
Write-Host "Service     : $(if ($Service) { $Service } else { "[ALL]" })"
Write-Host "File        : $ComposeFile"
Write-Host "------------------------------------------------"

Write-Host "Building..."
$BuildArgs = @("-f", $ComposeFile, "build")
if (-not [string]::IsNullOrWhiteSpace($Service)) { $BuildArgs += $Service }
$BuildArgs += "--no-cache", "--parallel", "--force-rm"

& docker compose $BuildArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed!"
    exit $LASTEXITCODE
}

Write-Host "Starting up..."
$UpArgs = @("-f", $ComposeFile, "up")
if (-not [string]::IsNullOrWhiteSpace($Service)) { $UpArgs += $Service }
$UpArgs += "-d", "--force-recreate", "--remove-orphans"

& docker compose $UpArgs
if ($LASTEXITCODE -ne 0) {
Write-Error "Startup failed!"
exit $LASTEXITCODE
}

Write-Host "Done."