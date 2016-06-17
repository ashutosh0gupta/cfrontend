// interface of cfrontend
#ifndef CFRONTEND_H
#define CFRONTEND_H

#include <cfrontend_config.h>
#include <iostream>
#include <cfrontend/program.h>

#ifdef Z3_PATH

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "z3++.h"
#pragma GCC diagnostic pop

#endif


namespace cfrontend{

  // template<class expr> class SimpleMultiThreadedProgram<expr>;

  class Cfrontend {
  public:

    // File* loadFile( const char * );

    //Cfrontend(int, char ** );

    Cfrontend() {};

    class Config{
    public:
      Config() {
        dumpLLVMcfg = false;
        verbose_set.insert( "mkthread");
      };

      // getter functions
      bool chkVeboseLevel( unsigned level ) { return level > verbosity; }
      bool verbose( const std::string str ) const {
        return verbose_set.find(str) != verbose_set.end();
      }
      std::ostream& out() { return std::cout; }
      std::string& getInputFile() { return inputFileName; }
      std::vector< std::pair<std::string,int> >& setThreadList() {
        return threadList;
      }
      bool isOutLLVMcfg() { return dumpLLVMcfg; }

      // setter functions
      void setVerbosity( unsigned level ) { verbosity = level; }
      void setInputFileName( std::string& str ) { inputFileName = str; };
      void setThreadList( std::vector< std::pair<std::string,int> >& thrs ) {
        threadList = thrs;
      }
      void setOutLLVMcfg( bool val ) { dumpLLVMcfg = val; }

    private:
      std::string inputFileName;
      unsigned verbosity; // todo: outdated
      std::set< std::string > verbose_set;
      std::vector< std::pair<std::string,int> > threadList;
      bool dumpLLVMcfg;
    };

    //-------------------------------------------------
    // get functions
    Config& getConfig() { return config; }

    //-------------------------------------------------
    // set functions


    //-------------------------------------------------
    // action functions


    void init();

#ifdef Z3_PATH
    SimpleMultiThreadedProgram<z3::expr>* z3Pass( z3::context&, std::string& );
    SimpleMultiThreadedProgram<z3::expr>*
    z3Pass( z3::context& ctx ) { return z3Pass( ctx, config.getInputFile() ); }
#endif

  private:
    Config config;

    template <class ExprHandler>
    SimpleMultiThreadedProgram<typename ExprHandler::expr>*
    pass( ExprHandler*, std::string& );

  };

}

#endif
