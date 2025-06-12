set source_dir="E:\keke\bin\tex"

for /R %source_dir% %%f in (*.*) do (
    E:/keke/bin/astcenc-sse2.exe -cs tex/%%~nxf _tex_astc/%%~nxf.astc 8x8 -medium
    echo SECCESS: %%~nxf
)
