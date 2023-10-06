#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/DebugInfo.h"
// #include "llvm/Support/InstIterator.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"

#include "cfrontend/utils.h"
#include "cfrontend/llvmUtils.h"

namespace cfrontend{
  bool hasBoolType(const llvm::Value* v ) {
    return v->getType()->isIntegerTy(1);
  }

  bool hasDebugInfo( const llvm::Instruction* I ) {
    const llvm::BasicBlock* b = I->getParent();
    auto it = b->begin(), end = b->end();
    for(; it!= end ; ++it ) {
      const llvm::Instruction* tmpI = &(*it);
      if( tmpI == I ) break;
    }
    if( it == end ) return false;
    it++;
    if( it == end ) return false;
    const llvm::Instruction* nextI = &(*it);
    if( const llvm::DbgValueInst* dVal =
        llvm::dyn_cast<llvm::DbgValueInst>(nextI) ) {
      const llvm::Value* var = dVal->getValue();
      if( var == I ) return true;
    }
    return false;
  }

  void
  removeBranchingOnPHINode( llvm::BranchInst *branch ) {
    if( branch->isUnconditional() ) return;
    llvm::Value* cond = branch->getCondition();
    if( llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(cond) ) {
      llvm::BasicBlock* phiBlock = branch->getParent();
      llvm::BasicBlock* phiDstTrue = branch->getSuccessor(0);
      llvm::BasicBlock* phiDstFalse = branch->getSuccessor(1);
      if( phiBlock->size() == 2 && cond == (phiBlock->getInstList()).begin() &&
          phi->getNumIncomingValues() == 2 ) {
        llvm::Value* val0      = phi->getIncomingValue(0);
        llvm::BasicBlock* src0 = phi->getIncomingBlock(0);
        llvm::BranchInst* br0  = (llvm::BranchInst*)(src0->getTerminator());
        unsigned br0_branch_idx = ( br0->getSuccessor(0) ==  phiBlock ) ? 0 : 1;
        if( llvm::ConstantInt* b = llvm::dyn_cast<llvm::ConstantInt>(val0) ) {
          assert( b->getType()->isIntegerTy(1) );
          llvm::BasicBlock* newDst = b->getZExtValue() ?phiDstTrue:phiDstFalse;
          br0->setSuccessor( br0_branch_idx, newDst );
        }else{cfrontend_error( "unseen case, not known how to handle!" );}
        llvm::Value*      val1 = phi->getIncomingValue(1);
        llvm::BasicBlock* src1 = phi->getIncomingBlock(1);
        llvm::BranchInst* br1  = (llvm::BranchInst*)(src1->getTerminator());
        llvm::Instruction* cmp1 = llvm::dyn_cast<llvm::Instruction>(val1);
        assert( cmp1 );
        assert( br1->isUnconditional() );
        if( cmp1 != NULL && br1->isUnconditional() ) {
          llvm::BranchInst *newBr =
            llvm::BranchInst::Create( phiDstTrue, phiDstFalse, cmp1);
          llvm::ReplaceInstWithInst( br1, newBr );
          // TODO: memory leaked here? what happend to br1 was it deleted??
          llvm::DeleteDeadBlock( phiBlock );
          removeBranchingOnPHINode( newBr );
        }else{cfrontend_error( "unseen case, not known how to handle!" );}
      }else{cfrontend_error( "unseen case, not known how to handle!" );}
    }
  }

  void
  findPhiTrail( const llvm::PHINode* phi,
                std::vector< const llvm::PHINode* >& infoLessValues,
                //ValueVec& infoWithValues,
                std::vector< std::pair<const llvm::PHINode*,unsigned> >&
                nonPHIEndings ) {
    if( exists( infoLessValues, phi ) ) return;
    // only call for infoless phi??
    infoLessValues.push_back( phi );
    for( unsigned i=0, num = phi->getNumIncomingValues(); i < num; i++ ) {
      llvm::Value* v = phi->getIncomingValue(i);
      if( !llvm::isa<llvm::Instruction>(v) ||
          hasDebugInfo( llvm::dyn_cast<llvm::Instruction>(v) ) ) {
        nonPHIEndings.push_back( std::make_pair( phi, i ) );
        // infoWithValues.push_back( v );
      }else if( const llvm::PHINode* p = llvm::dyn_cast<llvm::PHINode>(v) ) {
        findPhiTrail( p, infoLessValues, nonPHIEndings );
      }else{
        nonPHIEndings.push_back( std::make_pair( phi, i ) );
      }
    }
  }

  void
  findPhiTrail( const llvm::Value* I,
                ValueVec& infoLessValues, ValueVec& infoWithValues,
                std::vector< std::pair<const llvm::PHINode*,unsigned> >&
                phiWithConsts,
                ValueVec& unrecognizedValues
                ) {

    if( exists( infoLessValues, I ) || exists( infoWithValues, I ) ) {
      return;
    }

    if( hasDebugInfo( llvm::dyn_cast<llvm::Instruction>(I) ) ) {
      infoWithValues.push_back( I );
    }else{
      if( const llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(I) ) {
        infoLessValues.push_back( I );
        for( unsigned i=0, num = phi->getNumIncomingValues(); i < num; i++ ) {
          llvm::Value* v = phi->getIncomingValue(i);
          if( llvm::isa<llvm::Constant>(v) ) {
            phiWithConsts.push_back( std::make_pair(phi,i) );
          }else{
            findPhiTrail( v, infoLessValues, infoWithValues, phiWithConsts,
                          unrecognizedValues);
          }
        }
      }else{
        unrecognizedValues.push_back(I);
      }
    }
  }

  //-------------------------------
  // TODO: known issues
  //    -- variables with same name and declared at same line
  //       are indistinuishable e. g. int x = 2; { y = x+2; int x = y+5; }
  //    -- can not deal with inlined functions their naming conventions

  void buildLocalNameMap( const llvm::Function& f,
                          std::map<const llvm::Value*, std::string>& nameMap) {
    nameMap.clear();
    std::map<std::string,unsigned> declarationLocationMap;
    for( llvm::const_inst_iterator iter(f),end(f,true); iter != end; ++iter ) {
      const llvm::Instruction* I = &*iter;
      const llvm::Value* var = NULL;
      llvm::MDNode* md = NULL;
      //llvm::MDNode* iDbg = NULL;
      std::string str;
      unsigned lineNum = 0;
      const llvm::DebugLoc& dbgLoc = I->getDebugLoc();
      if( const llvm::DbgDeclareInst* dDecl =
          llvm::dyn_cast<llvm::DbgDeclareInst>(I) ) {
        // cfrontend_error( "buildLocalNameMap should be called only after"
        //                  << " PromoteMemoryToRegister pass" );
        var = dDecl->getAddress();
        md = dDecl->getVariable();
        llvm::DIVariable diMd(md);
        str = (std::string)( diMd.getName() );
        lineNum = diMd.getLineNumber();
      } else if( const llvm::DbgValueInst* dVal =
                 llvm::dyn_cast<llvm::DbgValueInst>(I)) {
        var = dVal->getValue();
        md = dVal->getVariable();
        llvm::DIVariable diMd(md);
        str = (std::string)( diMd.getName() );
        lineNum = diMd.getLineNumber();
        //iDbg = I->getMetadata( "dbg" );
      }
      if( var ) {
        //to look at the scope field
        // check if there has been a declaration with same name with different
        // line number
        auto it = declarationLocationMap.find( str );
        if( it == declarationLocationMap.end() ) {
          declarationLocationMap[str] = lineNum;
          nameMap[var] = str;
        }else if( it->second == lineNum ) {
          nameMap[var] = str;
        }else{
          nameMap[var] = str + "_at_"+ std::to_string( lineNum );
        }
      }

      // if( var ) {
      //   nameMap[var] = str;
      //   llvm::DIVariable di(md);
      //   llvm::outs()
      //     << "local variable: " << str
      //     << " " << di.getLineNumber() << " " << di.getArgNumber();
      //   if(iDbg){
      //   // printMetaData( std::cout, iDbg );
      //     if( llvm::MDNode* md_i = llvm::dyn_cast<llvm::MDNode>( iDbg->getOperand(2) ) ) {
      //       llvm::DIDescriptor dbInfo(md_i);
      //       if( dbInfo.isLexicalBlock() ) {
      //         llvm::DILexicalBlock lBlock(md_i);
      //         llvm::outs() << "[lexical block] "
      //                      << lBlock.getLineNumber() << " "
      //                      << lBlock.getColumnNumber();
      //       }else{
      //         dbInfo.print( llvm::outs() );
      //       }
      //       // printMetaData( std::cout, md_i );
      //     }
      //   }
      //   std::cout << "\n";
      // }
    }
  }

  int readInt( const llvm::ConstantInt* c ) {
    const llvm::APInt& n = c->getUniqueInteger();
    unsigned len = n.getNumWords();
    if( len > 1 ) cfrontend_error( "long integers not supported!!" );
    const uint64_t *v = n.getRawData();
    return *v;
  }

  //--------------------------------------------------------------------------
  // printing metaData of our own choice

  void printMetaData( std::ostream& os, llvm::MDNode* md ) {
    os << "{";
    llvm::DIDescriptor dbInfo(md);

    unsigned numOps = md->getNumOperands();
    // llvm::Value* tagVal = md->getOperand(0);
    unsigned tag =  dbInfo.getTag();
    const char* tagName = llvm::dwarf::TagString( tag );
    if( tagName )
      os <<  tagName;
    else
      os << std::hex << tag << std::dec;
    //Todo: this is not working
    for( unsigned i = 1; i < numOps; i++ ) {
    //   llvm::Metadata& vi = *(md->getOperand(i));
      os << ",";
    //   if( vi == NULL ) {
    //     os << "NULL";
    //   }else if( llvm::MDNode* md_i = llvm::dyn_cast<llvm::MDNode>(vi ) ) {
    //     printMetaData( os, md_i );
    //   }else{
    //     os << (std::string)( md->getOperand(i)->getName() );
    //   }
    }
    os << "}";
  }

  // -------------------------------------------------------------------------
  // -- unused code; set for deletions --- copied from somewhere

  const llvm::Function* findEnclosingFunc(const llvm::Value* val) {
    if( const llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(val) ) {
      return arg->getParent();
    }
    if( const llvm::Instruction* I = llvm::dyn_cast<llvm::Instruction>(val) ) {
      return I->getParent()->getParent();
    }
    return NULL;
  }

  const llvm::MDNode* findVar(const llvm::Value* V, const llvm::Function* F) {
    for( llvm::const_inst_iterator iter = llvm::inst_begin(F),
           end = llvm::inst_end(F); iter != end; ++iter) {
      const llvm::Instruction* I = &*iter;
      if( const llvm::DbgDeclareInst* dDecl =
          llvm::dyn_cast<llvm::DbgDeclareInst>(I) ) {
        if( dDecl->getAddress() == V ) return dDecl->getVariable();
      } else if( const llvm::DbgValueInst* dVal =
                 llvm::dyn_cast<llvm::DbgValueInst>(I)) {
        if (dVal->getValue() == V) return dVal->getVariable();
      }
    }
    return NULL;
  }

  llvm::StringRef getOriginalName(const llvm::Value* val) {
    // TODO handle globals as well

    const llvm::Function* f = findEnclosingFunc(val);
    if (!f) return val->getName();

    const llvm::MDNode* var = findVar(val, f);
    if (!var) return "tmp";

    return llvm::DIVariable(var).getName();
  }

}
