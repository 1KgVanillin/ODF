Get-ChildItem -Recurse -Include *.cpp, *.hpp, *.c, *.h | Where-Object { $_.Name -ne "json.hpp" } |Get-Content | Measure-Object -Line
