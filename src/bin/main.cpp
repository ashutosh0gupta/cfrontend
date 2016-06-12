#include "cfrontend/cfrontend.h"

/***************************************************************************\
 *                                                                         *
 *                        CFRONTEND MAIN                                   *
 *                                                                         *
\***************************************************************************/


void cmdParser( int argc, char * argv[], cfrontend::Cfrontend::Config* config );

int main( int argc, char * argv[] ) {

  z3::context c;
  cfrontend::Cfrontend* cf = new cfrontend::Cfrontend();

  cmdParser( argc, argv, &(cf->getConfig()) );
  cf->init();

  auto program = cf->z3Pass( c );
  program->printDot( std::cout );
  delete program;
  delete cf;

  return 0;
}
