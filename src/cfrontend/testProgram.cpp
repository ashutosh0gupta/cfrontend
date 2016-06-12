#include <cfrontend_config.h>
#include "cfrontend/program.h"

#ifdef Z3_PATH

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "z3++.h"
#pragma GCC diagnostic pop

namespace cfrontend{

void test_fun() {
  // z3 context
  z3::context c;

  //dummy init
  z3::expr init_(c);

  SimpleMultiThreadedProgram<z3::expr> p(3,init_);

  auto l0_0 = p.createFreshLocation(0);
  auto l0_1 = p.createFreshLocation(0);
  auto l0_2 = p.createFreshLocation(0);
  auto l0_3 = p.createFreshLocation(0);
  p.setStartLocation( 0, l0_0 );

  auto l1_0 = p.createFreshLocation(1);
  auto l1_1 = p.createFreshLocation(1);
  auto l1_2 = p.createFreshLocation(1);
  auto l1_3 = p.createFreshLocation(1);
  p.setStartLocation( 1, l1_0 );

  auto l2_0 = p.createFreshLocation(2);
  auto l2_1 = p.createFreshLocation(2);
  auto l2_2 = p.createFreshLocation(2);
  auto l2_3 = p.createFreshLocation(2);
  p.setStartLocation( 2, l2_0 );


  // globals
  z3::expr x   = c.int_const("x" );
  z3::expr xp  = c.int_const("xp");
  z3::expr y   = c.int_const("y" );
  z3::expr yp  = c.int_const("yp");
  //Locals
  z3::expr t0  = c.int_const("t0" );
  z3::expr t0p = c.int_const("t0p");
  z3::expr t1  = c.int_const("t1" );
  z3::expr t1p = c.int_const("t1p");
  z3::expr t2  = c.int_const("t2" );
  z3::expr t2p = c.int_const("t2p");

  z3::expr init = ( ( x == 0 ) && ( y == 1 ) );

  p.setInitialStates( init );

  p.addGlobal( x, xp );
  p.addGlobal( y, yp );

  p.addLocal( 0, t0, t0p );
  p.addLocal( 1, t1, t1p );
  p.addLocal( 2, t2, t2p );

  z3::expr t0_x = (t0p == x );
  z3::expr t0p_t0_plus_1 = (t0p == t0 + 1);
  z3::expr x_t0 = (xp == t0 );
  z3::expr tru = c.bool_val( true );

  p.addBlock( 0, l0_0, l0_1, t0_x          );
  p.addBlock( 0, l0_1, l0_2, t0p_t0_plus_1 );
  p.addBlock( 0, l0_2, l0_3, x_t0          );
  p.addBlock( 0, l0_3, l0_0, tru           );

  z3::expr t1_x = (t1p == x);
  z3::expr t1p_t1_plus_1 = (t1p == t1 + 1);
  z3::expr x_t1 = (xp == t1 );

  p.addBlock( 1, l1_0, l1_1, t1_x          );
  p.addBlock( 1, l1_1, l1_2, t1p_t1_plus_1 );
  p.addBlock( 1, l1_2, l1_3, x_t1          );

  z3::expr t2_x = (t2p == x );
  z3::expr t2p_t2_plus_1 = (t2p == t2 + 1);
  z3::expr x_t2 = (xp == t2 );

  p.addBlock( 2, l2_0, l2_1, t2_x          );
  p.addBlock( 2, l2_1, l2_2, t2p_t2_plus_1 );
  p.addBlock( 2, l2_2, l2_3, x_t2          );

  p.printDot( std::cerr );

  //TODO:
  // -- cutpoints
  // -- various access functions

}

}

#endif
