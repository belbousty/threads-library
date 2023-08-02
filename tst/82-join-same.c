#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include "thread.h"

/* test du cas où deux threads attendent le même thread.
 * main(th0) joine th1 et th1 qui joine th1 qui join th2.
 * il faut qu'un join renvoie ENOTSUP quand il detecte le join sur un même thread, et les autres renvoient 0 normalement.
 */


static thread_t th0, th1, th2;
int totalerr = 0;

static void * thfunc2(void *dummy __attribute__((unused)))
{
    void *res;
    int err = thread_join(th1, &res);
    printf("join th2->th1 = %d\n", err);
    totalerr += err;
    printf("Thread 2 terminé\n");
    assert(totalerr == EOPNOTSUPP);
    return 0;
}

static void * thfunc1(void *dummy __attribute__((unused)))
{
    thread_create(&th2, thfunc2, NULL);
    thread_yield();
    printf("Thread 1 terminé\n");
    return 0;
}

int main()
{
    void *res;

    th0 = thread_self();

    thread_create(&th1, thfunc1, NULL);
    int err = thread_join(th1, &res);
    printf("join th0->th1 = %d\n", err);
    totalerr += err;
    assert(totalerr == EOPNOTSUPP);

    err = thread_join(th2, &res);
    printf("join th0->th2 = %d\n", err);
    totalerr += err;

    printf("Main terminé\n");
    printf("somme des valeurs de retour = %d\n", totalerr);
    assert(totalerr == EOPNOTSUPP);
	return EXIT_SUCCESS;
}
