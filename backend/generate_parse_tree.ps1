# Generate a parse tree visualization from the trace file

param (
    [string]$TraceFile = "backend/build/parse_trace.json",
    [string]$OutputFile = "backend/build/parse_tree.txt"
)

Write-Host "LL(1) Parser Tree Generator" -ForegroundColor Cyan
Write-Host "===========================" -ForegroundColor Cyan

# Check if the trace file exists
if (-not (Test-Path $TraceFile)) {
    Write-Host "Error: Trace file '$TraceFile' not found." -ForegroundColor Red
    Write-Host "Run the parser first using: cd backend/build; .\parser_run.exe" -ForegroundColor Yellow
    exit 1
}

# Read and parse the JSON trace file
$traceJson = Get-Content $TraceFile -Raw | ConvertFrom-Json

# Create a tree structure from the trace
$rootNode = @{
    "symbol" = $traceJson[0].stack_top
    "children" = @()
    "depth" = 0
}

$currentNode = $rootNode
$nodeStack = @()

foreach ($entry in $traceJson) {
    if ($entry.action.StartsWith("expand:")) {
        # Extract production from action
        $production = $entry.action.Substring("expand: ".Length)
        $parts = $production -split " → "
        $lhs = $parts[0]
        $rhs = $parts[1] -split " "
        
        # Create children nodes for each symbol in RHS (except epsilon)
        foreach ($symbol in $rhs) {
            if ($symbol -ne "ε" -and $symbol -ne "Îµ") {
                $childNode = @{
                    "symbol" = $symbol
                    "children" = @()
                    "depth" = $currentNode.depth + 1
                }
                $currentNode.children += $childNode
            }
        }
        
        # Push current node to stack and make first child the current node (if any)
        if ($currentNode.children.Count -gt 0) {
            $nodeStack += $currentNode
            $currentNode = $currentNode.children[0]
        }
    }
    elseif ($entry.action -eq "match") {
        # When a terminal is matched, move to the next node
        if ($nodeStack.Count -gt 0) {
            $parent = $nodeStack[-1]
            $childIndex = $parent.children.IndexOf($currentNode) + 1
            
            if ($childIndex -lt $parent.children.Count) {
                # Move to next sibling
                $currentNode = $parent.children[$childIndex]
            }
            else {
                # Move back to parent and remove from stack
                $currentNode = $nodeStack[-1]
                $nodeStack = $nodeStack[0..($nodeStack.Count - 2)]
            }
        }
    }
}

# Function to output the tree
function Write-Tree {
    param (
        [Parameter(Mandatory=$true)]
        $Node,
        [string]$Prefix = "",
        [bool]$IsLast = $true
    )
    
    # Output current node
    $line = $Prefix
    if ($Prefix -ne "") {
        if ($IsLast) {
            $line += "└── "
            $childPrefix = $Prefix + "    "
        } else {
            $line += "├── "
            $childPrefix = $Prefix + "│   "
        }
    }
    $line += $Node.symbol
    
    # Write to file
    Add-Content -Path $OutputFile -Value $line
    
    # Process children
    for ($i = 0; $i -lt $Node.children.Count; $i++) {
        $isLastChild = ($i -eq ($Node.children.Count - 1))
        Write-Tree -Node $Node.children[$i] -Prefix $childPrefix -IsLast $isLastChild
    }
}

# Generate the tree
"Parse Tree for Input Tokens" | Out-File -FilePath $OutputFile
"==========================" | Add-Content -Path $OutputFile
"" | Add-Content -Path $OutputFile
Write-Tree -Node $rootNode

Write-Host "Parse tree generated and saved to: $OutputFile" -ForegroundColor Green
Write-Host "You can view it using: notepad $OutputFile" -ForegroundColor Yellow 