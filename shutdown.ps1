<#
.SYNOPSIS
    Shutdown
    Performs 'down' (for all) or 'rm -s -f' (for specific service).
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
Write-Host "Action      : SHUTDOWN"
Write-Host "Environment : $Env"
Write-Host "Service     : $(if ($Service) { $Service } else { "[ALL]" })"
Write-Host "File        : $ComposeFile"
Write-Host "------------------------------------------------"

if ([string]::IsNullOrWhiteSpace($Service)) {
    Write-Host "Stopping and removing all containers/networks..."
    & docker compose -f $ComposeFile down --remove-orphans
} else {
    Write-Host "Stopping and removing service: $Service..."
    & docker compose -f $ComposeFile rm -s -f $Service
}

if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }