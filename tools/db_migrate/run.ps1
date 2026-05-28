<#
.SYNOPSIS
    Migrate MegaVoltsPP database between MariaDB and PostgreSQL.
.EXAMPLE
    .\run.ps1                                           # Uses defaults (MariaDB -> PostgreSQL)
    .\run.ps1 -From mariadb -To postgresql              # Explicit direction
    .\run.ps1 -From postgresql -To mariadb              # Reverse migration
    .\run.ps1 -SrcHost db.example.com -SrcPort 3306     # Custom source
#>
param(
    [string]$From     = "mariadb",
    [string]$To       = "postgresql",
    [string]$SrcHost  = "127.0.0.1",
    [int]$SrcPort     = 3306,
    [string]$SrcDb    = "megavoltspp",
    [string]$SrcUser  = "root",
    [string]$SrcPass  = "god123",
    [string]$DstHost  = "127.0.0.1",
    [int]$DstPort     = 5432,
    [string]$DstDb    = "megavoltspp",
    [string]$DstUser  = "postgres",
    [string]$DstPass  = "god123"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$ReqFile = Join-Path $ScriptDir "requirements.txt"
$MigrateScript = Join-Path $ScriptDir "migrate.py"

Write-Host "Installing/updating dependencies..." -ForegroundColor Cyan
pip install -q -r $ReqFile --upgrade
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to install dependencies."
    exit 1
}

Write-Host ""
Write-Host "----------------------------------------------"
Write-Host "  Direction : $From -> $To"
Write-Host "  Source    : ${SrcUser}@${SrcHost}:${SrcPort}/${SrcDb}"
Write-Host "  Dest      : ${DstUser}@${DstHost}:${DstPort}/${DstDb}"
Write-Host "----------------------------------------------"
Write-Host ""

python $MigrateScript `
    --from $From --to $To `
    --src-host $SrcHost --src-port $SrcPort --src-db $SrcDb --src-user $SrcUser --src-pass $SrcPass `
    --dst-host $DstHost --dst-port $DstPort --dst-db $DstDb --dst-user $DstUser --dst-pass $DstPass

exit $LASTEXITCODE
