@echo off
:: 开启延迟环境变量扩展（用于循环内变量累加）
setlocal enabledelayedexpansion
:: 设置控制台为 UTF-8 编码，防止中文提示乱码
chcp 65001 > nul

echo =======================================================
echo          AceTank TCP Protobuf 自动批量编译工具
echo =======================================================
echo.

set OUT_DIR=./
set FILE_COUNT=0

echo [1/2] 正在检查 protoc 编译器...
protoc --version > nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [错误] 找不到 protoc.exe！请检查环境变量设置。
    goto :end
)

echo [2/2] 开始扫描并编译当前目录下的 .proto 文件...
echo.

:: 遍历当前目录下所有的 .proto 文件
for %%f in (*.proto) do (
    echo   - 正在编译: %%f ...
    protoc --cpp_out=%OUT_DIR% "%%f"
    
    :: 如果当前文件编译失败，直接跳转到错误处理，中断执行
    if errorlevel 1 goto :error
    
    set /a FILE_COUNT+=1
)

:: 检查是否真的找到了文件
if !FILE_COUNT! EQU 0 (
    echo [提示] 没活干。当前目录下没有找到任何 .proto 文件。
    goto :end
)

:success
echo.
echo [成功] 完美！共编译了 !FILE_COUNT! 个文件。
echo 所有的 .pb.h 和 .pb.cc 文件已更新到 %OUT_DIR% 目录下。
goto :end

:error
echo.
echo [失败] 编译中断！请检查上方报错的 proto 文件语法。

:end
echo.
echo =======================================================
pause