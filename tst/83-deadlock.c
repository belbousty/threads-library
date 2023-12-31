#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include "thread.h"

/* test de detection d'un deadlock lors d'un cycle de thread qui joignent tous le suivant.
 * main(th0) joine th1 qui joine th2 qui joine main.
 * il faut qu'un join renvoie -1 quand il detecte le deadlock, et les autres renvoient 0 normalement.
 */


static thread_t th0, th1, th2, th3, th4;
int totalerr = 0;

static void * thfunc4(void *dummy __attribute__((unused)))
{
    void *res;
    int err = thread_join(th2, &res);
    printf("join th4->th2 = %d\n", err);
    totalerr += err;
    thread_exit(NULL);
}

static void * thfunc3(void *dummy __attribute__((unused)))
{
    for(int i = 0; i < 10; i++) thread_yield();
    thread_exit(NULL);
}

static void * thfunc2(void *dummy __attribute__((unused)))
{
    void *res;
    int err = thread_create(&th3, thfunc3, NULL);
    assert(!err);
    err = thread_create(&th4, thfunc4, NULL);
    assert(!err);

    thread_yield();
    err = thread_join(th1, &res);
    printf("join th2->th1 = %d\n", err);
    totalerr += err;
    thread_exit(NULL);
}

static void * thfunc1(void *dummy __attribute__((unused)))
{
    void *res;
    int err = thread_create(&th2, thfunc2, NULL);
    assert(!err);
    err = thread_join(th0, &res);
    printf("join th1->th0 = %d\n", err);
    totalerr += err;
    thread_exit(NULL);
}

int main()
{
    void *res;
    int err;

    th0 = thread_self();

    err = thread_create(&th1, thfunc1, NULL);
    assert(!err);
    err = thread_join(th4, &res);
    printf("join th0->th4 = %d\n", err);
    totalerr += err;
    printf("somme des valeurs de retour = %d\n", totalerr);
    assert(totalerr == EDEADLK);

    if ( totalerr == EDEADLK ) {
	return EXIT_SUCCESS;
    }
    else {
	return EXIT_FAILURE;
    }
}
