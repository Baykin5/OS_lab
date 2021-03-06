#include "x86.h"
#include "device.h"

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

void GProtectFaultHandle(struct StackFrame *sf);

void syscallHandle(struct StackFrame *sf);

void syscallWrite(struct StackFrame *sf);
void syscallPrint(struct StackFrame *sf);

void timerHandle(struct StackFrame *sf);
void syscallFork(struct StackFrame *sf);
void syscallSleep(struct StackFrame *sf);
void syscallExit(struct StackFrame *sf);

void timerHandle(struct StackFrame *sf) {
	uint32_t tmpStackTop;
	int pid;
	for (int i = 1; i < MAX_PCB_NUM; i++)  //遍历pcb
	{
		pid = (i + current) % MAX_PCB_NUM;
		if (pcb[pid].state == STATE_BLOCKED && pcb[pid].sleepTime > 0)
		{
			pcb[pid].sleepTime--;			//将状态为STATE_BLOCKED的进程的sleepTime减1
			if (pcb[pid].sleepTime == 0)
			{
				pcb[pid].state = STATE_RUNNABLE;		//将当前进程的timeCount加1
			}
		}
	}

	if (pcb[current].state == STATE_RUNNING && pcb[current].timeCount < MAX_TIME_COUNT)
	{
		pcb[current].timeCount++;
	}
	else
	{
		if (pcb[current].state == STATE_RUNNING)
		{
			pcb[current].state = STATE_RUNNABLE;
			pcb[current].timeCount = 0;
		}

		for (int i = 1; i < MAX_PCB_NUM; i++)
		{
			pid = (i + current) % MAX_PCB_NUM;
			if (pid != 0 && pcb[pid].state == STATE_RUNNABLE)  //找到其他状态为STATE_RUNNABLE的进程
			{
				break;
			}
		}
		if (pcb[pid].state != STATE_RUNNABLE)
		{
			pid = 0;
		}
		current = pid;
		pcb[current].state = STATE_RUNNING;
		pcb[current].timeCount = 1;
		//进程切换
		tmpStackTop = pcb[current].stackTop;  
		pcb[current].stackTop = pcb[current].prevStackTop;
		tss.esp0 = (uint32_t) & (pcb[current].stackTop);
		asm volatile("movl %0, %%esp" ::"m"(tmpStackTop)); // switch kernel stack
		asm volatile("popl %gs");
		asm volatile("popl %fs");
		asm volatile("popl %es");
		asm volatile("popl %ds");
		asm volatile("popal");
		asm volatile("addl $8, %esp");
		asm volatile("iret");
	
	}

	return;
}

void syscallFork(struct StackFrame *sf) {
    
	int i, j;
	for (i = 0; i < MAX_PCB_NUM; i++)  //遍历pcb数组查找是否有未分配的控制块
    {
		if (pcb[i].state == STATE_DEAD)
			break;
	}
	if (i != MAX_PCB_NUM) 
    {
		enableInterrupt();  
		for (j = 0; j < 0x100000; j++)  //将父进程内存空间中的所有内容复制到子进程内存空间中
        {
			*(uint8_t *)(j + (i+1)*0x100000) = *(uint8_t *)(j + (current+1)*0x100000);
		}
		disableInterrupt();
		pcb[i].pid = i;
		pcb[i].timeCount = 0;
		pcb[i].sleepTime = pcb[current].sleepTime;
		pcb[i].prevStackTop = (uint32_t)&(pcb[i].stackTop);
		pcb[i].stackTop = (uint32_t)&(pcb[i].regs);
		pcb[i].state = STATE_RUNNABLE;
		/* 设置寄存器 */
		pcb[i].regs.cs = USEL(2*i + 1);
		pcb[i].regs.ds = USEL(2*i + 2);
		pcb[i].regs.es = USEL(2*i + 2);
		pcb[i].regs.fs = USEL(2*i + 2);
		pcb[i].regs.ss = USEL(2*i + 2);
		pcb[i].regs.eflags = pcb[current].regs.eflags;
		pcb[i].regs.edx = pcb[current].regs.edx;
		pcb[i].regs.ecx = pcb[current].regs.ecx;
		pcb[i].regs.ebx = pcb[current].regs.ebx;
		pcb[i].regs.esp = pcb[current].regs.esp;
		pcb[i].regs.ebp = pcb[current].regs.ebp;
		pcb[i].regs.edi = pcb[current].regs.edi;
		pcb[i].regs.esi = pcb[current].regs.esi;
		pcb[i].regs.eip = pcb[current].regs.eip;
		/* 设置返回值 */
		pcb[i].regs.eax = 0;
		pcb[current].regs.eax = i;
	}
	else {
		pcb[current].regs.eax = -1;
	}
	return;
}


void syscallSleep(struct StackFrame *sf) {
	pcb[current].sleepTime = sf->ecx;
	pcb[current].state = STATE_BLOCKED;
	asm volatile("int $0x20");
}

void syscallExit(struct StackFrame *sf) {
	pcb[current].state = STATE_DEAD;
	asm volatile("int $0x20");
	return;
}


void irqHandle(struct StackFrame *sf) { // pointer sf = esp
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));
	/*TODO Save esp to stackTop */
	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)sf;

	switch(sf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(sf);
			break;
		case 0x20:
			timerHandle(sf);
			break;
		case 0x80:
			syscallHandle(sf);
			break;
		default:assert(0);
	}
	/*TODO Recover stackTop */
	pcb[current].stackTop = tmpStackTop;
}

void GProtectFaultHandle(struct StackFrame *sf) {
	assert(0);
	return;
}

void syscallHandle(struct StackFrame *sf) {
	switch(sf->eax) { // syscall number
		case 0:
			syscallWrite(sf);
			break; // for SYS_WRITE
		/*TODO Add Fork,Sleep... */
		case 1:
			syscallFork(sf);
			break; // for SYS_FORK
		case 3:
			syscallSleep(sf);
			break; // for SYS_SLEEP
		case 4:
			syscallExit(sf);
			break; // for SYS_EXIT
		default:break;
	}
}

void syscallWrite(struct StackFrame *sf) {
	switch(sf->ecx) { // file descriptor
		case 0:
			syscallPrint(sf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct StackFrame *sf) {
	int sel = sf->ds; //TODO segment selector for user data, need further modification
	char *str = (char*)sf->edx;
	int size = sf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str+i));
		if(character == '\n') {
			displayRow++;
			displayCol=0;
			if(displayRow==25){
				displayRow=24;
				displayCol=0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80*displayRow+displayCol)*2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
			displayCol++;
			if(displayCol==80){
				displayRow++;
				displayCol=0;
				if(displayRow==25){
					displayRow=24;
					displayCol=0;
					scrollScreen();
				}
			}
		}
		//asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
		//asm volatile("int $0x20":::"memory"); //XXX Testing irqTimer during syscall
	}
	
	updateCursor(displayRow, displayCol);
	//TODO take care of return value
	return;
}

