section .data
	prompt_message: db 'fibonaccionacci Numbers:', 10
	.len equ $-prompt_message
	new_line: db 10
	.len equ $-new_line
	input_count: dd 0

	fibonacci_count: dd 0
	fibonacci_bit: dd 15
	fibonacci_result: times 15 db '0'
    fill_with_space: db ' '
	color_choice: db 0
	fibonacci_1: times 128 db '0'

	fibonacci_2: times 127 db '0'
	fill_with_1: db '1'
	digit_index: db 1
	fibonacci_length equ 128
	digit_10: dd 10

	color_1: db 27, '[31;1m', 0 ;red
	.len equ $-color_1
	color_2: db 27, '[32;1m', 0 ;green
	.len equ $-color_2
	color_3: db 27, '[33;1m', 0 ;yellow
	.len equ $-color_3


section .bss
	input_location: resb 63
	.len equ $-input_location
	one_number_length: resb 1
	numbers_location: resd 1
	number: resd 1
	input_numbers: resd 10


section .text
global  _start:
_start:
	;show prompt message
	mov rax, 4
	mov rbx, 1
	mov rcx, prompt_message
	mov rdx, prompt_message.len
	int 80h

	;get input from console
	mov rax, 3
	mov rbx, 0
	mov rcx, input_location
	mov rdx, input_location.len
	int 80h

	;change the last char of input from '\n' to ' '
	mov rbx, input_location
	mov rcx, rbx
	add rcx, rax
	dec rcx
	mov byte[rcx], ' '
	mov qword[numbers_location], rbx

process_input_numbers:
	mov dh, byte[rbx]
	cmp dh, ' '
	je got_one_number
	inc rbx
	jmp process_input_numbers

got_one_number:
	mov rdx, rbx
	mov rcx, [numbers_location]
	sub rdx, rcx
	mov [one_number_length], dl
 
	push rax
	push rbx
	push rcx
	push rdx
	call string_to_int
	pop rdx
	pop rcx
	pop rbx
	pop rax

	;check if input is end
	;by comparing rbx-rcx and the length of input
	;if so, end processing input and go to fibonaccionacci
	;if not, reset rbx and continue processing
	inc rbx
	mov rcx, input_location
	mov rdx, rbx
	sub rdx, rcx
	cmp rdx, rax
	je  finish_get_input
	mov qword[numbers_location], rbx
	jmp process_input_numbers

finish_get_input:
	call fibonacci

	;exit progrom
	mov eax, 1
	mov ebx, 0
	int 80h

string_to_int:
	mov ch, 0
	mov rbx, 0
	mov cl, [one_number_length]

process_single_char:
	cmp ch, cl
	je finish_string_to_int
	mov rdx, [numbers_location]  
	movzx eax, ch
	add rdx, rax
	mov ah, [rdx]
	sub ah, '0'
	movzx eax, ah
	cmp rax, 0
	je next_char
	mov dh, cl
	sub dh, ch
	dec dh

multifly_digit_10:
	cmp dh, 0
	je next_char
	push rdx
	mov rdx, [digit_10]
	mul rdx
	pop rdx
	dec dh
	jmp multifly_digit_10

next_char:
	add rbx, rax
	inc ch
	jmp process_single_char

finish_string_to_int:
	mov rcx, [input_count]
	inc rcx
	mov [input_count], rcx
	dec rcx
	sal rcx, 2
	mov rdx, input_numbers
	mov [rdx+rcx], rbx  
	ret

fibonacci:
	mov rax, [input_count]

fetch_all_numbers:
	cmp rax, 0
	je calculate
	mov rcx, input_numbers
	mov rbx, rax
	dec rbx
	sal rbx, 2 ;every decimal number stores as a word
	add rbx, rcx
	mov rbx, [rbx]
	push rbx ;push all input numbers in stack
	dec rax
	jmp fetch_all_numbers

calculate:
	pop rdx
	cmp rdx, 0
	jne calculate_single_number
	mov rdx, [fibonacci_count]
	inc rdx
	mov [fibonacci_count], rdx
	pop rdx
	call print_number_with_color

calculate_single_number:
	push rax
	mov eax, [fibonacci_count]
	cmp eax, [input_count]
	je finish_fibonacci
	pop rax
	call fibonacci_add
	inc eax
	cmp eax, edx
	jne calculate_single_number
	mov edx, [fibonacci_count]
	inc edx
	mov [fibonacci_count], edx
	pop rdx
	call print_number_with_color
	jmp calculate_single_number

finish_fibonacci:
	pop rax
	push rdx
	ret

fibonacci_add:
	push rax
	push rcx
	push rdx
	mov rbx, 0
	mov rax, fibonacci_1
	mov rcx, fibonacci_2
	add rax, fibonacci_length
	add rcx, fibonacci_length
	dec rax
	dec rcx
	mov dl, [digit_index]
	mov dh, 0

add_calculate:
	cmp dh, dl
	je finish_add_calculate
	push rdx
	mov dl, byte[rax]
	mov dh, byte[rcx]
	mov byte[rax], dh
	add dh, dl
	add dh, bl
	mov bl, 0
	sub dh, '0'
	cmp dh, '9'
	jle next_number
	mov bl, 1
	sub dh, 10

next_number:
	mov byte[rcx], dh
	pop rdx
	inc dh
	dec rcx
	dec rax
	jmp add_calculate

finish_add_calculate:
	mov bh, [digit_index]
	cmp bl, 0
	je reset
	inc bh
	mov [digit_index], bh
	mov byte[rcx], '1'

reset:
	pop rdx
	pop rcx
	pop rax
	ret
  
print_number_with_color:
	push rax
	push rbx
	push rcx
	push rdx
	call choose_color
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret

choose_color:
	mov rax, 4
	mov rbx, 1
	mov bl, [color_choice]
	cmp bl, 0
	je choose_color_1
	cmp bl, 1
	je choose_color_2
	jmp choose_color_3

choose_color_1:
	mov bl, 1
	mov [color_choice], bl
	mov rcx, color_1
	mov rdx, color_1.len
	int 80h
 	jmp print_result

choose_color_2:
	mov bl, 2
	mov [color_choice], bl
	mov rcx, color_2
	mov rdx, color_2.len
	int 80h
	jmp print_result

choose_color_3:
	mov bl, 0
	mov [color_choice], bl
	mov rcx, color_3
	mov rdx, color_3.len
	int 80h
	jmp print_result

print_result:
	mov rcx, fibonacci_2
	mov dh, [digit_index]
	mov dl, fibonacci_length
	movzx eax, dl
	movzx edx, dh

	add rcx, rax
	sub rcx, rdx
	mov rax, 4
	mov rbx, 1
	int 80h

	mov rax, 4
	mov rbx, 1
	mov rcx, new_line
	mov rdx, new_line.len
	int 80h
	ret