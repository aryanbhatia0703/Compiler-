# Display the parse trace in a readable format

param (
    [string]$TraceFile = "backend/build/parse_trace.json",
    [string]$OutputFile = "backend/build/parse_trace.txt"
)

Write-Host "LL(1) Parser Trace Visualizer" -ForegroundColor Cyan
Write-Host "=============================" -ForegroundColor Cyan

# Check if the trace file exists
if (-not (Test-Path $TraceFile)) {
    Write-Host "Error: Trace file '$TraceFile' not found." -ForegroundColor Red
    Write-Host "Run the parser first using: cd backend/build; .\parser_run.exe" -ForegroundColor Yellow
    exit 1
}

# Read and parse the JSON trace file
$traceJson = Get-Content $TraceFile -Raw | ConvertFrom-Json

# Display the parse trace as a table
Write-Host ""
Write-Host "Step | Stack Top | Input | Action" -ForegroundColor Green
Write-Host "---------------------------------------------" -ForegroundColor Green

$step = 1
foreach ($entry in $traceJson) {
    $line = "{0}`t| {1}`t| {2}`t| {3}" -f $step, $entry.stack_top, $entry.input, $entry.action
    
    # Choose color based on action type
    if ($entry.action -eq "match") {
        Write-Host $line -ForegroundColor Green
    } 
    elseif ($entry.action -eq "accept") {
        Write-Host $line -ForegroundColor Cyan
    }
    elseif ($entry.action.StartsWith("expand:")) {
        Write-Host $line -ForegroundColor Yellow
    }
    elseif ($entry.action.StartsWith("error:")) {
        Write-Host $line -ForegroundColor Red
    }
    else {
        Write-Host $line -ForegroundColor White
    }
    
    $step++
}

Write-Host ""
if ($traceJson[-1].action -eq "accept") {
    Write-Host "Parsing completed successfully!" -ForegroundColor Green
} 
else {
    Write-Host "Parsing failed. See error details above." -ForegroundColor Red
} 