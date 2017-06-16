
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
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

void find_next();


/*======================================================================*
                               schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS* p;
	for(p = proc_table; p < proc_table+NR_TASKS; p++) {
		if (p->ticks > 0) {
			p->ticks--;
		}		
	}
	find_next();
}


/*======================================================================*
                             sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}


/*======================================================================*
                               sys_sleep
 *======================================================================*/
PUBLIC void sys_sleep(int milli_sec)
{
	p_proc_ready->ticks = milli_sec*HZ/1000;
	find_next();	
}


/*======================================================================*
                              sys_disp_str
 *======================================================================*/
PUBLIC void sys_disp_str(char* str)
{
	disp_str(str);
}


/*======================================================================*
                           sys_disp_color_str
 *======================================================================*/
PUBLIC void sys_disp_color_str(char* str, int color)
{
	disp_color_str(str, color);
}


/*======================================================================*
                               sys_sem_p
 *======================================================================*/
PUBLIC void sys_sem_p(SEMAPHORE* sem, int index)
{	
	sem->value--;
	if(sem->value < 0) {
		sem->list[sem->in] = index;
		p_proc_ready->blocked = TRUE;
		find_next();
		sem->in = (sem->in+1) % QUEUE_SIZE;	
	}
}


/*======================================================================*
                               sys_sem_v
 *======================================================================*/
PUBLIC void sys_sem_v(SEMAPHORE* sem)
{
	sem->value++;
	if(sem->value <= 0) {
		int index = sem->list[sem->out];
		proc_table[index].blocked = FALSE;
		p_proc_ready = &proc_table[index];
		sem->out = (sem->out+1) % QUEUE_SIZE;	
	}
}


/*======================================================================*
                               find_next
 *======================================================================*/
void find_next()
{
	while(1) {
		p_proc_ready++;
		if(p_proc_ready >= proc_table + NR_TASKS) {
			p_proc_ready = proc_table;
		}
		if(p_proc_ready->ticks <= 0 && p_proc_ready->blocked != TRUE) {
			break;	
		}
	}
}


