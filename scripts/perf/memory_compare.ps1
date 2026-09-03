<#
.SYNOPSIS
    Samples total memory usage across a named set of processes over time and
    logs it to CSV, for comparing the embedded-browser build against the
    legacy media-plugin build.

.DESCRIPTION
    Neither build's memory usage lives in a single process, so this sums
    across the right set for each side rather than looking at secondlifeviewer
    alone:
      - legacy (media plugin):   secondlifeviewer, SLPlugin, dullahan_host
      - this build (embedded):   secondlifeviewer, SLMediaProducer

    The two sides' helper-process naming is NOT symmetric -- easy to miss and
    invalidates the comparison if you do (confirmed the hard way: an earlier
    comparison run omitted dullahan_host and undercounted legacy by roughly
    3x). SLMediaProducer re-execs itself for every CEF subprocess (renderer,
    GPU, network/storage utility), so a single -ProcessNames SLMediaProducer
    entry already catches all of them. The legacy media plugin's CEF
    subprocesses run under a DIFFERENT binary name, dullahan_host.exe, not
    SLPlugin.exe -- SLPlugin.exe is only the thin per-instance coordinator,
    and each of its own dullahan_host.exe children again spawns its own full,
    independent GPU + 2 utility + renderer set (no sharing across instances,
    unlike the embedded side's one shared SLMediaProducer family). Both
    dullahan_host and SLPlugin must be passed for a legacy run to be complete.

    Note the process name to pass for the Viewer's own main process is
    secondlifeviewer (not secondlife-bin) when testing an installed/packaged
    build (the CI installer, or anything under %LocalAppData%) -- that's the
    name viewer_manifest.py ships it under for both legacy and embedded
    builds alike. secondlife-bin is only the raw build-output name for a
    local, unpackaged dev build.

    Logs one row per process name per sample (Count/WorkingSetMB/PrivateMB),
    plus one "TOTAL" row per sample summing across every name passed in.
    This fixed, process-name-agnostic schema is deliberate: a legacy run and
    an embedded run pass different -ProcessNames, so a wide layout (one
    column per process name) would produce a different CSV header for each
    run and break appending two runs into the same file. This long/tidy
    layout appends cleanly regardless of which names either run used, and
    pivots easily in Excel/PowerBI (group by Label + ProcessName).

    WorkingSet is physical RAM in use (what Task Manager's "Memory" column
    shows); PrivateMemorySize is memory not shareable with any other
    process. Note WorkingSet will slightly over-count relative to true
    unique physical memory for the embedded build's TOTAL row, since the
    llshmframe segment is mapped into both secondlife-bin and
    cefshm_producer and WorkingSet counts mapped pages per-process; use
    VMMap if you need the precise shared-vs-private split.

.PARAMETER ProcessNames
    Process names to sample (no .exe suffix). For a complete legacy run this
    must include dullahan_host, not just SLPlugin -- see .DESCRIPTION.

.PARAMETER Label
    Free-text tag written into every row (e.g. "legacy" or "embedded") so
    two separate runs can be appended into the same CSV and graphed as
    separate series.

.PARAMETER OutFile
    CSV path to append to. Defaults to a timestamped file in the current
    directory.

.PARAMETER IntervalSeconds
    Seconds between samples. Default 5.

.PARAMETER DurationMinutes
    Stop after this many minutes. Default 0 (run until Ctrl+C).

.EXAMPLE
    .\memory_compare.ps1 -ProcessNames secondlifeviewer,SLPlugin,dullahan_host -Label legacy -OutFile mem.csv -DurationMinutes 5

.EXAMPLE
    .\memory_compare.ps1 -ProcessNames secondlifeviewer,SLMediaProducer -Label embedded -OutFile mem.csv -DurationMinutes 5
#>
param(
    [Parameter(Mandatory = $true)]
    [string[]]$ProcessNames,

    [string]$Label = "",

    [string]$OutFile = "memory-log-$(Get-Date -Format 'yyyyMMdd-HHmmss').csv",

    [int]$IntervalSeconds = 5,

    # double, not int -- a fractional value silently rounds to 0 under [int],
    # and 0 means "run forever" below, so e.g. -DurationMinutes 0.5 would
    # otherwise turn into an infinite run with no warning.
    [double]$DurationMinutes = 0
)

function Get-TotalMB {
    param([double[]]$Values)
    if (-not $Values -or $Values.Count -eq 0) { return 0 }
    $sum = ($Values | Measure-Object -Sum).Sum
    if (-not $sum) { return 0 }
    return [math]::Round($sum / 1MB, 1)
}

function New-Row {
    param([string]$ProcessName, [int]$Count, [double]$WorkingSetMB, [double]$PrivateMB)
    [pscustomobject][ordered]@{
        Timestamp    = $script:sampleTime
        Label        = $Label
        ProcessName  = $ProcessName
        Count        = $Count
        WorkingSetMB = $WorkingSetMB
        PrivateMB    = $PrivateMB
    }
}

$deadline = [datetime]::MaxValue
if ($DurationMinutes -gt 0) {
    $deadline = (Get-Date).AddMinutes($DurationMinutes)
}

Write-Host "Logging memory for: $($ProcessNames -join ', ') every $IntervalSeconds s to $OutFile (Ctrl+C to stop)"

while ((Get-Date) -lt $deadline) {
    $allProcs = Get-Process -Name $ProcessNames -ErrorAction SilentlyContinue
    $script:sampleTime = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'

    $rows = @()
    foreach ($name in $ProcessNames) {
        $group = @($allProcs | Where-Object { $_.ProcessName -eq $name })
        $rows += New-Row -ProcessName $name -Count $group.Count `
            -WorkingSetMB (Get-TotalMB $group.WorkingSet64) -PrivateMB (Get-TotalMB $group.PrivateMemorySize64)
    }
    $totalRow = New-Row -ProcessName "TOTAL" -Count @($allProcs).Count `
        -WorkingSetMB (Get-TotalMB $allProcs.WorkingSet64) -PrivateMB (Get-TotalMB $allProcs.PrivateMemorySize64)
    $rows += $totalRow

    $rows | Export-Csv -Path $OutFile -NoTypeInformation -Append

    Write-Host "$($script:sampleTime)  [$Label]  TotalWorkingSetMB=$($totalRow.WorkingSetMB)  TotalPrivateMB=$($totalRow.PrivateMB)"

    Start-Sleep -Seconds $IntervalSeconds
}

Write-Host "Done. Log written to $OutFile"
