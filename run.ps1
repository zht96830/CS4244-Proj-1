# Preliminary Testcase batch file

# Run this script in powershell:
# run.ps1
# Check command line to verify SAT/UNSAT

# $testcaseFolder = ".\testcase-sat1"
# $testcaseFolder = ".\testcase-unsat1"
$testcaseFolder = ".\test"

Get-ChildItem $testcaseFolder | 
Foreach-Object {
    $testfile = $_.DirectoryName + '\' + $_.Name
    Start-Process .\Source.exe -RedirectStandardInput $testfile -NoNewWindow -Wait
}