# Phase 9: Strip all OpenGL lines from files with dual ImRenderer paths.
# Lines containing gl*/glu* calls (but NOT g_imRenderer/g_renderStates/g_renderDevice) are removed.
# Also removes lines with only GL constants, GL_TEXTURE_2D enables, glLineWidth, glPointSize,
# glShadeModel, glTexParameteri, glMatrixMode, glLoadIdentity, glLoadMatrixd, gluOrtho2D, gluPerspective.
# Also removes CHECK_OPENGL_STATE() calls.

param([string[]]$Files, [switch]$DryRun)

foreach ($filePath in $Files) {
    if (-not (Test-Path $filePath)) { Write-Host "SKIP: $filePath not found"; continue }
    
    $lines = [System.IO.File]::ReadAllLines($filePath)
    $output = [System.Collections.Generic.List[string]]::new()
    $removed = 0
    $blankRun = 0
    
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        $trimmed = $line.Trim()
        
        # Never remove lines that are part of comments
        if ($trimmed -match '^\s*//' -or $trimmed -match '^\s*\*' -or $trimmed -match '^\s*/\*') {
            $output.Add($line); $blankRun = 0; continue
        }
        
        # Never remove lines that have ImRenderer / RenderStates / RenderDevice
        if ($trimmed -match 'g_imRenderer|g_renderStates|g_renderDevice|g_textureManager') {
            $output.Add($line); $blankRun = 0; continue
        }
        
        # Never remove #include lines (handled separately)
        if ($trimmed -match '^#include') {
            $output.Add($line); $blankRun = 0; continue
        }
        
        # Never remove lines with #define or #ifdef
        if ($trimmed -match '^#define|^#ifdef|^#ifndef|^#endif|^#else|^#pragma') {
            $output.Add($line); $blankRun = 0; continue
        }
        
        # Detect pure GL lines - lines that are ONLY a GL call
        $isGLLine = $false
        
        # Direct GL function calls
        if ($trimmed -match '^\s*gl[A-Z]\w*\s*\(' -and $trimmed -notmatch 'global|g_app|m_gl') { $isGLLine = $true }
        if ($trimmed -match '^\s*glu[A-Z]\w*\s*\(') { $isGLLine = $true }
        if ($trimmed -match '^\s*wgl[A-Z]\w*\s*\(') { $isGLLine = $true }
        
        # CHECK_OPENGL_STATE macro
        if ($trimmed -match '^CHECK_OPENGL_STATE') { $isGLLine = $true }
        
        # glEnd()
        if ($trimmed -match '^\s*glEnd\s*\(\s*\)\s*;') { $isGLLine = $true }
        
        # SwapBuffers (OpenGL flip)
        if ($trimmed -match '^\s*SwapBuffers\s*\(') { $isGLLine = $true }
        
        if ($isGLLine) {
            $removed++
            # Track blank lines to avoid double-blanks
            continue
        }
        
        # Collapse multiple blank lines
        if ($trimmed -eq '') {
            $blankRun++
            if ($blankRun -le 2) { $output.Add($line) }
            continue
        }
        
        $blankRun = 0
        $output.Add($line)
    }
    
    if ($removed -gt 0) {
        if ($DryRun) {
            Write-Host "$filePath : would remove $removed GL lines"
        } else {
            [System.IO.File]::WriteAllLines($filePath, $output.ToArray())
            Write-Host "$filePath : removed $removed GL lines"
        }
    } else {
        Write-Host "$filePath : no GL lines found"
    }
}
