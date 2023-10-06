#include <pthread.h>
#include <stdio.h>

int x = 0, y=0;

void assume( int );
void assert( int );

void* p0(void * a_ptr) {
  // *((int *)a_ptr) = 2;
  assume( x > 0 );
  x = 1;
  y = 1;
  return NULL;
}

void* p1(void * a_ptr) {
  // *((int *)a_ptr) = 3;
  int r1 = y;
  int r2 = x;
  assert( r1 == 0 && r2 == 1);
  return NULL;
}


int main() {
  int a = 0;
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_create(&thr_0, NULL, p0, &a );
  pthread_create(&thr_1, NULL, p1, &a );

  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  assert( x > 0 );
}

