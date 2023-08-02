#include "thread.h"
#include "queue.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <valgrind/valgrind.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#define STACK_SIZE 1024*16 //2048*2048

struct thread_c;
enum status{
    ACTIVE, FINISHED, SLEEP
};

struct join_chain{
    struct thread_c* next;
    struct thread_c* head;
    struct thread_c* tail;
};

typedef struct thread_c {
    thread_t id;
    ucontext_t context;
    TAILQ_ENTRY(thread_c) entry;
    void *ret;
    enum status state;
    int valgrind_stackid;
    struct join_chain jc;
    struct thread_c* waiting_last_mutex_locked;
    #ifdef STATIC
    char stack[STACK_SIZE];
    #endif
} thread_c;

#define MAX_THREAD 20000
#ifndef STATIC
thread_c* thread_table[MAX_THREAD];
#else
thread_c thread_table[MAX_THREAD];
#endif

thread_t main_id = (thread_t) 0x0DEADBEEFUL;
thread_c *curr_t = NULL; //RULE : Should always be the first of the active queue !
thread_c * main_t = NULL;



#define TAILQ_REMOVE_HEAD(head, field) do {		\
	thread_c* temp = TAILQ_FIRST(head);         \
    TAILQ_REMOVE(head, temp, field);		    \
} while (/*CONSTCOND*/0)

TAILQ_HEAD(queue, thread_c);
struct queue active_list = TAILQ_HEAD_INITIALIZER(active_list);
struct queue finish_list = TAILQ_HEAD_INITIALIZER(finish_list);
struct queue wait_list = TAILQ_HEAD_INITIALIZER(wait_list);

void thread_yield_handler(int sig);
struct sigaction prepaSignal;

void __attribute__((constructor)) my_constructor(void) {
    #ifndef STATIC
    main_t = (thread_c *) malloc(sizeof(thread_c ));
    #else
    main_t = thread_table;
    #endif
    main_t->id = main_id;
    getcontext(&main_t->context);
    main_t->context.uc_link = NULL;
    main_t->state = ACTIVE;
    main_t->context.uc_stack.ss_size = STACK_SIZE;
    #ifndef STATIC
    main_t->context.uc_stack.ss_sp = malloc(STACK_SIZE);
    #else
    main_t->context.uc_stack.ss_sp = main_t->stack;
    #endif
    main_t->valgrind_stackid = VALGRIND_STACK_REGISTER(main_t->context.uc_stack.ss_sp, main_t->context.uc_stack.ss_sp + main_t->context.uc_stack.ss_size);
    main_t->context.uc_stack.ss_flags = 0;
    main_t->ret = (void*) 0x0DEADBEEF;
    main_t->jc = (struct join_chain) {NULL, main_t, main_t};
    main_t->waiting_last_mutex_locked = NULL;

    TAILQ_INSERT_TAIL(&active_list, main_t, entry);
    curr_t = main_t;
    #ifndef STATIC
    thread_table[0] = main_t;
    #endif

    prepaSignal.sa_handler= thread_yield_handler;
    sigemptyset(&prepaSignal.sa_mask);
    prepaSignal.sa_flags = 0;
    sigaction(SIGALRM,&prepaSignal,NULL);
    ualarm(100000, 100000);
}


thread_c * get_thread(thread_t thread){
    #ifndef STATIC
    if(thread == main_id) return thread_table[0];
    return thread_table[(size_t) thread];
    #else
    if(thread == main_id) return thread_table;
    return thread_table + ((size_t) thread);
    #endif
}

thread_t thread_self(void){
    return curr_t->id;
}

void function_handler(thread_c* t, void *(*func)(void *), void *funcarg) {
    t->ret = func(funcarg);
    thread_exit(t->ret);
}

void remove_thread(thread_c *t){
    if(t->state == FINISHED)    TAILQ_REMOVE(&finish_list, t, entry);
    else                        TAILQ_REMOVE(&active_list, t, entry);
    VALGRIND_STACK_DEREGISTER(t->valgrind_stackid);
    #ifndef STATIC
    free(t->context.uc_stack.ss_sp);
    free(t);
    #endif
}

size_t last_id = 0;

int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg)
{
    last_id++;
    assert(last_id < MAX_THREAD);
    *newthread = (void *)last_id;
    
    #ifndef STATIC
    thread_c * t = (thread_c *) malloc(sizeof(thread_c));
    #else
    thread_c * t = thread_table + last_id;
    #endif

    if (t == NULL) return -1;
    t->id = *newthread;
    getcontext(&t->context);
    t->context.uc_link = NULL;
    t->context.uc_stack.ss_size = STACK_SIZE;
    #ifndef STATIC
    t->context.uc_stack.ss_sp = malloc(STACK_SIZE);
    #else
    t->context.uc_stack.ss_sp = t->stack;
    #endif
    t->valgrind_stackid = VALGRIND_STACK_REGISTER(t->context.uc_stack.ss_sp, t->context.uc_stack.ss_sp + t->context.uc_stack.ss_size);
    t->context.uc_stack.ss_flags = 0;
    t->state = ACTIVE;
    t->ret = NULL;
    t->jc = (struct join_chain) {NULL, t, t};
    t->waiting_last_mutex_locked = NULL;

    thread_c * temp =  curr_t;
    TAILQ_INSERT_HEAD(&active_list, t, entry);
    curr_t = t;
    //TAILQ_INSERT_TAIL(&active_list, t, entry);
    #ifndef STATIC
    thread_table[(size_t) t->id] = t;
    #endif

    makecontext(&t->context, (void (*)(void))function_handler, 3, t, func, funcarg);
    swapcontext(&temp->context, &t->context);
    return 0;
}

int thread_yield_to(thread_c* new){
    if(curr_t->id == new->id) return 0;
    if(curr_t->state == ACTIVE){
        TAILQ_REMOVE(&active_list, curr_t, entry);
        TAILQ_INSERT_TAIL(&active_list, curr_t, entry);
    }

    if(new->state == SLEEP){
        new->state = ACTIVE;
        TAILQ_REMOVE(&wait_list, new, entry);
        TAILQ_INSERT_HEAD(&active_list, new, entry);
    }else{
        TAILQ_REMOVE(&active_list, new, entry);
        TAILQ_INSERT_HEAD(&active_list, new, entry);
    }

    thread_c* temp = curr_t;
    curr_t = new;
    swapcontext(&temp->context, &new->context);
    return 0;
}

int thread_yield(void){
    thread_c* end = TAILQ_LAST(&active_list, queue);
    if(curr_t->id == end->id) return 0;
    thread_c* next = TAILQ_NEXT(curr_t, entry);
    thread_yield_to(next);
    return 0;
}

void thread_yield_handler(int sig){
    curr_t = TAILQ_FIRST(&active_list);
    thread_c* end = TAILQ_LAST(&active_list, queue);
    if(curr_t->id != end->id)
    {
        thread_c* next = TAILQ_NEXT(curr_t, entry);
        thread_yield_to(next);
    }
}


int thread_join(thread_t thread, void **retval){
    if(curr_t->jc.head->id == thread) return EDEADLK;
    thread_c* joined_thread = get_thread(thread);

    if(joined_thread->state != FINISHED){
        if(joined_thread->jc.next != NULL) return ENOTSUP;

        joined_thread->jc.next = curr_t;
        (curr_t->jc.head)->jc.tail = joined_thread->jc.tail;
        (joined_thread->jc.tail)->jc.head = curr_t->jc.head;

        curr_t->state = SLEEP;
        TAILQ_REMOVE(&active_list, curr_t, entry);
        TAILQ_INSERT_TAIL(&wait_list, curr_t, entry);

        if((joined_thread->jc.tail)->state == SLEEP){
            if(TAILQ_EMPTY(&active_list)) return EDEADLK;
            thread_c* new = TAILQ_FIRST(&active_list);
            thread_yield_to(new);
        }
        else thread_yield_to(joined_thread->jc.tail);

        joined_thread->jc.next = NULL;
        curr_t->jc.tail = curr_t;
        (curr_t->jc.head)->jc.tail = curr_t;
    }

    if (retval != NULL) *retval = joined_thread->ret;
    if (joined_thread->id != main_id) remove_thread(joined_thread);
    
    return 0;
}



void thread_exit(void *retval){
    alarm(0);
    if (retval != NULL) curr_t->ret = retval;
    curr_t->state = FINISHED;

    TAILQ_REMOVE(&active_list, curr_t, entry);
    TAILQ_INSERT_TAIL(&finish_list, curr_t, entry);

    thread_c* new = curr_t->jc.next;
    if(new != NULL){
        thread_yield_to(new);
    }else if(!TAILQ_EMPTY(&active_list)){
        new = TAILQ_FIRST(&active_list);
        thread_yield_to(new);
    }
    ualarm(1000000, 100000);
    setcontext(&main_t->context);
    exit(0);
}


// ______________________MUTEX________________________

typedef struct mutex_c {
    int id;
    thread_t user;
    thread_t last_locked;
} mutex_c;

size_t last_mutex_id = 0;
#define MAX_MUTEX 10000
#ifndef STATIC
mutex_c* mutex_table[MAX_MUTEX];
#else
mutex_c mutex_table[MAX_MUTEX];
#endif

int thread_mutex_init(thread_mutex_t *mutex){
    alarm(0);
    mutex->dummy = (++last_mutex_id);
    assert(last_mutex_id < MAX_MUTEX);

    #ifndef STATIC
    mutex_c* m = malloc(sizeof(mutex_c));
    #else
    mutex_c* m = mutex_table + last_mutex_id;
    #endif
    if(m == NULL) return -1;
    m->id = mutex->dummy;
    m->user = NULL;
    m->last_locked = NULL;

    #ifndef STATIC
    mutex_table[m->id] = m;
    #endif
    return 0;
}

mutex_c * get_mutex(thread_mutex_t mutex){
    if(mutex.dummy <= 0) return NULL;
    #ifndef STATIC
    return mutex_table[mutex.dummy];
    #else
    return mutex_table + ((size_t) mutex.dummy);
    #endif
}

int thread_mutex_destroy(thread_mutex_t *mutex){
    #ifndef STATIC
    free(get_mutex(*mutex));
    #endif
    return 0;
}

int thread_mutex_lock(thread_mutex_t *mutex){
    mutex_c* m = get_mutex(*mutex);

    if(m->user != NULL){
        if(m->last_locked != NULL) curr_t->waiting_last_mutex_locked = get_thread(m->last_locked);
        m->last_locked = curr_t->id;

        curr_t->state = SLEEP;
        TAILQ_REMOVE(&active_list, curr_t, entry);
        TAILQ_INSERT_TAIL(&wait_list, curr_t, entry);
        thread_yield_to(get_thread(m->user));
    }

    m->user = thread_self();
    return 0;
}

int thread_mutex_unlock(thread_mutex_t *mutex){
    mutex_c* m = get_mutex(*mutex);
    m->user = NULL;
    curr_t->state = ACTIVE;

    if(m->last_locked != NULL){
        thread_c* unlocked = get_thread(m->last_locked);
        if(unlocked->waiting_last_mutex_locked != NULL) m->last_locked = unlocked->waiting_last_mutex_locked->id;
        else                                            m->last_locked = NULL;

        thread_yield_to(unlocked);
    }

    return 0;
}



void __attribute__((destructor)) my_destructor(void) {
    thread_c* t;
    while (!TAILQ_EMPTY(&active_list)) {
        t = TAILQ_FIRST(&active_list);
        remove_thread(t);
    }

    while (!TAILQ_EMPTY(&wait_list)) {
        t = TAILQ_FIRST(&wait_list);
        remove_thread(t);
    }

    while (!TAILQ_EMPTY(&finish_list)) {
        t = TAILQ_FIRST(&finish_list);
        remove_thread(t);
    }
}
