
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

void initialize_global_variables();
void clean_screen();
void customer(int process_id);
void int2string(char* str, int num);

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task			= task_table;
	PROCESS*	p_proc			= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16			selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	initialize_global_variables();
	
	k_reenter = 0;
	
	p_proc_ready = proc_table;

	/* 初始化 8253 PIT */
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
	out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

	put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
	enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

	restart();

	while(1){}
}

/*======================================================================*
                      initialize_global_variables
 *======================================================================*/
void initialize_global_variables()
{
	proc_table[0].ticks = proc_table[0].blocked = 0;
	proc_table[1].ticks = proc_table[0].blocked = 0;
	proc_table[2].ticks = proc_table[0].blocked = 0;
	proc_table[3].ticks = proc_table[0].blocked = 0;
	proc_table[4].ticks = proc_table[0].blocked = 0;
	
	ticks = 0;
	id = 0;	

	waiting = 0;
	customers.value = 0;
	customers.in = 0;
	customers.out = 0;
	mutex.value = 1;
	mutex.in = 0;
	mutex.out = 0;
	barber.value = 0;
	barber.in = 0;
	barber.out = 0;

	//设置光标位置
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, 0 & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, 0 & 0xFF);

	clean_screen();
}

/*======================================================================*
                               clean_screen 
 *======================================================================*/
void clean_screen()
{
	int i;
	disp_pos = 0;
	for(i=0; i<80*25; i++) {
		disp_str(" ");	
	}
	disp_pos = 0;
}

/*======================================================================*
                             Default Process
 *======================================================================*/
void TestA()
{
	while (TRUE) {
		milli_delay(200000);
		clean_screen();
	}
}

/*======================================================================*
                                 Barber
 *======================================================================*/
void TestB()
{
	while(TRUE) {
		if (waiting == 0) {
			print_color_str("B sleep.  ", MAKE_COLOR(BLACK, GREEN));
		}
		sem_p(&customers, 1);
		sem_p(&mutex, 1);
		waiting--;
		print_color_str("B start for ", MAKE_COLOR(BLACK, GREEN));
		sem_v(&barber);
		sem_v(&mutex);
		sleep(2000);
	}
}

/*======================================================================*
                               CustomerA
 *======================================================================*/
void TestC()
{
	while(TRUE) {
		customer(2);
	}
}


/*======================================================================*
                               CustomerB
 *======================================================================*/
void TestD()
{
	while(TRUE) {
		customer(3);
	}
}

/*======================================================================*
                               CustomerC
 *======================================================================*/
void TestE()
{
	while(TRUE) {
		customer(4);
	}
}

/*======================================================================*
                                Customer
 *======================================================================*/
void customer(int process_id)
{
	//记录客人ID，然后ID+1
	char id_output[10];
	int2string(id_output, id);
	id++;

	//输出客人到店的信息
	print_color_str("C", MAKE_COLOR(BLACK, RED));
	print_color_str(id_output, MAKE_COLOR(BLACK, RED));
	print_color_str(" come.  ", MAKE_COLOR(BLACK, RED));
	
	sem_p(&mutex, process_id);

	//判断是否等待队列已满
	if (waiting < CHAIRS) {
		waiting++;
		sem_v(&customers);
		sem_v(&mutex);
		sem_p(&barber, process_id);

		//输出客人开始理发的信息
		print_color_str("C", MAKE_COLOR(BLACK, GREEN));
		print_color_str(id_output, MAKE_COLOR(BLACK, GREEN));
		print_color_str(".  ", MAKE_COLOR(BLACK, GREEN));
	} else {
		sem_v(&mutex);

		//输出客人离店的信息
		print_color_str("C", MAKE_COLOR(BLACK, RED));
		print_color_str(id_output, MAKE_COLOR(BLACK, RED));
		print_color_str(" go.  ", MAKE_COLOR(BLACK, RED));
	}

	sleep(2000);
}


/*======================================================================*
                                int2string
 *======================================================================*/
void int2string(char* str, int num)
{
	char temp[10];
	int length = 0;
	int i = 0;

	do {
		temp[i++] = num%10 + '0'; //取下一个数字
		length++;
	} while ((num/=10) > 0); //删除该数字

	for(i=0; i<length; i++) {
		*(str+i) = temp[length-i-1];
	}
}

