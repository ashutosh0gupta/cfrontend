#ifndef CFRONTEND_LLVMPASS_H
#define CFRONTEND_LLVMPASS_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "llvm/Pass.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#pragma GCC diagnostic pop

#include "cfrontend/cfrontend.h"
#include "cfrontend/program.h"
#include "cfrontend/llvmUtils.h"
#include <map>

namespace cfrontend{

  class SplitAtAssumePass : public llvm::FunctionPass {
  public:
    static char ID;
    SplitAtAssumePass() : llvm::FunctionPass(ID) {}
    virtual bool runOnFunction( llvm::Function &f );
    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const;
  };

  template <typename EHandler>
  class BuildSMTProgram : public llvm::FunctionPass {
  public:
    typedef typename
    SimpleMultiThreadedProgram<typename EHandler::expr>::location_id_type
    program_location_id_t;
    typedef typename
    SimpleMultiThreadedProgram<typename EHandler::expr>::thread_id_type
    thread_id_t;
    typedef
    std::map<const llvm::Value*,typename EHandler::expr> ValueExprMap;

    static char ID;

    BuildSMTProgram( EHandler* eHandler_,
                     Cfrontend::Config& config_,
      SimpleMultiThreadedProgram<typename EHandler::expr>* program_ )
      : llvm::FunctionPass(ID)
      , eHandler( eHandler_ )
      , config(config_)
      , program( program_ )
      , threadCount( 0 )
    {}

    virtual bool runOnFunction( llvm::Function & );

    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const {
      au.setPreservesAll();
      //TODO: ...
      // au.addRequired<llvm::Analysis>();
    }


  private:
    EHandler* eHandler;
    Cfrontend::Config& config;
    SimpleMultiThreadedProgram<typename EHandler::expr>* program;
    unsigned threadCount;
    std::map< const llvm::BasicBlock*, program_location_id_t > numBlocks;
    std::vector<unsigned> pendingSrc;
    std::vector<unsigned> pendingDst;
    std::vector<typename EHandler::expr> pendingTerms;

    //private functions
    program_location_id_t getBlockCount( const llvm::BasicBlock* b  );
    void initBlockCount( llvm::Function &f, size_t threadId );

    // typename EHandler::expr
    // getPhiMap( const llvm::PHINode*, StringVec, ValueExprMap& );

    typename EHandler::expr
    getPhiMap( const llvm::PHINode* p, ValueExprMap& m );

    typename EHandler::expr
    translateBlock( llvm::BasicBlock* b, unsigned succ, ValueExprMap& );

    // void post_insertEdge( unsigned, unsigned, typename EHandler::expr );

    void resetPendingInsertEdge();
    void addPendingInsertEdge( unsigned, unsigned, typename EHandler::expr);
    void applyPendingInsertEdges( unsigned );

    typename EHandler::expr
    getTerm( const llvm::Value* op ,ValueExprMap& m ) {
      if( const llvm::ConstantInt* c = llvm::dyn_cast<llvm::ConstantInt>(op) ) {
        int i = readInt( c );
        return eHandler->mkIntVal( i );
      }else if( auto c = llvm::dyn_cast<llvm::ConstantPointerNull>(op) ) {
      // }else if( LLCAST( ConstantPointerNull, c, op) ) {
        return eHandler->mkIntVal( 0 );
      }else if( const llvm::Constant* c = llvm::dyn_cast<llvm::Constant>(op) ) {
        cfrontend_error( "un recognized constant!" );
        // int i = readInt(c);
        // return eHandler->mkIntVal( i );
      }else if( eHandler->isLocalVar( op ) ) {
        return eHandler->getLocalVar( op );
      }else{
        auto it = m.find( op );
        if( it == m.end() ) {
          op->print( llvm::outs() ); //llvm::outs() << "\n";
          cfrontend_error( "local term not found!" );
        }
        return it->second;
      }
    }

    bool
    isValueMapped( const llvm::Value* op ,ValueExprMap& m ) {
      if( const llvm::Constant* c = llvm::dyn_cast<llvm::Constant>(op) ) {
      }else if( !eHandler->isLocalVar( op ) ) {
        auto it = m.find( op );
        if( it == m.end() )
          return false;
      }
      return true;
    }

  };

}

#endif
