# Preliminary Testcase batch file

# Run this script in powershell:
# run.ps1
# Check command line to verify SAT/UNSAT

$testcaseFolder = ".\testcase-sat1"

Get-ChildItem $testcaseFolder | 
Foreach-Object {
    $testfile = $_.DirectoryName + '\' + $_.Name
    Start-Process .\solver.exe -RedirectStandardInput $testfile -NoNewWindow -Wait
    ""
}