REM % https://stackoverflow.com/questions/11634784/rename-extracted-file-based-on-zip-file-in-batch

for %%f in (*.ProcSpec) do (
7z e "%%f"
del OOISignatures.xml
del OOIVersion.txt
move ps*.xml %%~nf.xml
)
