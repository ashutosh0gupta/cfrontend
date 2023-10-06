int g1,g2,g3; // All are itegers

//----------------------------------------------------------------------------
// TODO: remove this
void assume( bool b );
void assert( bool b );


/* int bar(int x) { */
/*   if( x < 0 ) { */
/*     x++; */
/*   }else{ */
/*     x--; */
/*   }  */
/*   x = g1 + x; */
/*   return x; */
/* } */

//----------------------------------------------------------------------------
int foo(int x, int y ) {
  /* int result = x > 61 ? x : (x / 17); */
  int z = 10*(x - 1) + y;
  {
    int z = x + y;
    x = z - y;
  }
  x = g2 + g1;
  g1 = (x + 1)*y + g1;
  x=0;
  bool p = false;
  for( unsigned i = g1; i < 10; i++ ) {
    x = x + 1;
    if(  x > y || x + y > 0 ) {
      /* z = z + 1; */
      assume( x > y || ( x + 2*y > 0) );
      // assume( x > y || (!p && x + 2*y > 0) );
      z = x > 61 ? x : (x * 17 + z);
      int z = 10;
      x = x + z;
    }
    i = 2;
    // p = !p;
  }
  assume( x > 0 );
  x = x + 2 + z;
  assert( x + y > 2 );
  y = y + 5;
  return x;
}

