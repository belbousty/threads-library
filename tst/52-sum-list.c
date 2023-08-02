#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include "thread.h"


static int *list = NULL;

static void * sum_from(void *_value)
{
  unsigned long index = (unsigned long) _value;
  long sum_part = 0;

  for(size_t i=index; i<index+4; i++){
    thread_yield(); //on passe un peu la main aux autres
    sum_part += list[i];
  }

  return (void*) sum_part;
}

long sum_checker(size_t n)
{
  long sum = 0;
  for(size_t i=0; i < n; i++) sum += list[i];
  return sum;
}

int main(int argc, char *argv[])
{
  thread_t* th = NULL;
  unsigned long n, nt, i;
  long sum = 0;
  struct timeval tv1, tv2;
  int err;
  double s;

  if (argc < 2) {
    printf("argument manquant: entier n nombre de threads\n");
    return -1;
  }

  nt = atoi(argv[1]);
  n = (nt*4);
  list = malloc(n*sizeof(int));
  th = malloc(nt*sizeof(thread_t));
  if (!list || !th) { perror("malloc"); return -1; }

  srand(0); //Seed de la génération pseudo-aléatoire, peut être modifiée.
  printf("Liste : [ ");
  for(size_t i=0; i < n; i++){
    list[i] = (rand() % 21) - 10; //Les entiers générés sont compris entre [-10,10]
    printf("%d ", list[i]);
  }
  printf("]\n");

  gettimeofday(&tv1, NULL);
  for(i=0; i<nt; i++) {
    err = thread_create(&th[i], sum_from, (void*) (4*i));
    assert(!err);
  }

  // on leur passe la main, ils vont tous terminer
  for(i=0; i<nt; i++) {
    thread_yield();
  }

  void* res;
  // on les joine tous, maintenant qu'ils sont tous morts
  for(i=0; i<nt; i++) {
    err = thread_join(th[i], &res);
    assert(!err);
    sum += (long) res;
  }
  gettimeofday(&tv2, NULL);
  s = (tv2.tv_sec-tv1.tv_sec) + (tv2.tv_usec-tv1.tv_usec) * 1e-6;

  long real_sum = sum_checker(n);
  if ( sum != real_sum ) {
    printf("somme de la liste calculée %ld != (réelle) %ld (FAILED)\n", sum, real_sum );
    free(list); free(th);
    return EXIT_FAILURE;
  } else {
    printf("somme de la liste calculée %ld == (réelle) %ld en %e s\n", sum, real_sum, s);
    free(list); free(th);
    return EXIT_SUCCESS;
  }
}
