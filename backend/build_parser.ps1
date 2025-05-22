# Build script for the LL(1) parser

# Create build directory if it doesn't exist
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build"
}

# Navigate to build directory
Set-Location build

# Run CMake to generate build files
cmake ..

# Build the parser
cmake --build .

# Check if build was successful
if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Parser build successful!"
    
    # Copy the executable to the backend directory
    Copy-Item ".\parser_run.exe" "..\backend\" -Force
    
    Write-Host "✅ Executable copied to backend directory"
    Write-Host "You can now run the parser using: .\backend\parser_run.exe"
} else {
    Write-Host "❌ Build failed with error code: $LASTEXITCODE"
}

# Return to original directory
Set-Location .. 