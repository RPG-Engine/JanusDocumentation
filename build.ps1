# Builds the Janus Engine documentation with Doxygen.
# Usage (from anywhere):  powershell -File <path>\JanusDocumentation\build.ps1
# Output:                 build\html\index.html
#
# Requires Doxygen (https://www.doxygen.nl) and Graphviz (https://graphviz.org).
# If they are not on PATH, the default Windows install folders are tried.
#
# The engine commit the docs describe is read from docs-version.json and passed to
# Doxygen through environment variables, so every GitHub source link points at it.

param([string]$EngineDir = '')

$ErrorActionPreference = 'Stop'
Set-Location -Path $PSScriptRoot

$doxygen = (Get-Command doxygen -ErrorAction SilentlyContinue).Source
if (-not $doxygen -and (Test-Path 'C:\Program Files\doxygen\bin\doxygen.exe')) {
    $doxygen = 'C:\Program Files\doxygen\bin\doxygen.exe'
}
if (-not $doxygen) { throw 'Doxygen not found. Install it from https://www.doxygen.nl/download.html' }

if (-not (Get-Command dot -ErrorAction SilentlyContinue) -and (Test-Path 'C:\Program Files\Graphviz\bin\dot.exe')) {
    $env:PATH = 'C:\Program Files\Graphviz\bin;' + $env:PATH
}

# The engine (RPG-Engine-Core) is a separate repository whose headers Doxygen reads.
# Pass -EngineDir, set JANUS_ENGINE_DIR, or keep it at ..\ProtoRPG\Engine.
if (-not $EngineDir) { $EngineDir = if ($env:JANUS_ENGINE_DIR) { $env:JANUS_ENGINE_DIR } else { Join-Path (Split-Path -Parent $PSScriptRoot) 'ProtoRPG\Engine' } }
if (-not (Test-Path (Join-Path $EngineDir 'include'))) { throw "Engine not found at $EngineDir. Pass -EngineDir <path> or set JANUS_ENGINE_DIR." }
$env:JANUS_ENGINE_DIR = (Resolve-Path $EngineDir).Path.Replace([string][char]92, '/')

# Version information used by the Doxyfile ($(JANUS_DOCS_*) variables)
$version = Get-Content docs-version.json -Raw | ConvertFrom-Json
if ($version.engineCommit -notmatch '^[0-9a-f]{40}$') { throw 'docs-version.json: engineCommit must be a full 40-character commit hash' }
$env:JANUS_DOCS_COMMIT       = $version.engineCommit
$env:JANUS_DOCS_COMMIT_SHORT = $version.engineCommit.Substring(0, 7)
$env:JANUS_DOCS_REPO         = $version.engineRepository
$env:JANUS_DOCS_DATE         = $version.documentedOn
# JanusDocumentation repository and the branch (or commit) whose examples/ folder the example links point to
$env:JANUS_DOCS_OWN_REPO     = $version.docsRepository
$env:JANUS_DOCS_EXAMPLES_REF = if ($version.examplesRef) { $version.examplesRef } else { 'master' }

# Clear old output (contents only, so a preview server serving the folder doesn't block the build)
if (Test-Path build\html) { Get-ChildItem build\html -Force | Remove-Item -Recurse -Force }
New-Item -ItemType Directory -Force build | Out-Null

& $doxygen Doxyfile
if ($LASTEXITCODE -ne 0) { throw "Doxygen failed with exit code $LASTEXITCODE" }

$warnings = if (Test-Path build\warnings.log) { @(Get-Content build\warnings.log | Where-Object { $_.Trim() }) } else { @() }
Write-Host ''
Write-Host "Built: $PSScriptRoot\build\html\index.html"
Write-Host "Documents engine commit $($env:JANUS_DOCS_COMMIT_SHORT) ($($version.documentedOn))"
Write-Host "Warnings: $($warnings.Count) (see build\warnings.log)"
