@echo off
cd /d "C:\Users\jiangzhuangzhuang\study\study"   REM 替换为您的代码仓库路径

        REM 执行Git命令向远程仓库推送代码
        git add .
        git commit -m "自动推送代码"
        git push origin master
    

