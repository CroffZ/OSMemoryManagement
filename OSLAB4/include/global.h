
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif

EXTERN  int			ticks;

EXTERN  SEMAPHORE	customers;	//客户信号量
EXTERN  SEMAPHORE	barber;		//理发师信号量
EXTERN  SEMAPHORE	mutex;		//互斥信号量
EXTERN  int			waiting;	//等待的客户数
EXTERN  int			id;			//客户编号

EXTERN	int			disp_pos;
EXTERN	u8			gdt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	DESCRIPTOR	gdt[GDT_SIZE];
EXTERN	u8			idt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	GATE		idt[IDT_SIZE];

EXTERN	u32			k_reenter;

EXTERN	TSS			tss;
EXTERN	PROCESS*	p_proc_ready;

extern	PROCESS		proc_table[];	//进程表
extern	char		task_stack[];
extern  TASK		task_table[];
extern	irq_handler	irq_table[];

