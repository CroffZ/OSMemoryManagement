
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);
extern void select_console(int nr_console);
extern void init_screen(TTY* p_tty);
extern void search(CONSOLE* p_con,int search_char_length);
extern void clean(CONSOLE* p_con);
extern int is_current_console(CONSOLE* p_con);
extern void keyboard_read(TTY* p_tty);

PRIVATE int stop_input;
PRIVATE int esc_flag;
PRIVATE int search_char_length;
extern char console_color;

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	TTY*	p_tty;

	init_keyboard();

	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
		init_tty(p_tty);
	}
	select_console(0);
	while (1) {
		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			//对每一个TTY进行读写操作
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}

/*======================================================================*
			   init_tty
 *----------------------------------------------------------------------*
  初始化全局变量，启动屏幕
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	search_char_length = 0;
	esc_flag = 0;
	stop_input = 0;

	init_screen(p_tty);
}

/*======================================================================*
				in_process
 *----------------------------------------------------------------------*
  处理从keyboard传来的字符，若需要输出则放入缓存区中
 *======================================================================*/
PUBLIC void in_process(TTY* p_tty, u32 key)
{
        char output[2] = {'\0', '\0'};

		if (esc_flag%2==1 && ((key & MASK_RAW)!=ESC)) {  
			if (!stop_input) {
				search_char_length++;
			}
		}

        if (!(key & FLAG_EXT)) {
        	//可直接打印
			if (!stop_input) {
				put_key(p_tty, key);
			}
        } else {
        	//不可直接打印
			int raw_code = key & MASK_RAW;
			switch(raw_code) {
			case ENTER:
				console_color = DEFAULT_CHAR_COLOR;
				//判断是否要搜索
				if ((esc_flag)%2==0) {
					//esc_flag为0，正常换行
					put_key(p_tty, '\n');
				} else {
					//esc_flag为1，启动console的搜索函数search()，并且屏蔽新的输入
					search(p_tty->p_console,search_char_length); 
					stop_input = 1;
				}
				break;
			case ESC:
				esc_flag++;
				//需要判断第一次还是第二次按下Esc
				if (esc_flag%2==1) {
					//第一次按下，进入搜索模式
					//改变字体颜色，等待输入要搜索的文本，esc_flag为1
					console_color = SEARCH_CHAR_COLOR;
				} else {
					//第二次按下，退出搜索模式
					//删除搜索文本，删除搜索匹配文本的颜色
					clean(p_tty->p_console);
					for(; search_char_length>1; search_char_length--) {
						put_key(p_tty, '\b');
					}
					search_char_length = 0;

					//切换回默认字体颜色，关闭屏蔽输入
					console_color = DEFAULT_CHAR_COLOR;
					stop_input = 0;
				}
				break;
			case BACKSPACE:
				if (!stop_input) {
					put_key(p_tty, '\b');
				}
				break;
			case TAB:
				if (!stop_input) {
						put_key(p_tty, '\t');
				}
				break;
			case UP:
				if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
					scroll_screen(p_tty->p_console, SCR_DN);
				}
				break;
			case DOWN:
				if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
					scroll_screen(p_tty->p_console, SCR_UP);
				}
				break;
			case F1:
			case F2:
			case F3:
			case F4:
			case F5:
			case F6:
			case F7:
			case F8:
			case F9:
			case F10:
			case F11:
			case F12:
				/* Alt + F1~F12 */
				if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
					select_console(raw_code - F1);
				}
				break;
			default:
				break;
			}
        }
}

/*======================================================================*
			      put_key
 *----------------------------------------------------------------------*
  将输入的字符存入缓存区，若缓存区已满则直接丢弃
*======================================================================*/
PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *----------------------------------------------------------------------*
  从键盘读取字符
 *======================================================================*/
PRIVATE void tty_do_read(TTY* p_tty)
{
	if (is_current_console(p_tty->p_console)) {
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      tty_do_write
 *----------------------------------------------------------------------*
  将缓冲区的字符输出到console
 *======================================================================*/
PRIVATE void tty_do_write(TTY* p_tty)
{
	if (p_tty->inbuf_count) {
		//如果缓冲区有数据就进行操作
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++; //指向tty缓冲区中下一个应处理的键值
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			//判断缓冲区是否已经装满
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch);
	}
}


