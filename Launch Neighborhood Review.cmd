@echo off
setlocal
set "WildlineReview=%~dp0native\Build\NeighborhoodReview\Windows\Wildline.exe"
if not exist "%WildlineReview%" (
    echo The neighborhood review build has not been packaged yet.
    pause
    exit /b 1
)
start "" /D "%~dp0native\Build\NeighborhoodReview\Windows" "%WildlineReview%" -preferNvidia
