
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);

/* protect.c */
PUBLIC void	init_prot();
PUBLIC u32	seg2phys(u16 seg);

/* klib.c */
PUBLIC void	delay(int time);

/* kernel.asm */
void restart();

/* main.c */
void TestA();
void TestB();
void TestC();
void TestD();
void TestE();

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);

/* 以下是系统调用相关 */

/* SEMAPHORE */
typedef struct s_sem{
	int		value;				//信号量值
	int		list[QUEUE_SIZE];	//进程队列
	int     in;					//队尾指针
	int 	out;				//队首指针
}SEMAPHORE;

/* proc.c */
PUBLIC  int     sys_get_ticks();        /* sys_call */
PUBLIC  void    sys_disp_str(char* buf);
PUBLIC 	void	sys_disp_color_str(char *buf,int color);
PUBLIC  void    sys_sleep(int milli_sec);
PUBLIC  void    sys_sem_p(SEMAPHORE* sem,int index);
PUBLIC  void    sys_sem_v(SEMAPHORE* sem);

/* syscall.asm */
PUBLIC  void    sys_call();             /* int_handler */
PUBLIC  int     get_ticks();
PUBLIC  void    print_str(char* buf);
PUBLIC 	void	print_color_str(char *buf,int color);
PUBLIC  void    sleep(int milli_sec);
PUBLIC  void    sem_p(SEMAPHORE* sem,int index);
PUBLIC  void    sem_v(SEMAPHORE* sem);


