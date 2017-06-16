
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);
PUBLIC void scroll_screen(CONSOLE* p_con, int direction);
PUBLIC void search(CONSOLE* p_con,int search_char_num);
PUBLIC void clean(CONSOLE* p_con);
PUBLIC void clear_screen(CONSOLE* p_con);
PUBLIC void init_screen(TTY* p_tty);
PUBLIC int is_current_console(CONSOLE* p_con);
PUBLIC void out_char(CONSOLE* p_con, char ch);

char console_color = DEFAULT_CHAR_COLOR;

extern void disable_int();
extern void enable_int();

/*======================================================================*
			   init_screen
 *----------------------------------------------------------------------*
  初始化屏幕参数和全局变量
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	} else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
 *----------------------------------------------------------------------*
  判断是否是当前显示的屏幕
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   out_char
 *----------------------------------------------------------------------*
  输出普通字符和处理特殊字符
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2); //指向当前位置

	switch(ch) {
	case '\n':	//换行键
		if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH) {
			*p_vmem++ = '\n';
			*p_vmem++ = 0x00;
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH * ((p_con->cursor - p_con->original_addr) / SCREEN_WIDTH + 1);
		}
		break;
	case '\t':	//Tab键
		if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 1) {
			int i = 0;
			int len = 4 - (p_con->cursor%4);
			// 位置移动到4对齐的位置
			for (; i < len; i++) {
				//字符位用0填充
				*p_vmem = 0;
				p_vmem += 2;
			}
			p_con->cursor += len;
		}
		break;
	case '\b':	//退格键
		if (p_con->cursor > p_con->original_addr) {
			if (p_con->cursor % SCREEN_WIDTH == 0) {
				//先查找前面的字符是不是换行
				
				//while(p_con->cursor % SCREEN_WIDTH == 0) {
				//	if(p_con->cursor == p_con->original_addr) {
				//		break;
				//	}

					p_vmem -= SCREEN_WIDTH * 2;
					int i = 0;
					int end = 0;
					for (; i < SCREEN_WIDTH; i++) {
						if (*p_vmem == '\n') {
							p_con->cursor -= SCREEN_WIDTH - i;
							*p_vmem = ' ';
							*(p_vmem + 1) = console_color;
							end = 1;
							break;
						}
						p_vmem += 2;
					}
				//}
			} else if (*(p_vmem-2) == 0) {
				//再查找前面的字符是不是Tab（字符位用0填充）
				int i = 0;
				for (; i < 4; i++) {
					p_vmem -= 2;
					p_con->cursor--;
					if (*(p_vmem - 2) != 0) {
						break;
					}
				}
			} else {
				//以上情况都不是则当作正常退格处理
				*(p_vmem-2) = ' ';
				*(p_vmem-1) = console_color;
				p_con->cursor--;
			}
		}
		break;
	default:
		if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 1) {
			*p_vmem++ = ch;
			*p_vmem++ = console_color;
			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor < p_con->current_start_addr) {
		//向上滚屏
		scroll_screen(p_con, SCR_UP);
	}
	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		//向下滚屏
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
 *----------------------------------------------------------------------*
  更新光标位置和显示区域
 *======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
	set_cursor(p_con->cursor);
	//set the position of cursor
	set_video_start_addr(p_con->current_start_addr);
	//set the start address of video in memory
}

/*======================================================================*
			    set_cursor
 *----------------------------------------------------------------------*
  设置光标位置
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *----------------------------------------------------------------------*
  设置显存起始地址，即设置显示区域
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *----------------------------------------------------------------------*
  切换屏幕
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	//set the position of cursor
	set_video_start_addr(console_table[nr_console].current_start_addr);
	//set the start address of video in memory
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
  滚屏
 *----------------------------------------------------------------------*
  direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	} else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE < p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	} else {

	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

/*======================================================================*
			   clear_screen
 *----------------------------------------------------------------------*
  清除当前屏幕内容，光标回到屏幕左上角
 *======================================================================*/
PUBLIC void clear_screen(CONSOLE* p_con)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	for (; p_con->cursor > p_con->original_addr; p_con->cursor--) {
		*--p_vmem = console_color;
		*--p_vmem = ' ';
	}
	flush(p_con);
}

/*======================================================================*
			   search
 *----------------------------------------------------------------------*
  搜索输入的字符串，将匹配成功的字符串改变颜色
 *======================================================================*/
PUBLIC void search(CONSOLE* p_con,int search_char_length)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2-search_char_length*2+2); 
	//指向search的第一个字符的位置
	u8* search_vmem = (u8*)(V_MEM_BASE); 
	//指向开头位置

	for(; search_vmem<p_vmem; search_vmem+=2) {
		if(*(search_vmem) == *(p_vmem)) {
			int i;
			int equ_flag = 1;
			//当equ_flag为1时表示相等
			for(i=1; i<search_char_length-1; i++) {
				if(*(search_vmem+i*2) == *(p_vmem+i*2)) {
					
				} else {
					equ_flag = 0;
					break;
				}
			}

			if(equ_flag == 1) {
			//相等的情况下进行颜色的转换
				for(i=0; i<search_char_length-1; i++) {
					*(search_vmem + 2*i + 1) = SEARCH_CHAR_COLOR;
				}
			}
		}
	}

	flush(p_con);
}

/*======================================================================*
			   clean
 *----------------------------------------------------------------------*
  清除搜索中匹配成功的字符串的颜色，变回默认颜色
 *======================================================================*/
PUBLIC void clean(CONSOLE* p_con)
{
	u8* search_vmem = (u8*)(V_MEM_BASE);//指向开头位置
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	for(; search_vmem<p_vmem; search_vmem+=2) {
		if(*(search_vmem+1) == SEARCH_CHAR_COLOR) {
			*(search_vmem+1)=DEFAULT_CHAR_COLOR;
        }
	}

	flush(p_con);
}
