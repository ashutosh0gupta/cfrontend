// the file that calls llvm and generates a program object

#include <cfrontend_config.h>
#include <iostream>

#include "cfrontend/cfrontend.h"

namespace cfrontend{

  extern void test_fun();

  void Cfrontend::init() {
// #ifdef Z3_PATH
//     test_fun();
// #endif
    std::cout << "# " << PACKAGE_STRING << std::endl;
  }

  // Cfrontend::Cfrontend( int argc, char * argv[] )
  //   : config( argc, argv ) { init(); }

}
