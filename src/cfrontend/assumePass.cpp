
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-parameter"

#include "cfrontend/llvmPass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"

// #pragma GCC diagnostic pop

namespace cfrontend{

  //--------------------------------------------------------------------------
  // Splitting for assume pass

  char SplitAtAssumePass::ID = 0;

  bool SplitAtAssumePass::runOnFunction( llvm::Function &f ) {
    llvm::LLVMContext &C = f.getContext();

    llvm::BasicBlock *dum = llvm::BasicBlock::Create( C, "dummy", &f, f.end());
    new llvm::UnreachableInst(C, dum);
    llvm::BasicBlock *err = llvm::BasicBlock::Create( C, "err", &f, f.end() );
    new llvm::UnreachableInst(C, err);

    std::vector<llvm::BasicBlock*> splitBBs;
    std::vector<llvm::CallInst*> calls;
    std::vector<llvm::Value*> args;
    std::vector<llvm::Instruction*> splitIs;
    std::vector<bool> isAssume;

    //collect calls to assume statements
    auto bit = f.begin(), end = f.end();
    for( ; bit != end; ++bit ) {
      llvm::BasicBlock* bb = bit;
      auto iit = bb->begin(), iend = bb->end();
      size_t bb_sz = bb->size();
      for( unsigned i = 1; iit != iend; ++iit ) {
        i++;
        llvm::Instruction* I = iit;
        if( llvm::CallInst* call = llvm::dyn_cast<llvm::CallInst>(I) ) {
          llvm::Function* fp = call->getCalledFunction();
          // TODO : llvm question how to deal with undefined functions?
          if( fp != NULL &&
              ( fp->getName() == "_Z6assumeb" ||
                fp->getName() == "_Z6assertb" ) &&
              i < bb_sz ) {
            splitBBs.push_back( bb );
            auto arg_iit = iit;
            llvm::Instruction* arg = --arg_iit;
            llvm::Value * arg0 = call->getArgOperand(0);
            if( arg != arg0 ) cfrontend_error( "last out not passed!!" );
            calls.push_back( call );
            args.push_back( arg0 );
            // // TODO: how to access next iit, without making local copy
            auto local_iit = iit;
            splitIs.push_back( ++local_iit );
            isAssume.push_back( (fp->getName() == "_Z6assumeb") );
          }
        }
      }
    }

    // Reverse order splitting for the case if there are more than
    // one assumes in one basic block
    for( unsigned i = splitBBs.size(); i != 0 ; ) { i--;
      llvm::BasicBlock* head = splitBBs[i];
      llvm::BasicBlock* tail = llvm::SplitBlock( head, splitIs[i], this );
      llvm::Instruction* cmp;
      if( llvm::Instruction* c = llvm::dyn_cast<llvm::Instruction>(args[i]) ) {
        cmp = c;
      }else{ cfrontend_error( "instruction expected here!!"); }
      llvm::BasicBlock* elseBlock = isAssume[i] ? dum : err;
      llvm::BranchInst *branch =llvm::BranchInst::Create( tail, elseBlock, cmp);
      llvm::ReplaceInstWithInst( head->getTerminator(), branch );
      calls[i]->eraseFromParent();
      // head->getInstList().erase(calls[i]);
      if( llvm::isa<llvm::PHINode>(cmp) ) {
        removeBranchingOnPHINode( branch );
      }
    }

    return false;
  }

  void SplitAtAssumePass::getAnalysisUsage(llvm::AnalysisUsage &au) const {
    // it modifies blocks therefore may not preserve anything
    // au.setPreservesAll();
    //TODO: ...
    // au.addRequired<llvm::Analysis>();
  }

  //--------------------------------------------------------------------------
  // .... pass

  //--------------------------------------------------------------------------
}
