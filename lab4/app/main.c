#include "lib.h"
#include "types.h"
void philosopher(int i, sem_t forks[])
{
	int id = getpid();
	while (1)
	{
		printf("Philosopher %d: think\n", id);
		// sem_wait(&forks[i]);
		// sem_wait(&forks[(i+1)%5]);
		if (i % 2 == 0)
		{
			sem_wait(&forks[i]);
			sleep(128);
			sem_wait(&forks[(i+1)%5]);
			sleep(128);
		}
		else
		{
			sem_wait(&forks[(i+1)%5]);
			sleep(128);
			sem_wait(&forks[i]);
			sleep(128);
		}
		printf("Philosopher %d: eat\n", id);
		sleep(128);
		sem_post(&forks[i]);
		sleep(128);
		sem_post(&forks[(i+1)%5]);
	}
}

void producer(sem_t *empty, sem_t *full, sem_t *mutex)
{
	int id=getpid();
	
	for (int i=0;i<2;i++)
	{
		sem_wait(empty);
		sleep(128);
		sem_wait(mutex);
		sleep(128);
		printf("Producer %d: produce\n", id);
		sleep(128);
		sem_post(mutex);
		sleep(128);
		sem_post(full);
	}
	return;
}

void consumer(sem_t *empty, sem_t *full, sem_t *mutex)
{
	for (int i=0;i<10;i++)
	{
		sem_wait(full);
		sleep(128);
		sem_wait(mutex);
		sleep(128);
		printf("Consumer : consume\n");
		sleep(128);
		sem_post(mutex);
		sleep(128);
		sem_post(empty);
	}
	return;
}
void reader(int *Rcount, sem_t *writeblock, sem_t *mutex)
{
	int id = getpid();
	
	if ((*Rcount) == 0)
	{
		sem_wait(writeblock);
		sleep(128);
	}
	++(*Rcount);
	sleep(128);
	sem_post(mutex);
	sleep(128);
	printf("Reader %d: read, total %d reader\n", id, (*Rcount));
	sleep(128);
	sem_wait(mutex);
	sleep(128);
	--(*Rcount);
	if ((*Rcount) == 0)
	{
		sem_post(writeblock);
		sleep(128);
	}
	sem_post(mutex);
}

void writer(sem_t *writeblock)
{
	int id = getpid();
	sem_wait(writeblock);
	sleep(128);
	printf("Writer %d: write\n", id);
	sleep(128);
	sem_post(writeblock);
}
int uEntry(void) {
	//printf("11\n");
	// For lab4.1
	// Test 'scanf' 
	/*
	int ret = 0;
	int dec = 0;
	int hex = 0;
	char str[6];
	char cha = 0;

	while(1){
		printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
		ret = scanf(" Test %c Test %6s %d %x", &cha, str, &dec, &hex);
		printf("Ret: %d; %c, %s, %d, %x.\n", ret, cha, str, dec, hex);
		if (ret == 4)
			break;
	}
	*/

	// For lab4.2
	// Test 'Semaphore'
	/*
	int i = 4;

	sem_t sem;
	printf("Father Process: Semaphore Initializing.\n");
	ret = sem_init(&sem, 2);
	if (ret == -1) {
		printf("Father Process: Semaphore Initializing Failed.\n");
		exit();
	}

	ret = fork();
	if (ret == 0) {
		while( i != 0) {
			i --;
			printf("Child Process: Semaphore Waiting.\n");
			sem_wait(&sem);
			printf("Child Process: In Critical Area.\n");
		}
		printf("Child Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}
	else if (ret != -1) {
		while( i != 0) {
			i --;
			printf("Father Process: Sleeping.\n");
			sleep(128);
			printf("Father Process: Semaphore Posting.\n");
			sem_post(&sem);
		}
		printf("Father Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}
	*/
	// For lab4.3
	// TODO: You need to design and test the philosopher problem.
	// Note that you can create your own functions.
	// Requirements are demonstrated in the guide.
    /*哲学家就餐问题*/
    
	printf("philosopher\n");
	int i = 0;
	sem_t forks[5];
	for (int i = 0; i < 5; i++)
	{
		sem_init(&forks[i], 1);
	}

	for (; i < 4; i++)
	{
		if (fork() == 0)
		{
			philosopher(i, forks);
			exit();
		}
	}
	philosopher(i, forks);
    

    /*生产者消费者问题*/
    /*
    printf("bounded_buffer\n");
	sem_t empty;
	sem_t full;
	sem_t mutex;
	sem_init(&empty, 5);
	sem_init(&full, 0);
	sem_init(&mutex, 1);
	for (int i = 0; i < 4; i++)
	{
		if (fork() == 0)
		{
			producer(&empty, &full, &mutex);
			exit();
		}
	}
	consumer(&empty, &full, &mutex);

	sem_destroy(&empty);
	sem_destroy(&full);
	sem_destroy(&mutex);
    */

    /*读者写者问题*/
	/*
    printf("reader_writer\n");
	int Rcount = 0;
	sem_t writeblock;
	sem_t mutex;
	sem_init(&writeblock, 1);
	sem_init(&mutex, 1);
	for (int i = 0; i < 3; i++)
	{
		if (fork() == 0)
		{
			writer(&writeblock);
			exit();
		}
	}
	for (int i = 0; i < 3; i++)
	{
		if (fork() == 0)
		{
			reader(&Rcount, &writeblock, &mutex);
			exit();
		}
	}

	exit();
	*/
	return 0;
}
