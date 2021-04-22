# Preliminary Testcase batch file

# Run this script in powershell:
# run.ps1
# Check command line to verify SAT/UNSAT

# $testcaseFolder = ".\testcase-sat150"
# $testcaseFolder = ".\testcase-unsat150"
# $testcaseFolder = ".\script_test_folder"
# $testoutput = ".\output.txt"

Get-ChildItem $testcaseFolder | 
Foreach-Object {
    $testfile = $_.DirectoryName + '\' + $_.Name
    # Start-Process .\Source.exe -RedirectStandardInput $testfile -NoNewWindow -Wait -redirectstandardoutput $testoutput
    Start-Process .\Source.exe -RedirectStandardInput $testfile -NoNewWindow -Wait
}