Write-Host "Building parsing table component..."

# Compile source files
$sources = @(
    "run_parsing_table.cpp",
    "parser/parsing_table.cpp",
    "utils/first_follow.cpp",
    "utils/utils.cpp",
    "utils/left_recursion.cpp",
    "utils/left_factoring.cpp",
    "utils/trie/memory_efficient_trie.cpp",
    "grammar/grammar.cpp",
    "grammar/grammar_transform.cpp"
)

$objects = @()
foreach ($source in $sources) {
    $obj = $source -replace '\.cpp$', '.o'
    $command = "g++ -std=c++17 -c $source -I include -o $obj"
    Write-Host "Compiling $source..."
    Invoke-Expression $command
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to compile $source"
        exit 1
    }
    $objects += $obj
}

# Link the object files
$objList = $objects -join " "
$command = "g++ $objList -o parsing_table_run.exe"
Write-Host "Linking..."
Invoke-Expression $command

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful! Executable created at parsing_table_run.exe"
    # Clean up object files
    foreach ($obj in $objects) {
        Remove-Item $obj
    }
} else {
    Write-Host "Linking failed!"
} 