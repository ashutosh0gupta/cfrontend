
// command line parser for cfrontend

#include <sstream>
#include "llvm/Support/CommandLine.h"
#include "cfrontend/utils.h"
#include "cfrontend/cfrontend.h"

// namespace cfrontend{

//----------------------------------------------------------------------------
//Specify static

  llvm::cl::opt<std::string>
  inputFileName_static( llvm::cl::Positional,
                        llvm::cl::desc( "<input LLVM bitcode file>" ),
                        llvm::cl::Required,
                        llvm::cl::value_desc( "filename" ) );

  llvm::cl::opt<unsigned>
  verbosity_static( "verbose", llvm::cl::desc("verbosity level") );
  llvm::cl::alias
  verbosity_static_a("v", llvm::cl::desc("Alias for -verbose"),
                     llvm::cl::aliasopt(verbosity_static));


  llvm::cl::opt<bool>
  outLLVMcfg_static( "out-cfg", llvm::cl::desc("out LLVM cfg") );

  llvm::cl::list<std::string>
  threadList_static("thr",
        llvm::cl::desc("comma seperated list of threads (<funName>,copyNum,)+"),
                    llvm::cl::CommaSeparated);

//----------------------------------------------------------------------------

  void c2bc( std::string& filename, std::string& outname ) {
    // make a system call
    std::ostringstream cmd;
    cmd << "clang-3.6 -emit-llvm -O0 -g " << filename << " -o " << outname << " -c";
    if( system( cmd.str().c_str() ) != 0 ) exit(1);
  }

void cmdParser( int argc, char * argv[], cfrontend::Cfrontend::Config* config ){
    // llvm::StringMap< llvm::cl::Option* >& Map =
    //   llvm::cl::getRegisteredOptions();
    // if( Map.count("version") > 0 ) {
    //   Map["version"]->setDescription( "hippi hippi hoye hoye" );
    //   // Map["version"]->setDescription( PACKAGE_STRING );
    // }
    llvm::cl::ParseCommandLineOptions(argc, argv);

    std::string inputFileName = inputFileName_static;
    bool name_error = 1;
    {
      size_t sz = inputFileName.size();
      if( sz > 0 && inputFileName[sz-1] == 'c' ) {
        if( sz > 1 && inputFileName[sz-2] == '.' ) {
          std::string outfile = inputFileName+".bc";
          c2bc( inputFileName,  outfile );
          inputFileName = outfile;
        }else if( sz < 4 || inputFileName[sz-2] != 'b' ) {
          goto FILE_NAME_ERROR;
        }
      }else if( sz > 4 && inputFileName[sz-1] == 'p'
                       && inputFileName[sz-2] == 'p'
                       && inputFileName[sz-3] == 'c'
                       && inputFileName[sz-4] == '.') {
        std::string outfile = inputFileName+".bc";
        c2bc( inputFileName,  outfile );
        inputFileName = outfile;
      }else{
        goto FILE_NAME_ERROR;
      }
    }
    name_error = 0;
 FILE_NAME_ERROR:
    if(name_error) {
        cfrontend_error( "expect *.bc or *.c filename!" );
    }

    config->setOutLLVMcfg( outLLVMcfg_static );
    config->setInputFileName( inputFileName );
    config->setVerbosity( verbosity_static );

    std::vector< std::pair<std::string,int> > threadList;
    size_t thr_sz = threadList_static.size();
    for( unsigned i = 0; i < thr_sz; ) {
      std::string name = threadList_static[i];
      ++i;
      int copies = 1;
      std::string str = "*"; // dummy initialization
      if( i < thr_sz ) {
        str = threadList_static[i];
        if( str == "*" ) {
          copies = -1;
          ++i;
        }else if( str.size() > 0 && std::isdigit( str[0] )  ) {
          copies = atoi( str.c_str() );
          ++i;
        }
      }
      if( name == "" || str == "" ) {
        cfrontend_error( "wrong parameters to option -thr!");
      }
      std::cerr << name << ","<< copies << "\n";
      std::pair<std::string,int> thr = std::make_pair( name, copies );
      threadList.push_back( thr );
    }
    config->setThreadList( threadList );
  }

  // Cfrontend::Config::Config( int argc, char * argv[] ) {
  //   llvm::StringMap< llvm::cl::Option* > Map;
  //   // llvm::cl::getRegisteredOptions(Map);
  //   // if( Map.count("version") > 0 ) {
  //   //   Map["version"]->setDescription( PACKAGE_STRING );
  //   // }
  //   llvm::cl::ParseCommandLineOptions(argc, argv);
  //   inputFileName = inputFileName_static;
  //   size_t sz = inputFileName.size();

  //   if( inputFileName[sz-1] == 'c' ) {
  //     if( inputFileName[sz-2] == '.' ) {
  //       std::string outfile = inputFileName+".bc";
  //       c2bc( inputFileName,  outfile );
  //       inputFileName = outfile;
  //     }else if( inputFileName[sz-2] != 'b' ) {
  //       cfrontend_error( "expect *.bc or *.c filename!" );
  //     }
  //   }else{
  //       cfrontend_error( "expect *.bc or *.c filename!" );
  //   }
  //   verbosity = verbosity_static;
  // }

// }
