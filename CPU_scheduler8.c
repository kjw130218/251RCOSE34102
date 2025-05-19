#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#define MAX_TIME_UNIT 200
#define MAX_PROCESS_NUM 10


#define TRUE 1
#define FALSE 0


//process
typedef struct myProcess* processPointer;
typedef struct myProcess {
    int pid;
    int priority;
    int arrivalTime;
    int CPUburst;
    int IOburst;
    int IOrequest;
    int IOrequestremain;
    int CPUremainingTime;
    int IOremainingTime;
    int IOwait;
    int waitingTime;
    int turnaroundTime;

}myProcess;

typedef struct gantt* ganttPointer;
typedef struct gantt {
    int pid;
    int time;
}gantt;
int tempprevpid = 0;
int temppid;

ganttPointer ganttchart[200];
int ganttnum = 0;

int Computation_start = 0;
int Computation_end = 0;
int Computation_idle = 0;

typedef struct evaluation* evalPointer;
typedef struct evaluation {
	int alg;
	int preemptive;
	int startTime;
	int endTime;
	int avg_waitingTime;
	int avg_turnaroundTime;
	double CPU_util;
	int completed;
}evaluation;

evalPointer evals[30];
int cur_eval_num = 0;

void init_evals(){
	cur_eval_num = 0;
	int i;
	for(i=0;i<30;i++)
		evals[i]=NULL;
}

void clear_evals() {
	
	int i;
	for(i=0;i<30;i++){
		free(evals[i]);
		evals[i]=NULL;
	}
	cur_eval_num = 0;
}

void clear_ganttchart() {
	int i;
	for(i=0;i<200;i++){
		free(ganttchart[i]);
		ganttchart[i]=NULL;
	}
}

//Job Queue
processPointer jobQueue[MAX_PROCESS_NUM];
int cur_proc_num_JQ = 0;

void init_JQ () {
	cur_proc_num_JQ = 0;
    int i;
    for (i = 0; i < MAX_PROCESS_NUM; i++)
        jobQueue[i] = NULL;
}

void sort_JQ() { //JobQueue pid 순으로 정렬
    int i, j;
    processPointer remember;
    for ( i = 1; i < cur_proc_num_JQ; i++ )
    {
      remember = jobQueue[(j=i)];
      while ( --j >= 0 && remember->pid < jobQueue[j]->pid )
          jobQueue[j+1] = jobQueue[j];
      jobQueue[j+1] = remember; 
    }
}

int getProcByPid_JQ (int givenPid) { //JobQueue에서 해당 pid를 가지고 있는 process의 index를 리턴한다.
    int result = -1;
    int i;
    for(i = 0; i < cur_proc_num_JQ; i++) {
        int temp = jobQueue[i]->pid;
        if(temp == givenPid)
            return i;
    }
    return result;
}

void insertInto_JQ (processPointer proc) {
    if(cur_proc_num_JQ<MAX_PROCESS_NUM) {
        int temp = getProcByPid_JQ(proc->pid);
        if (temp != -1) {
            printf("<ERROR> The process with pid: %d already exists in Job Queue\n", proc->pid);
            return;  
        }
        jobQueue[cur_proc_num_JQ++] = proc;
    }
    else {
        puts("<ERROR> Job Queue is full");
        return;
    }
}

processPointer removeFrom_JQ (processPointer proc) { //process 하나를 JobQueue에서 제거하고 빈 공간을 수축을 통해 없앤다.
    if(cur_proc_num_JQ>0) {
        int temp = getProcByPid_JQ(proc->pid);
        if (temp == -1) {
            printf("<ERROR> Cannot find the process with pid: %d\n", proc->pid);
            return NULL;    
        } else {
            processPointer removed = jobQueue[temp];
            
            int i;
            for(i = temp; i < cur_proc_num_JQ - 1; i++)
                jobQueue[i] = jobQueue[i+1];   
            jobQueue[cur_proc_num_JQ - 1] = NULL;
            
            cur_proc_num_JQ--;
            return removed;
        }
        
    } else {
        puts("<ERROR> Job Queue is empty");
        return NULL;
    }
}

void clear_JQ() { //메모리 회수용 함수
    int i;
    for(i = 0; i < MAX_PROCESS_NUM; i++) {
        free(jobQueue[i]);
        jobQueue[i] = NULL;
    }
    cur_proc_num_JQ = 0;
}

void print_JQ() { //debug를 위한 print 함수
    //puts("\nprint_JQ()");
	printf("Number of the processes: %d\n", cur_proc_num_JQ);
	int i;
	puts("pid    priority    arrival_time    CPU burst    IO burst     max IO request");
	puts("===========================================================================");
    for(i = 0; i < cur_proc_num_JQ; i++) {
        printf("%3d    %8d    %12d    %9d    %8d    %14d\n", jobQueue[i]->pid, jobQueue[i]->priority, jobQueue[i]->arrivalTime, jobQueue[i]->CPUburst, jobQueue[i]->IOburst, jobQueue[i]->IOrequest);
    }
    puts("===========================================================================\n");
}

processPointer cloneJobQueue[MAX_PROCESS_NUM];
int cur_proc_num_clone_JQ = 0;

void clone_JQ() {
	// 여러 시뮬레이션을 처리하기 위해 clone을 만들어준다. 
	
	int i;
	for (i=0; i< MAX_PROCESS_NUM; i++) { //init clone
		cloneJobQueue[i] = NULL;
	}
	
	for (i=0; i<cur_proc_num_JQ; i++) {
		
		processPointer newProcess = (processPointer)malloc(sizeof(struct myProcess));
        newProcess->pid = jobQueue[i]->pid;
        newProcess->priority = jobQueue[i]->priority;
        newProcess->arrivalTime = jobQueue[i]->arrivalTime;
        newProcess->CPUburst = jobQueue[i]->CPUburst;
        newProcess->IOburst = jobQueue[i]->IOburst;
        newProcess->IOrequest = jobQueue[i]->IOrequest;
        newProcess->IOrequestremain = jobQueue[i]->IOrequestremain;
        newProcess->CPUremainingTime = jobQueue[i]->CPUremainingTime;
        newProcess->IOremainingTime = jobQueue[i]->IOremainingTime;
        newProcess->IOwait = jobQueue[i]->IOwait;
        newProcess->waitingTime = jobQueue[i]->waitingTime;
        newProcess->turnaroundTime = jobQueue[i]->turnaroundTime;
        
        cloneJobQueue[i] = newProcess;
	}
	
	cur_proc_num_clone_JQ = cur_proc_num_JQ;
}

void loadClone_JQ() {
	// 클론으로부터 JQ에 복사한다. 
	clear_JQ(); //clear JQ
	int i;
	for (i=0; i<cur_proc_num_clone_JQ; i++) {
	
		processPointer newProcess = (processPointer)malloc(sizeof(struct myProcess));
	    newProcess->pid = cloneJobQueue[i]->pid;
	    newProcess->priority = cloneJobQueue[i]->priority;
	    newProcess->arrivalTime = cloneJobQueue[i]->arrivalTime;
	    newProcess->CPUburst = cloneJobQueue[i]->CPUburst;
	    newProcess->IOburst = cloneJobQueue[i]->IOburst;
        newProcess->IOrequest = cloneJobQueue[i]->IOrequest;
        newProcess->IOrequestremain = cloneJobQueue[i]->IOrequestremain;
	    newProcess->CPUremainingTime = cloneJobQueue[i]->CPUremainingTime;
	    newProcess->IOremainingTime = cloneJobQueue[i]->IOremainingTime;
        newProcess->IOwait = cloneJobQueue[i]->IOwait;
	    newProcess->waitingTime = cloneJobQueue[i]->waitingTime;
	    newProcess->turnaroundTime = cloneJobQueue[i]->turnaroundTime;
	    
	    jobQueue[i] = newProcess;
	}
	
	cur_proc_num_JQ = cur_proc_num_clone_JQ;
	//print_JQ();
}

void clearClone_JQ() { //메모리 회수용 함수
    int i;
    for(i = 0; i < MAX_PROCESS_NUM; i++) {
        free(cloneJobQueue[i]);
        cloneJobQueue[i] = NULL;
    }
}

//running state 현재 running 중인 process
processPointer runningProcess = NULL;
int timeConsumed = 0;

//readyQueue
//arrivalTime이 순서대로 정렬된 채로 process가 create된다고 가정
processPointer readyQueue[MAX_PROCESS_NUM];
int cur_proc_num_RQ = 0; // 현재 process의 수

void init_RQ () {
    cur_proc_num_RQ = 0;
	int i;
    for (i = 0; i < MAX_PROCESS_NUM; i++)
        readyQueue[i] = NULL;
}

int getProcByPid_RQ (int givenPid) { //readyQueue에서 해당 pid를 가지고 있는 process의 index를 리턴한다.
    int result = -1;
    int i;
    for(i = 0; i < cur_proc_num_RQ; i++) {
        int temp = readyQueue[i]->pid;
        if(temp == givenPid)
            return i;
    }
    return result;
}

void insertInto_RQ (processPointer proc) {
    if(cur_proc_num_RQ<MAX_PROCESS_NUM) {
        int temp = getProcByPid_RQ(proc->pid);
        if (temp != -1) {
            printf("<ERROR> The process with pid: %d already exists in Ready Queue\n", proc->pid);
            return;  
        }
        readyQueue[cur_proc_num_RQ++] = proc;
    }
    else {
        puts("<ERROR> Ready Queue is full");
        return;
    }
}

processPointer removeFrom_RQ (processPointer proc) { //process 하나를 readyQueue에서 제거하고 빈 공간을 수축을 통해 없앤다.
    if(cur_proc_num_RQ>0) {
        int temp = getProcByPid_RQ(proc->pid);
        if (temp == -1) {
            printf("<ERROR> Cannot find the process with pid: %d\n", proc->pid);
            return NULL;    
        } else {
            processPointer removed = readyQueue[temp];
            
            int i;
            for(i = temp; i < cur_proc_num_RQ - 1; i++)
                readyQueue[i] = readyQueue[i+1];   
            readyQueue[cur_proc_num_RQ - 1] = NULL;
            
            cur_proc_num_RQ--;
            return removed;
        }
        
    } else {
        puts("<ERROR> Ready Queue is empty");
        return NULL;
    }
}

void clear_RQ() { //메모리 회수용 함수
    int i;
    for(i = 0; i < MAX_PROCESS_NUM; i++) {
        free(readyQueue[i]);
        readyQueue[i]=NULL;
    }
    cur_proc_num_RQ = 0;
}

void print_RQ() { //debug를 위한 print 함수
    puts("\nprintf_RQ()");
	int i;
    for(i = 0; i < cur_proc_num_RQ; i++) {
        printf("%d ", readyQueue[i]->pid);   
    }
    printf("\n총 프로세스 수: %d\n", cur_proc_num_RQ);
}

//waitingQueue
processPointer waitingQueue[MAX_PROCESS_NUM];
int cur_proc_num_WQ = 0; 

void init_WQ () {
	cur_proc_num_WQ = 0;
    int i;
    for (i = 0; i < MAX_PROCESS_NUM; i++)
        waitingQueue[i] = NULL;
}

int getProcByPid_WQ (int givenPid) { //waitingQueue에서 해당 pid를 가지고 있는 process의 index를 리턴한다.
    int result = -1;
    int i;
    for(i = 0; i < cur_proc_num_WQ; i++) {
        int temp = waitingQueue[i]->pid;
        if(temp == givenPid)
            return i;
    }
    return result;
}

void insertInto_WQ (processPointer proc) {
    if(cur_proc_num_WQ<MAX_PROCESS_NUM) {
        int temp = getProcByPid_WQ(proc->pid);
        if (temp != -1) {
            printf("<ERROR> The process with pid: %d already exists in Waiting Queue\n", proc->pid);
            return;  
        }
        waitingQueue[cur_proc_num_WQ++] = proc;
    }
    else {
        puts("<ERROR> Waiting Queue is full");
        return;
    }
    //print_WQ();
}

processPointer removeFrom_WQ (processPointer proc) { //process 하나를 waitingQueue에서 제거하고 빈 공간을 수축을 통해 없앤다.
    if(cur_proc_num_WQ>0) {
        int temp = getProcByPid_WQ(proc->pid);
        if (temp == -1) {
            printf("<ERROR> Cannot find the process with pid: %d\n", proc->pid);
            return NULL;    
        } else {
        	
            processPointer removed = waitingQueue[temp];
            int i;
            for(i = temp; i < cur_proc_num_WQ - 1; i++)
                waitingQueue[i] = waitingQueue[i+1];
				   
            waitingQueue[cur_proc_num_WQ - 1] = NULL;
            
            cur_proc_num_WQ--;
            
            return removed;
        }
        
    } else {
        puts("<ERROR> Waiting Queue is empty");
        return NULL;
    }
}

void clear_WQ() { //메모리 회수용 함수
    int i;
    for(i = 0; i < MAX_PROCESS_NUM; i++) {
        free(waitingQueue[i]);
        waitingQueue[i] = NULL;
    }
    cur_proc_num_WQ = 0;
}

void print_WQ() { //debug를 위한 print 함수
    puts("\nprintf_WQ()");
	int i;
    
    for(i = 0; i < cur_proc_num_WQ; i++) {
        printf("%d ", waitingQueue[i]->pid);   
    }
    printf("\n총 프로세스 수: %d\n", cur_proc_num_WQ);
}

//terminatedQueue
processPointer terminated[MAX_PROCESS_NUM];
int cur_proc_num_T = 0; 

void init_T () {
	cur_proc_num_T = 0;
    int i;
    for (i = 0; i < MAX_PROCESS_NUM; i++)
        terminated[i] = NULL;
}

void clear_T() { //메모리 회수용 함수
    int i;
    for(i = 0; i < MAX_PROCESS_NUM; i++) {
        free(terminated[i]);
        terminated[i] = NULL;
    }
    cur_proc_num_T = 0;
}

void insertInto_T (processPointer proc) {
    if(cur_proc_num_T<MAX_PROCESS_NUM) {
        terminated[cur_proc_num_T++] = proc;
    }
    else {
        puts("<ERROR> Cannot terminate the process");
        return;
    }
}

void print_T() { //debug를 위한 print 함수
    puts("\nprintf_T()");
	
	int i;
    for(i = 0; i < cur_proc_num_T; i++) {
        printf("%d ", terminated[i]->pid);   
    }
    printf("\ntotal process : %d\n", cur_proc_num_T);
}

void Readygantt() {
	ganttnum = 0;
    int i = 0;
    for (i;i<200;i++){
        ganttchart[i] = (ganttPointer)malloc(sizeof(gantt));
        if (ganttchart[i] == NULL) {
            fprintf(stderr, "메모리 할당 실패\n");
            exit(-1);
        }
    }
    ganttchart[0]->pid = -1;   
    ganttchart[0]->time = 0;
}


processPointer createProcess(int pid, int priority, int arrivalTime, int CPUburst, int IOburst, int IORequest, int IOwait) { //프로세스 하나를 만든다.
    //랜덤으로 생성해서 여러 알고리즘 테스트하는 건 clone을 사용하자

    if (arrivalTime > MAX_TIME_UNIT || arrivalTime < 0) {
        printf("<ERROR> arrivalTime should be in [0..MAX_TIME_UNIT]\n");
        printf("<USAGE> createProcess(int pid, int priority, int arrivalTime, int CPUburst, int IOburst)\n");
        return NULL;
    }
    
    if (CPUburst <= 0 || IOburst < 0) {
        printf("<ERROR> CPUburst and should be larger than 0 and IOburst cannot be a negative number.\n");
        printf("<USAGE> createProcess(int pid, int priority, int arrivalTime, int CPUburst, int IOburst)\n");
        return NULL;
    }
    
        processPointer newProcess = (processPointer)malloc(sizeof(struct myProcess));
        newProcess->pid = pid;
        newProcess->priority = priority;
        newProcess->arrivalTime = arrivalTime;
        newProcess->CPUburst = CPUburst;
        newProcess->IOburst = IOburst;
        newProcess->IOrequest = IORequest;
        newProcess->IOrequestremain = IORequest;
        newProcess->CPUremainingTime = CPUburst;
        newProcess->IOremainingTime = IOburst;
        newProcess->IOwait = IOwait;
        newProcess->waitingTime = 0;
        newProcess->turnaroundTime = 0;
        
        //job queue에 넣는다.
        insertInto_JQ(newProcess);

    //debug
    //printf("%d %d %d %d created\n",newProcess.pid ,newProcess.priority, newProcess.arrivalTime, newProcess.CPUburst);
    return newProcess;
}

processPointer FCFS_alg() {
        
        processPointer earliestProc = readyQueue[0]; //가장 먼저 도착한 process를 찾는다.
        
        if (earliestProc != NULL){
            
            if(runningProcess != NULL) { //이미 수행중인 프로세스가 있었다면 preemptive가 아니므로 기다린다.  
                /*
				if(runningProcess->arrivalTime > earliestProc->arrivalTime)
					puts("<ERROR> Invalid access."); //오류메세지를 출력한다. 
	            */	
				return runningProcess;
        	} else {
				return removeFrom_RQ(earliestProc);
			}
        
        } else { //readyQueue에 아무것도 없는 경우
            return runningProcess;
        }
}

processPointer SJF_alg(int preemptive) {
	
	processPointer shortestJob = readyQueue[0];
	
	if(shortestJob != NULL) {
		int i;
        for(i = 0; i < cur_proc_num_RQ; i++) {
            if (readyQueue[i]->CPUremainingTime <= shortestJob->CPUremainingTime) { 
                
                if(readyQueue[i]->CPUremainingTime == shortestJob->CPUremainingTime) { //남은 시간이 같을 경우먼저 도착한 process가 먼저 수행된다.
                    if (readyQueue[i]->arrivalTime < shortestJob->arrivalTime) shortestJob = readyQueue[i];
                } else {
                    shortestJob = readyQueue[i];
                }
            }
        }
		
		if(runningProcess != NULL) { //이미 수행중인 프로세스가 있을 때 
				if(preemptive){ //preemptive면 
				
					if(runningProcess->CPUremainingTime >= shortestJob->CPUremainingTime) {
						if(runningProcess->CPUremainingTime == shortestJob->CPUremainingTime) { //남은 시간이 같을 경우먼저 도착한 process가 먼저 수행된다.
		                    if (runningProcess->arrivalTime < shortestJob->arrivalTime){
								return runningProcess;
							} else if(runningProcess->arrivalTime == shortestJob->arrivalTime)
								return runningProcess; //arrivalTime까지 같으면 굳이 Context switch overhead를 감수하면서까지 preempt하지 않는다. 
						}
						puts("preemption is detected.");
						insertInto_RQ(runningProcess);
						return removeFrom_RQ(shortestJob);
					}
				
					return runningProcess;
				}				
	            //non-preemptive면 기다린다. 
				return runningProcess;
        	} else {
				return removeFrom_RQ(shortestJob);
			}
		
	}else {
		return runningProcess;
	}
}

processPointer PRIORITY_alg(int preemptive) {
	
	processPointer importantJob = readyQueue[0];
	
	if(importantJob != NULL) {
		int i;
        for(i = 0; i < cur_proc_num_RQ; i++) {
            if (readyQueue[i]->priority <= importantJob->priority) { 
                
                if(readyQueue[i]->priority == importantJob->priority) { //priority가 같을 경우먼저 도착한 process가 먼저 수행된다.
                    if (readyQueue[i]->arrivalTime < importantJob->arrivalTime)
						importantJob = readyQueue[i];
                } else {
                    importantJob = readyQueue[i];
                }
            }
        }
		
		if(runningProcess != NULL) { //이미 수행중인 프로세스가 있을 때 
				if(preemptive){ //preemptive면 
				
					if(runningProcess->priority >= importantJob->priority) {
						if(runningProcess->priority == importantJob->priority) { //priority가 같을 경우먼저 도착한 process가 먼저 수행된다.
		                    if (runningProcess->arrivalTime < importantJob->arrivalTime){
								return runningProcess;
							} else if(runningProcess->arrivalTime == importantJob->arrivalTime) {
								return runningProcess; //arrivalTime까지 같다면 굳이 preempt안한다 (context - switch overhead 줄이기 위해) 
								
							}
						}
						puts("preemption is detected.");
						insertInto_RQ(runningProcess);
						return removeFrom_RQ(importantJob);
					}
				
					return runningProcess;
				}				
	            //non-preemptive면 기다린다. 
				return runningProcess;
        	} else {
				return removeFrom_RQ(importantJob);
			}
		
	}else {
		return runningProcess;
	}
}

processPointer RR_alg(int time_quantum){
	
	processPointer earliestProc = readyQueue[0]; //가장 먼저 도착한 process를 찾는다.
        
        if (earliestProc != NULL){
            
            if(runningProcess != NULL) { //이미 수행중인 프로세스가 있었다면
				//return runningProcess;
				
				if(timeConsumed >= time_quantum){ //이미 수행중이 었던 프로세스가 Time expired되었다면 		
					insertInto_RQ(runningProcess);
					return removeFrom_RQ(earliestProc);
				} else {
					return runningProcess;
				}
				
        	} else {
				return removeFrom_RQ(earliestProc);
			}
        
        } else { //readyQueue에 아무것도 없는 경우
            return runningProcess; 
        }
}

processPointer schedule(int alg, int preemptive, int time_quantum) { //timelimit 시간동안 scheduling 알고리즘을 진행한다.
	processPointer selectedProcess = NULL;
    
    switch(alg) {
        case 1:
            selectedProcess = FCFS_alg();
            break;
        case 2:
        	selectedProcess = SJF_alg(preemptive);
        	break;
        case 3:
        	selectedProcess = PRIORITY_alg(preemptive);
        	break;
        case 4:
        	selectedProcess = RR_alg(time_quantum);
        	break;
        default:
        return NULL;
    }
    
    return selectedProcess;
}

void simulate(int amount, int alg, int preemptive, int time_quantum) { //amount 시점이 흐른 뒤의 상태 -> 반복문에 넣어서 사용 
	//우선, Job queue에서 해당 시간에 도착한 프로세스들을 ready queue에 올려준다. 
	processPointer tempProcess = NULL;
    int jobNum = cur_proc_num_JQ;
	int i;
	for(i = 0; i < cur_proc_num_JQ; i++) {
		if(jobQueue[i]->arrivalTime == amount) {
			tempProcess = removeFrom_JQ(jobQueue[i--]);
			insertInto_RQ(tempProcess);
		}
	}
	processPointer prevProcess = runningProcess;
	runningProcess = schedule(alg, preemptive, time_quantum); //이번 turn에 수행될 process를 pick up한다. 
    
    if (runningProcess == NULL) {
        temppid = 0;
    } else {
        temppid = runningProcess->pid;
    }


	printf("%d: ",amount);
    if (tempprevpid != temppid) {
        if (ganttchart[ganttnum] == NULL) {
            ganttchart[ganttnum] = (ganttPointer)malloc(sizeof(gantt));
            if (ganttchart[ganttnum] == NULL) {
                fprintf(stderr, "메모리 할당 실패\n");
                exit(1);
            }
        }

        if (runningProcess == NULL) {
            ganttchart[ganttnum]->pid = 0;
        } else {
            ganttchart[ganttnum]->pid = temppid;
        }

        ganttchart[ganttnum]->time = amount;
        ganttnum++;  
        timeConsumed = 0;
    }
	
    for(i = 0; i < cur_proc_num_RQ; i++) { //readyQueue에 있는 process들을 기다리게 한다. 
        
        if(readyQueue[i]) {
        	readyQueue[i]->waitingTime++;
        	readyQueue[i]->turnaroundTime++;
    	}
    }
	
    for(i = 0; i < cur_proc_num_WQ; i++) { //waitingQueue에 있는 process들이 IO 작업을 수행한다. 
		if(waitingQueue[i]) {
			//waitingQueue[i]->waitingTime++;
			waitingQueue[i]->turnaroundTime++;
			waitingQueue[i]->IOremainingTime--;
			
			if(waitingQueue[i]->IOremainingTime <= 0 ) { //IO 작업이 완료된 경우 
				printf("(pid: %d) -> IO complete, ", waitingQueue[i]->pid); 
                waitingQueue[i]->IOrequestremain--;
                if (waitingQueue[i]->IOrequestremain > 0) waitingQueue[i]->IOremainingTime = waitingQueue[i]->IOburst;
				insertInto_RQ(removeFrom_WQ(waitingQueue[i--])); //ready queue로 프로세스를 다시 돌려보내준다. 
				//print_WQ();
			}
		}
	}

    

    if (runningProcess != NULL) { //running 중인 프로세스가 있다면 실행시킴
        tempprevpid = runningProcess->pid;
        runningProcess->CPUremainingTime--;
        runningProcess->turnaroundTime++;
        runningProcess->IOwait--;
        timeConsumed++;
        printf("(pid: %d) -> running ", runningProcess->pid);

        if (runningProcess->CPUremainingTime <= 0) { //모두 수행이 된 상태라면, terminated로 보내준다. 
            insertInto_T(runningProcess);
            runningProcess = NULL;
            printf("-> terminated");
        }
        else { //아직 수행할 시간이 남아있을 경우 
            if (runningProcess->IOremainingTime > 0) { //IO 작업을 수행해야 한다면, waiting queue로 보내준다. 
                if (runningProcess->IOwait <= 0) {
                    runningProcess->IOwait = rand() % 5 + 1;
                    insertInto_WQ(runningProcess);
                    runningProcess = NULL;
                    printf("-> IO request");
                }
            }
        }
                
        printf("\n");
    } else { //running 중인 프로세스가 없다면 idle을 출력함 
        tempprevpid = 0;
    	printf("idle\n");
    	Computation_idle++;
	}
}
void analyize(int alg, int preemptive) {
	
	int wait_sum = 0;
	int turnaround_sum = 0;
	int i;
	processPointer p=NULL;

    puts("<Gantt Chart>");
    if(ganttchart[0]->time!=0){
        printf("(0)--[idle]--");
    }
    for (int j=0;j<=ganttnum-1;j++){
        if (ganttchart[j]->pid == 0) {
            printf("(%d)--[idle]--",ganttchart[j]->time);
        } else {
            printf("(%d)--[P%d]--",ganttchart[j]->time,ganttchart[j]->pid);
        }
    }
    printf("(%d)\n",Computation_end);
    clear_ganttchart();
    Readygantt();

	puts  ("===========================================================");
	for(i=0;i<cur_proc_num_T;i++){
		p = terminated[i];
		printf("(pid: %d)\n",p->pid);
		printf("waiting time = %d, ",p->waitingTime);
		printf("turnaround time = %d\n",p->turnaroundTime);
		//printf("CPU remaining time = %d\n",p->CPUremainingTime);
		//printf("IO remaining time = %d\n",p->IOremainingTime);
		
		puts  ("===========================================================");
		wait_sum += p->waitingTime;
		turnaround_sum += p->turnaroundTime;
    }
	printf("start time: %d / end time: %d / CPU utilization : %.2lf%% \n",Computation_start, Computation_end,
	 (double)(Computation_end - 1 - Computation_idle)/(Computation_end - 1 - Computation_start)*100);
	
	if(cur_proc_num_T != 0) {
		printf("Average waiting time: %d\n",wait_sum/cur_proc_num_T);
		printf("Average turnaround time: %d\n",turnaround_sum/cur_proc_num_T);
	}	
		printf("Completed: %d\n",cur_proc_num_T);
		
	if(cur_proc_num_T != 0) {
		evalPointer newEval = (evalPointer)malloc(sizeof(struct evaluation));
		newEval->alg = alg;
		newEval->preemptive = preemptive;
		
		newEval->startTime = Computation_start;
		newEval->endTime = Computation_end;
		newEval->avg_waitingTime = wait_sum/cur_proc_num_T;
		newEval->avg_turnaroundTime = turnaround_sum/cur_proc_num_T;
		newEval->CPU_util = (double)(Computation_end - 1 - Computation_idle)/(Computation_end - 1 - Computation_start)*100;
		newEval->completed = cur_proc_num_T;
		evals[cur_eval_num++] = newEval;
	}
	puts  ("===========================================================");

}
void startSimulation(int alg, int preemptive, int time_quantum, int count, int slow) {
	loadClone_JQ();
	
	switch(alg) {
        case 1:
            puts("<FCFS Algorithm>");
            break;
        case 2:
        	if(preemptive) printf("<Preemptive ");
        	else printf("<Non-preemptive ");
        	puts("SJF Algorithm>");
        	break;
        case 3:
        	if(preemptive) printf("<Preemptive ");
        	else printf("<Non-preemptive ");
        	puts("Priority Algorithm>");
        	break;
        case 4:
            printf("<Round Robin Algorithm (time quantum: %d)>\n",time_quantum);
            break;
        default:
            printf("<ERROR> Choose algorithm 1~4\n");
            exit(-1);
        return;
    }
	
	int initial_proc_num = cur_proc_num_JQ; //실제 시뮬레이션을 하기 전 프로세스의 수를 저장해둔다. 
	
	int i;
	if(cur_proc_num_JQ <= 0) {
		puts("<ERROR> Simulation failed. Process doesn't exist in the job queue");
		return;
	}
	
	int minArriv = jobQueue[0]->arrivalTime;
	for(i=0;i<cur_proc_num_JQ;i++) {
		if(minArriv > jobQueue[i]->arrivalTime)
			minArriv = jobQueue[i]->arrivalTime;		
	}
	Computation_start = minArriv;
	Computation_idle = 0;
	for(i=0;i<count;i++) {
		simulate(i,alg, preemptive, time_quantum);
        if (slow == 1) sleep(1);
        if(cur_proc_num_T == initial_proc_num) {
			i++;
			break;
		}
	}
	Computation_end = i;
	
	analyize(alg, preemptive);
	clear_JQ();
    clear_RQ();
    clear_T();
    clear_WQ();
    free(runningProcess);
    runningProcess = NULL;
    timeConsumed = 0;
    Computation_start = 0;
	Computation_end = 0;
	Computation_idle = 0;
}

void evaluate() {
	
	puts ("\n                       <Evaluation>                    \n");
	int i;
	for(i=0;i<cur_eval_num;i++) {
		
		puts ("===========================================================");
		
		int alg = evals[i]->alg;
		int preemptive = evals[i]->preemptive;
		
		switch (evals[i]->alg) {
		
		case 1:
            puts("<FCFS Algorithm>");
            break;
        case 2:
        	if(preemptive) printf("<Preemptive ");
        	else printf("<Non-preemptive ");
        	puts("SJF Algorithm>");
        	break;
        case 3:
        	if(preemptive) printf("<Preemptive ");
        	else printf("<Non-preemptive ");
        	puts("Priority Algorithm>");
        	break;
        case 4:
        	puts("<Round Robin Algorithm>");
        	break;
        
        default:
        return;
		}
		puts ("-----------------------------------------------------------");
		printf("start time: %d / end time: %d / CPU utilization : %.2lf%% \n",evals[i]->startTime,evals[i]->endTime,evals[i]->CPU_util);
		printf("Average waiting time: %d\n",evals[i]->avg_waitingTime);
		printf("Average turnaround time: %d\n",evals[i]->avg_turnaroundTime);
		printf("Completed: %d\n",evals[i]->completed);
	}
	
	puts  ("===========================================================");
}


void createRandomProcesses(int total_num, int io_num) {
    if (io_num > total_num) {
        puts("<ERROR> The number of IO event cannot be higher than the number of processes");
        exit(-1);
    }

    int i;
    for (i = 0; i < total_num; i++) {
        //CPU burst : 5~20
        //IO burst : 1~10
        createProcess(i + 1, rand() % total_num + 1, rand() % (total_num + 10), rand() % 16 + 5, 0, 0, rand() % 5 + 1);
    
    }
	for(i=0;i<io_num;i++) {
		
		int randomIndex = rand() % total_num ;
		if(jobQueue[randomIndex]->IOburst ==0) {
		
			int randomIOburst = rand() % 10 + 1;
            int randomIOrequest = rand() % 5 + 1;
			jobQueue[randomIndex]->IOburst = randomIOburst;
			jobQueue[randomIndex]->IOremainingTime = randomIOburst;
            jobQueue[randomIndex]->IOrequest = randomIOrequest;
            jobQueue[randomIndex]->IOrequestremain = randomIOrequest;


		} else {
			i--;
		}
	}
	sort_JQ();
	clone_JQ(); //backup this JQ
}

void Config() {
    init_RQ();
    init_JQ();
    init_T();
    init_WQ();
    init_evals();
    Readygantt();
}

void createProcesses() {
    int totalProcessNum;
    int totalIOProcessNum;
    int randomproc;

    printf("Choose the option\n 1. Random processes / 2. Manually set the processes\n");
    scanf("%d", &randomproc);

    switch (randomproc) {
    case 1:
        printf("How many processes do you want to simulate? (3~10)\n");
        scanf("%d", &totalProcessNum);
        if (totalProcessNum < 3 || totalProcessNum > 10) {
            puts("<ERROR> The number of process should be in [3,10]");
            exit(-1);
        }

        printf("How many I/O processes do you want to simulate? (1~%d)\n", totalProcessNum);
        scanf("%d", &totalIOProcessNum);
        if (totalIOProcessNum < 0 || totalIOProcessNum > totalProcessNum) {
            puts("<ERROR> Invalid number of I/O processes");
            exit(-1);
        }

        createRandomProcesses(totalProcessNum, totalIOProcessNum);
        break;

    case 2:
        printf("How many processes do you want to simulate? (3~10)\n");
        scanf("%d", &totalProcessNum);
        if (totalProcessNum < 3 || totalProcessNum > 10) {
            puts("<ERROR> The number of process should be in [3,10]");
            exit(-1);
        }

        int i;
        int tpriority, tarrivaltime, tCPUburst, tIOburst, tIOrequest;

        for (i = 0; i < totalProcessNum; i++) {
            printf("Process %d priority (1 ~ %d): ", i + 1, totalProcessNum);
            scanf("%d", &tpriority);
            if (tpriority < 1 || tpriority > totalProcessNum) {
                printf("<ERROR> Priority should be in [1,%d]\n", totalProcessNum);
                exit(-1);
            }

            printf("Process %d arrival time (0 ~ %d): ", i + 1, totalProcessNum + 9);
            scanf("%d", &tarrivaltime);
            if (tarrivaltime < 0 || tarrivaltime > totalProcessNum + 9) {
                printf("<ERROR> Arrival time should be in [0,%d]\n", totalProcessNum + 9);
                exit(-1);
            }

            printf("Process %d CPU burst (5 ~ 20): ", i + 1);
            scanf("%d", &tCPUburst);
            if (tCPUburst < 5 || tCPUburst > 20) {
                printf("<ERROR> CPU burst should be in [5,20]\n");
                exit(-1);
            }

            printf("Process %d IO burst (0 ~ 10): ", i + 1);
            scanf("%d", &tIOburst);
            if (tIOburst < 0 || tIOburst > 10) {
                printf("<ERROR> IO burst should be in [0,10]\n");
                exit(-1);
            }

            switch (tIOburst) {
            case 0:
                tIOrequest = 0;
                break;
            default:
                printf("Process %d max IO request (1 ~ 5): ", i + 1);
                scanf("%d", &tIOrequest);
                if (tIOrequest < 1 || tIOrequest > 5) {
                    puts("<ERROR> Max IO request should be in [1,5]");
                    exit(-1);
                }
            }

            createProcess(i + 1, tpriority, tarrivaltime, tCPUburst, tIOburst, tIOrequest, rand() % 5 + 1);
        }

        sort_JQ();
        clone_JQ();
        break;

    default:
        puts("<ERROR> Please choose option 1 or 2");
        exit(-1);
    }

    print_JQ();

    // 버퍼에 남은 개행 제거 
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}





void main() {
    Config();
    srand(time(NULL));
    printf("Welcome to CPU Scheduling Simulator\n");
    createProcesses();

    while (1) {
        char input[100];
        int slowch, algorithm, preemptive = 0, timequantum;
        int amount = 200;

        puts("Which algorithm would you want to test?");
        puts("1. FCFS / 2. SJF / 3. PRIORITY / 4. RR / 5. Evaluate algorithms / 6. Exit");

        printf("Enter choice: ");
        fgets(input, sizeof(input), stdin);

        // 빈 입력 체크
        if (input[0] == '\n') {
            puts("<ERROR>Empty input! Please enter a number from 1 to 6.");
            continue;
        }

        if (sscanf(input, "%d", &algorithm) != 1 || algorithm < 1 || algorithm > 6) {
            puts("<ERROR>Wrong input! Please enter a number from 1 to 6.");
            continue;
        }

        switch (algorithm) {
            case 1:
                puts("Slow check (yes = 1),(no = 0)");
                fgets(input, sizeof(input), stdin);
                if (input[0] == '\n' || sscanf(input, "%d", &slowch) != 1 || (slowch != 0 && slowch != 1)) {
                    puts("<ERROR>Wrong input! Please enter 0 or 1.");
                    continue;
                }
                startSimulation(algorithm, 0, 1, amount, slowch);
                break;

            case 2:
            case 3:
                puts("Preemptive (=1) / Nonpreemptive (=0)");
                fgets(input, sizeof(input), stdin);
                if (input[0] == '\n' || sscanf(input, "%d", &preemptive) != 1 || (preemptive != 0 && preemptive != 1)) {
                    puts("<ERROR>Wrong input! Please enter 0 or 1.");
                    continue;
                }

                puts("Slow check (yes = 1),(no = 0)");
                fgets(input, sizeof(input), stdin);
                if (input[0] == '\n' || sscanf(input, "%d", &slowch) != 1 || (slowch != 0 && slowch != 1)) {
                    puts("<ERROR>Wrong input! Please enter 0 or 1.");
                    continue;
                }

                startSimulation(algorithm, preemptive, 1, amount, slowch);
                break;

            case 4:
                printf("Set time quantum: ");
                fgets(input, sizeof(input), stdin);
                if (input[0] == '\n' || sscanf(input, "%d", &timequantum) != 1 || timequantum < 1) {
                    puts("<ERROR>Wrong input! Time quantum must be a positive integer.");
                    continue;
                }

                puts("Slow check (yes = 1),(no = 0)");
                fgets(input, sizeof(input), stdin);
                if (input[0] == '\n' || sscanf(input, "%d", &slowch) != 1 || (slowch != 0 && slowch != 1)) {
                    puts("<ERROR>Wrong input! Please enter 0 or 1.");
                    continue;
                }

                startSimulation(algorithm, 0, timequantum, amount, slowch);
                break;

            case 5:
                clear_evals();
                startSimulation(1, 0, 3, amount, 0);
                startSimulation(2, 0, 3, amount, 0);
                startSimulation(2, 1, 3, amount, 0);
                startSimulation(3, 0, 3, amount, 0);
                startSimulation(3, 1, 3, amount, 0);
                startSimulation(4, 0, 3, amount, 0);
                evaluate();
                break;

            case 6:
                clear_JQ();
                clear_RQ();
                clear_T();
                clear_WQ();
                clearClone_JQ();
                clear_evals();
                exit(0);
        }
    }
}

        

