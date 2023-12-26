@echo off
setlocal enabledelayedexpansion

set "searchDir=C:\Users\jiangzhuangzhuang\workspace\24MM\UCM\updatecomposer\updateagent\src\main\java\com\iauto\updateagent\common\ziptools"
set "outputFile=C:\Users\jiangzhuangzhuang\Desktop\log\log.txt"

REM 遍历指定目录下的所有 Java 文件
for /r "%searchDir%" %%G in (*.java) do (
    echo process file: %%G
    echo.

    REM 提取方法名并输出到文件中
    for /f "tokens=2 delims=()" %%A in ('findstr /r "^\s*public\s*static\s*final.*\(.*\)" "%%G"') do (
        set "methodName=%%A"
        set "className=%%~nG"
        set "className=!className:.java=!"

        REM 输出到文件
		echo public static final short LOG_ID_!className!_!methodName! = 0x0000;
        echo public static final short LOG_ID_!className!_!methodName! = 0x0000; >> "%outputFile%"
    )

    echo.
)

echo finish！

pause

