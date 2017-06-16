GLOBAL printStr
GLOBAL printFileName
GLOBAL printFileCatalog

section .data
	color_default: db 27, '[37;1m', 0
	.len equ $-color_default
	color_red: db 27, '[31;1m', 0 ;red
	.len equ $-color_red
	color_yellow: db 27, '[33;1m', 0 ;yellow
	.len equ $-color_yellow

section .text
printStr:
	push ebp
	mov ebp, esp
	mov eax, 4
	mov ebx, 1
	mov ecx, color_default
	mov edx, color_default.len
	int 80h

	mov eax, 4
	mov ebx, 1
	mov ecx, [esp+8] ;首字符的地址
	mov edx, [esp+12] ;字符串的长度
	int 80h

	mov esp, ebp
	pop ebp
	ret

printFileName:
	push ebp
	mov ebp, esp
	mov eax, 4
	mov ebx, 1
	mov ecx, color_red
	mov edx, color_red.len
	int 80h

	mov eax, 4
	mov ebx, 1
	mov ecx, [esp+8] ;首字符的地址
	mov edx, [esp+12] ;字符串的长度
	int 80h

	mov esp, ebp
	pop ebp
	ret

printFileCatalog:
	push ebp
	mov ebp, esp
	mov eax, 4
	mov ebx, 1
	mov ecx, color_yellow
	mov edx, color_yellow.len
	int 80h

	mov eax, 4
	mov ebx, 1
	mov ecx, [esp+8] ;首字符的地址
	mov edx, [esp+12] ;字符串的长度
	int 80h

	mov esp, ebp
	pop ebp
	ret