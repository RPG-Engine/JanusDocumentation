<#
.SYNOPSIS
  Shows what changed in the engine since the commit the docs describe, and which pages to update.

.DESCRIPTION
  Reads docs-version.json (engineCommit + coverage map) and compares that commit with the
  engine's HEAD (or with the working tree). For every changed file it lists the pages that
  document it, and every GitHub source reference in those pages that points into a changed
  file (their line numbers may have moved).

  Typical update:
    1. tools\docs-delta.ps1                    see what changed and which pages are affected
    2. update those pages (and the examples, if an API they use changed)
    3. tools\docs-delta.ps1 -CheckLinks -Ref HEAD   check every source link against the new code
    4. tools\docs-delta.ps1 -Pin               move engineCommit to HEAD, then run build.ps1

.PARAMETER Ref
  Commit to compare against. Default: HEAD.

.PARAMETER WorkingTree
  Compare against the working tree (uncommitted changes included) instead of a commit.

.PARAMETER CheckLinks
  Check every \src, \srclink, \srcdir and \engex reference in pages/: the file must exist and
  the line must be in range. For \engex blocks, the first code line must match the source line.
  Checks against the pinned commit, or against -Ref when it is given.

.PARAMETER Pin
  Set engineCommit in docs-version.json to -Ref (default HEAD) and documentedOn to today.
  Do this only after the pages are up to date.
#>
param(
    [string]$Ref = '',
    [switch]$WorkingTree,
    [switch]$CheckLinks,
    [switch]$Pin
)

$ErrorActionPreference = 'Stop'
$docsDir   = Split-Path -Parent $PSScriptRoot
$engineDir = Split-Path -Parent $docsDir
$versionFile = Join-Path $docsDir 'docs-version.json'
$version = Get-Content $versionFile -Raw | ConvertFrom-Json
$pinned = $version.engineCommit

function Invoke-Git { & git -C $engineDir @args; if ($LASTEXITCODE -ne 0) { throw "git $args failed" } }

$target = if ($Ref) { (Invoke-Git rev-parse $Ref).Trim() } else { (Invoke-Git rev-parse HEAD).Trim() }

# ---------------------------------------------------------------- Pin
if ($Pin) {
    $json = Get-Content $versionFile -Raw
    $json = $json -replace '"engineCommit":\s*"[0-9a-f]{40}"', ('"engineCommit": "' + $target + '"')
    $json = $json -replace '"documentedOn":\s*"[^"]*"', ('"documentedOn": "' + (Get-Date -Format 'yyyy-MM-dd') + '"')
    Set-Content -Path $versionFile -Value $json -NoNewline -Encoding utf8
    Write-Host "docs-version.json now pins $($target.Substring(0,7)). Rebuild with build.ps1."
    return
}

# ---------------------------------------------------------------- Source references in pages
$refPattern = '\\(src|srclink|srcdir|engex|engexopen)\{([^,}]+)(?:,(\d+))?(?:,(\d+))?'
function Get-SourceRefs {
    $refs = @()
    Get-ChildItem (Join-Path $docsDir 'pages') -Recurse -Include *.md, *.dox | ForEach-Object {
        $page = $_.FullName.Substring($docsDir.Length + 1).Replace('\', '/')
        $lines = Get-Content $_.FullName
        for ($i = 0; $i -lt $lines.Count; $i++) {
            foreach ($m in [regex]::Matches($lines[$i], $refPattern)) {
                $refs += [pscustomobject]@{
                    Page = $page; PageLine = $i + 1; Kind = $m.Groups[1].Value
                    File = $m.Groups[2].Value.Trim()
                    From = if ($m.Groups[3].Success) { [int]$m.Groups[3].Value } else { 0 }
                    To   = if ($m.Groups[4].Success) { [int]$m.Groups[4].Value } else { 0 }
                    # first code line of an \engex block: the line after the opening ``` fence
                    FirstCode = if ($m.Groups[1].Value -like 'engex*') {
                        $j = $i + 1
                        while ($j -lt $lines.Count -and $lines[$j] -notmatch '^\s*```') { $j++ }
                        $j++
                        while ($j -lt $lines.Count -and ($lines[$j].Trim() -eq '' -or $lines[$j].Trim() -eq '// ...')) { $j++ }
                        if ($j -lt $lines.Count) { $lines[$j] } else { '' }
                    } else { '' }
                }
            }
        }
    }
    return $refs
}

if ($CheckLinks) {
    $commit = if ($Ref) { $target } else { $pinned }
    Write-Host "Checking source references against $($commit.Substring(0,7))"
    $cache = @{}
    $problems = 0
    foreach ($r in Get-SourceRefs) {
        $key = $r.File
        if (-not $cache.ContainsKey($key)) {
            if ($r.Kind -eq 'srcdir') {
                $out = & git -C $engineDir ls-tree -d --name-only $commit -- $r.File 2>$null
                $cache[$key] = if ($out) { @('dir') } else { $null }
            } else {
                $text = & git -C $engineDir show "$($commit):$($r.File)" 2>$null
                $cache[$key] = if ($LASTEXITCODE -eq 0) { @($text) } else { $null }
            }
        }
        $content = $cache[$key]
        $where = "$($r.Page):$($r.PageLine)"
        if ($null -eq $content) { Write-Host "  MISSING  $where  $($r.File)"; $problems++; continue }
        if ($r.Kind -eq 'srcdir') { continue }
        foreach ($n in @($r.From, $r.To)) {
            if ($n -gt $content.Count) { Write-Host "  RANGE    $where  $($r.File):$n (file has $($content.Count) lines)"; $problems++ }
        }
        if ($r.FirstCode -and $r.From -gt 0 -and $r.From -le $content.Count) {
            $expected = ($content[$r.From - 1] -replace '\s+', ' ').Trim().TrimStart([char]0xFEFF)
            $actual = ($r.FirstCode -replace '\s+', ' ').Trim()
            if ($expected -ne $actual) {
                Write-Host "  SNIPPET  $where  $($r.File):$($r.From)"
                Write-Host "           page:   $actual"
                Write-Host "           source: $expected"
                $problems++
            }
        }
    }
    Write-Host "$problems problem(s) found."
    return
}

# ---------------------------------------------------------------- Delta report
Write-Host "Docs describe:  $($pinned.Substring(0,7))  (documented $($version.documentedOn))"
if ($WorkingTree) {
    Write-Host "Comparing with: working tree"
    $diff = Invoke-Git diff --name-status $pinned -- . ':(exclude)Documentation'
    $untracked = Invoke-Git ls-files --others --exclude-standard -- . ':(exclude)Documentation'
    $diff = @($diff) + @($untracked | ForEach-Object { "A`t$_" })
} else {
    Write-Host "Comparing with: $($target.Substring(0,7))"
    Write-Host ''
    Write-Host 'Commits:'
    Invoke-Git log --oneline "$pinned..$target" | ForEach-Object { Write-Host "  $_" }
    $diff = Invoke-Git diff --name-status $pinned $target -- . ':(exclude)Documentation'
}

$changes = @($diff | Where-Object { $_ } | ForEach-Object {
    $parts = $_ -split "`t"
    [pscustomobject]@{ Status = $parts[0].Substring(0,1); File = $parts[-1]; OldFile = $parts[1] }
})
if ($changes.Count -eq 0) { Write-Host ''; Write-Host 'No engine changes. The docs are up to date.'; return }

$coverage = $version.coverage.PSObject.Properties
$pages = @{}
Write-Host ''
Write-Host 'Changed files:'
foreach ($c in $changes) {
    $matched = @()
    foreach ($entry in $coverage) {
        if ($entry.Name -eq '*') { continue }
        if ($c.File -eq $entry.Name -or ($entry.Name.EndsWith('/') -and $c.File.StartsWith($entry.Name))) { $matched += $entry.Value }
    }
    if ($matched.Count -eq 0) { $matched = @('(not in the coverage map: decide where it belongs and add it to docs-version.json)') }
    Write-Host ("  {0}  {1}" -f $c.Status, $c.File)
    foreach ($p in ($matched | Select-Object -Unique)) {
        if (-not $pages.ContainsKey($p)) { $pages[$p] = @() }
        $pages[$p] += $c.File
    }
}
foreach ($p in ($coverage | Where-Object Name -eq '*').Value) { if (-not $pages.ContainsKey($p)) { $pages[$p] = @('(any change)') } }

Write-Host ''
Write-Host 'Pages to review:'
foreach ($p in ($pages.Keys | Sort-Object)) {
    Write-Host "  $p"
    $pages[$p] | Select-Object -Unique | ForEach-Object { Write-Host "      <- $_" }
}

$changedFiles = $changes | ForEach-Object { $_.File; $_.OldFile }
$stale = Get-SourceRefs | Where-Object { $changedFiles -contains $_.File }
Write-Host ''
Write-Host "Source references into changed files ($(@($stale).Count)), check their line numbers:"
$stale | Sort-Object Page, PageLine | ForEach-Object {
    $lines = if ($_.From) { ":$($_.From)" + $(if ($_.To) { "-$($_.To)" } else { '' }) } else { '' }
    Write-Host ("  {0}:{1}  \{2}{{{3}{4}}}" -f $_.Page, $_.PageLine, $_.Kind, $_.File, $lines)
}
Write-Host ''
Write-Host 'When the pages are updated: tools\docs-delta.ps1 -CheckLinks -Ref HEAD, then tools\docs-delta.ps1 -Pin, then build.ps1.'
