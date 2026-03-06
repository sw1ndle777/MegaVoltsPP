<#
.SYNOPSIS
    Restart
    Performs 'up -d --force-recreate'.
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
Write-Host "Action      : RESTART (Force Recreate)"
Write-Host "Environment : $Env"
Write-Host "Service     : $(if ($Service) { $Service } else { "[ALL]" })"
Write-Host "File        : $ComposeFile"
Write-Host "------------------------------------------------"

$UpArgs = @("-f", $ComposeFile, "up")
if (-not [string]::IsNullOrWhiteSpace($Service)) { $UpArgs += $Service }
$UpArgs += "-d", "--force-recreate", "--remove-orphans"

& docker compose $UpArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }