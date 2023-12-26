; Typora
; 快捷增加字体颜色
; SendInput {Text} 解决中文输入法问题
 
#IfWinActive ahk_exe Typora.exe
{
    ; Ctrl+O 橙色
    ^!o::addFontColor("orange")
 
    ; Ctrl+R 红色
    ^!r::addFontColor("red")
 
    ; Ctrl+B 浅蓝色
    ^!b::addFontColor("cornflowerblue")

    ; Ctrl+Shift+G 绿色
    ^+g::addFontColor("green")

    ; Ctrl+Shift+Y 黄色
    ^+y::addFontColor("yellow")

    ; Ctrl+空格 居中
    ^SPACE::addCenterStyle()
}
 
; 快捷增加字体颜色
addFontColor(color){
    clipboard := "" ; 清空剪切板
    Send {ctrl down}c{ctrl up} ; 复制
    SendInput {TEXT}<font color='%color%'>
    SendInput {ctrl down}v{ctrl up} ; 粘贴
    If(clipboard = ""){
        SendInput {TEXT}</font> ; Typora 在这不会自动补充
    }else{
        SendInput {TEXT}</ ; Typora中自动补全标签
    }
}

; 快捷添加居中样式
addCenterStyle(){
    clipboard := "" ; 清空剪切板
    Send {ctrl down}c{ctrl up} ; 复制
    SendInput {TEXT}<div align='center'>
    SendInput {ctrl down}v{ctrl up} ; 粘贴
    SendInput {TEXT}</ ; Typora中自动补全标签
    
}
