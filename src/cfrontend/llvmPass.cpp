#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// pragam'ed to aviod warnings due to llvm included files

#include "llvm/IRReader/IRReader.h"

#include "llvm/IR/InstIterator.h"

#include "llvm/LinkAllPasses.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/IR/DebugInfo.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"

// #include "llvm/IR/InstrTypes.h"
// #include "llvm/IR/BasicBlock.h"

#include "llvm/IRReader/IRReader.h"
// #include "llvm/Support/Debug.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Dwarf.h"

#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/ADT/StringMap.h"


#pragma GCC diagnostic pop

// #undef DEBUG
//----------------------------------------------------------------------------


#include "cfrontend/program.h"
#include "cfrontend/cfrontend.h"
#include "cfrontend/llvmUtils.h"
#include "cfrontend/llvmPass.h"
#include "cfrontend/z3Handler.h"

#define UNSUPPORTED_INSTRUCTIONS( InstTYPE, Inst )                       \
  if( const llvm::InstTYPE* t = llvm::dyn_cast<llvm::InstTYPE>(Inst) ) { \
     cfrontend_error( "Unsupported instruction!!");                      \
  }


namespace cfrontend{


  template <typename EHandler>
  void
  BuildSMTProgram<EHandler>::
  resetPendingInsertEdge() {
    pendingSrc.clear();
    pendingDst.clear();
    pendingTerms.clear();
    // cfrontend_warning( "this function in not implemented yet!!" );
      return;
  }


  template <typename EHandler>
  void
  BuildSMTProgram<EHandler>::
  addPendingInsertEdge( unsigned src, unsigned dst,
                             typename EHandler::expr e ) {
    pendingSrc.push_back( src );
    pendingDst.push_back( dst );
    pendingTerms.push_back( e );
    // cfrontend_warning( "this function in not implemented yet!!" );
      return;
  }

  template <typename EHandler>
  void
  BuildSMTProgram<EHandler>::
  applyPendingInsertEdges( unsigned tid ) {
    unsigned pendingLength = pendingSrc.size();
    std::vector<bool> doneIdxs;
    doneIdxs.resize( pendingLength, false );

    unsigned i = 0;
    while( i < pendingLength ) {
      if( doneIdxs[i] ) { i++; continue; }
      unsigned src = pendingSrc[i];
      unsigned dst = pendingDst[i];
      typename EHandler::expr blockTerm = pendingTerms[i];
      doneIdxs[i] = true; // this write is not necessory  --??--
      for( unsigned j = i+1; j < pendingLength; j++ ) {
        if( pendingSrc[j] == src && pendingDst[j] == dst ) {
          doneIdxs[j] = true;
          blockTerm = eHandler->mkAnd( blockTerm, pendingTerms[j] );
        }
      }
      program->insertAfterBlock( tid, src, dst,  blockTerm );
      i++;
    }
  }

  //--------------------------------------------------------------------------

  template <typename EHandler>
  typename EHandler::expr
  BuildSMTProgram<EHandler>::
  getPhiMap( const llvm::PHINode* p, ValueExprMap& m ) {
    static std::vector< const llvm::PHINode* > visited;
    if( exists( visited, p) ) return getTerm( p, m);
    visited.push_back( p );
    // ValueVec infoLessInstructions, infoWithInstructions;
    // std::vector< std::pair<const llvm::PHINode*,unsigned> > nonPHIEndngs;
    // ValueVec unrecognizedInstructions;
    // Traverse graph generated by phi instructions
    // infoLessInstructions     : values with no debug info
    // infoWithInstructions     : values with debug info
    // phiWithConsts            : nodes that are constants
    // unrecognizedInstructions : no debug info not phi

    std::vector< const llvm::PHINode* > visitedPhis;
    std::vector< std::pair<const llvm::PHINode*,unsigned> > endings;

    findPhiTrail( p, visitedPhis, endings );

    typename EHandler::expr x  = eHandler->mkEmptyExpr();
    typename EHandler::expr xp = eHandler->mkEmptyExpr();

    if( hasDebugInfo( llvm::dyn_cast<llvm::Instruction>(p) ) ) {
      x  = getTerm( p, m);
      xp = eHandler->getLocalVarNext( p );
    }else{
      std::vector<typename EHandler::expr> eList;
      bool findXp = true;
      for( auto v : endings ) {
        const llvm::PHINode* p = v.first; unsigned i = v.second;
        llvm::Value* c   = p->getIncomingValue( i );
        if( !llvm::isa<llvm::Constant>(c) ) {
          if( isValueMapped( c, m ) ) {
            auto term = getTerm( c, m );
            eList.push_back( term );
          }
          if(findXp && hasDebugInfo( llvm::dyn_cast<llvm::Instruction>(c) ) ) {
            findXp = false;
            xp = eHandler->getLocalVarNext(llvm::dyn_cast<llvm::Instruction>(c));
          }
        }//else{ cfrontend_error( "undebugged and unmapped value found!!" ); }
      }
      if( findXp )
        cfrontend_error( "next value xp not found!!" );

      unsigned i = 1;
      for(; i < eList.size(); i++ ) {
        if( !eHandler->isEq( eList[ i - 1 ], eList[ i ] ) ) break;
      }
      if( i != eList.size() ) // if not all terms in eList are equal
        cfrontend_error( "get phi map : no unique debug info!" );

      x  = eList[0];

      for( const llvm::Value* v : visitedPhis ) {
        if( !isValueMapped( v, m ) ) { //Value may be mapped in m??
          eHandler->insertMap( v, x, m);
        }else{
          auto a = getTerm( v, m );
          if( !eHandler->isEq( a, x ) )
            cfrontend_error( "why this is not error!!" );
        }
      }
    }

    for( auto v : endings ) {
      const llvm::PHINode* p = v.first; unsigned i = v.second;
      llvm::Value* c   = p->getIncomingValue( i );
      unsigned src = getBlockCount( p->getIncomingBlock(i) );
      unsigned dst = getBlockCount( p->getParent()         );
      if( isValueMapped( c, m ) ) {
        auto term = getTerm( c, m );
        if( !eHandler->isEq( x, term ) ) {
          addPendingInsertEdge( src, dst, xp == term );
        }
      }else{ cfrontend_error( "undebugged and unmapped value found!!" ); }
    }

    return x;

    // if( infoWithInstructions.size() == 0  &&
    //     unrecognizedInstructions.size() == 0 ) {
    //     cfrontend_error( "no debug information found!!" );
    // }

    // // construct eList = m[infoWithInstructions]
    // std::vector<typename EHandler::expr> eList;
    // for( const llvm::Value* v : infoWithInstructions ) {
    //   eList.push_back( getTerm( v, m ) );
    // }

    // if( hasBoolType(p) ) {
    //   // cfrontend_error( "unspported!!");
    //   // cfrontend_warning( "boolean type found!" );
    //   std::vector<typename EHandler::expr> unrecogList;
    //   for( const llvm::Value* v : unrecognizedInstructions ) {
    //     if( isValueMapped( v, m ) ) {
    //       unrecogList.push_back( getTerm( v, m ) );
    //     }else{
    //       cfrontend_error( "undebugged and unmapped value found!!" );
    //     }
    //   }
    // }
    // else
    // {
    //   // check if all terms in eList are equal
    //   if( unrecognizedInstructions.size() != 0 ) {
    //     cfrontend_error( "no debug information found!!" );
    //   }

    //   unsigned i = 1;
    //   for(; i < eList.size(); i++ ) {
    //     if( !eHandler->isEq( eList[ i - 1 ], eList[ i ] ) ) break;
    //   }
    //   if( i != eList.size() ) // if not all terms in eList are equal
    //     cfrontend_error( "get phi map : no unique debug info!" );
    //   typename EHandler::expr e = eList[0];

    //   for( const llvm::Value* v : infoLessInstructions ) {
    //     if( !isValueMapped( v, m ) ) { //Value may be mapped in m??
    //       eHandler->insertMap( v, e, m);
    //     }else{
    //       auto a = getTerm( v, m );
    //       if( !eHandler->isEq( a, e ) )
    //         cfrontend_error( "why this is not error!!" );
    //     }
    //   }

    //   for( auto v : phiWithConsts ) {
    //     const llvm::PHINode* p = v.first;
    //     unsigned i = v.second;
    //     llvm::Value* c   = p->getIncomingValue( i );
    //     const llvm::Value* x   = infoWithInstructions[0];
    //     //TODO:
    //     llvm::BasicBlock *const_block = p->getIncomingBlock(i);
    //     unsigned src = getBlockCount( const_block );
    //     const llvm::BasicBlock* b = p->getParent();
    //     unsigned dst = getBlockCount( b );
    //     addPendingInsertEdge( src, dst,
    //                           eHandler->getLocalVarNext( x ) == getTerm( c, m )
    //                           );
    //   }
    //   return e;
    // }
  }

  template <class ExprHandler>
  char BuildSMTProgram<ExprHandler>::ID = 0;

  template <typename EHandler>
  typename BuildSMTProgram<EHandler>::program_location_id_t
  BuildSMTProgram<EHandler>::
  getBlockCount( const llvm::BasicBlock* b  ) {
    auto it_num = numBlocks.find( b );
    if( it_num == numBlocks.end() ) {
      cfrontend_error( "block was not found in earlier pass!!");
    }else{
      return it_num->second;
    }
    return 0;
  }

  template <typename EHandler>
  typename EHandler::expr
  BuildSMTProgram<EHandler>::
  translateBlock( llvm::BasicBlock* b, unsigned succ, ValueExprMap& m ) {
    assert(b);
    std::vector<typename EHandler::expr> blockTerms;
    //iterate over instructions
    for( llvm::Instruction& Iobj : b->getInstList() ) {
      llvm::Instruction* I = &(Iobj);
      assert( I );
      typename EHandler::expr term = eHandler->mkEmptyExpr();
      bool recognized = false, record = false;
      if( const llvm::StoreInst* str = llvm::dyn_cast<llvm::StoreInst>(I) ) {
        llvm::Value* v = str->getOperand(0);
        llvm::Value* g = str->getOperand(1);
        term = getTerm( v, m );
        typename EHandler::expr gp     = eHandler->getGlobalVarNext( g );
        typename EHandler::expr assign = eHandler->mkEq( gp, term );
        blockTerms.push_back( assign );
        assert( !recognized );recognized = true;
      }
      if( const llvm::BinaryOperator* bop =
          llvm::dyn_cast<llvm::BinaryOperator>(I) ) {
        llvm::Value* op0 = bop->getOperand( 0 );
        llvm::Value* op1 = bop->getOperand( 1 );
        typename EHandler::expr o0 = getTerm( op0, m );
        typename EHandler::expr o1 = getTerm( op1, m );
        unsigned op = bop->getOpcode();
        switch( op ) {
        case llvm::Instruction::Add : term = eHandler->mkPlus ( o0, o1 ); break;
        case llvm::Instruction::Sub : term = eHandler->mkMinus( o0, o1 ); break;
        case llvm::Instruction::Mul : term = eHandler->mkMul  ( o0, o1 ); break;
        case llvm::Instruction::Xor : term = eHandler->mkXor  ( o0, o1 ); break;
        // case llvm::Instruction::SDiv: term = eHandler->mkSDiv ( o0, o1 ); break;
        default: {
          const char* opName = bop->getOpcodeName();
          cfrontend_error( "unsupported instruction " << opName
                           << " occurred!!"  );
        }
        }
        record = true;
        assert( !recognized );recognized = true;
      }
      if( const llvm::UnaryInstruction* str =
          llvm::dyn_cast<llvm::UnaryInstruction>(I) ) {
        if( const llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(I) ) {
          llvm::Value* g = load->getOperand(0);
          term = eHandler->getGlobalVar(g);
          record = true;
          assert( !recognized );recognized = true;
        }else{
          assert( false );
          cfrontend_warning( "I am uniary instruction!!" );
          assert( !recognized );recognized = true;
        }
      }
      if( const llvm::CmpInst* cmp = llvm::dyn_cast<llvm::CmpInst>(I) ) {
        llvm::Value* lhs = cmp->getOperand( 0 );
        llvm::Value* rhs = cmp->getOperand( 1 );
        typename EHandler::expr l = getTerm( lhs, m );
        typename EHandler::expr r = getTerm( rhs, m );
        llvm::CmpInst::Predicate pred = cmp->getPredicate();
        switch( pred ) {
        case llvm::CmpInst::ICMP_EQ  : term = eHandler->mkEq ( l, r ); break;
        case llvm::CmpInst::ICMP_NE  : term = eHandler->mkNEq( l, r ); break;
        case llvm::CmpInst::ICMP_UGT : term = eHandler->mkGT ( l, r ); break;
        case llvm::CmpInst::ICMP_UGE : term = eHandler->mkGEq( l, r ); break;
        case llvm::CmpInst::ICMP_ULT : term = eHandler->mkLT ( l, r ); break;
        case llvm::CmpInst::ICMP_ULE : term = eHandler->mkLEq( l, r ); break;
        case llvm::CmpInst::ICMP_SGT : term = eHandler->mkGT ( l, r ); break;
        case llvm::CmpInst::ICMP_SGE : term = eHandler->mkGEq( l, r ); break;
        case llvm::CmpInst::ICMP_SLT : term = eHandler->mkLT ( l, r ); break;
        case llvm::CmpInst::ICMP_SLE : term = eHandler->mkLEq( l, r ); break;
        default: {
          cfrontend_error( "unsupported predicate in compare instruction"
                           << pred << "!!"  );
        }
        }
        record = true;
        assert( !recognized );recognized = true;
      }
      if( const llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(I) ) {
        term = getPhiMap( phi, m);
        record = 1;
        assert( !recognized );recognized = true;
      }
      if( auto ret = llvm::dyn_cast<llvm::ReturnInst>(I) ) {
        llvm::Value* v = ret->getReturnValue();
        if( v ) {
          typename EHandler::expr retTerm = getTerm( v, m );
          //todo: use retTerm somewhere
        }
        if( config.verbose("mkthread") )
          cfrontend_warning( "return value ignored!!" );
        assert( !recognized );recognized = true;
      }
      if( auto unreach = llvm::dyn_cast<llvm::UnreachableInst>(I) ) {
        if( config.verbose("mkthread") )
          cfrontend_warning( "unreachable instruction ignored!!" );
        assert( !recognized );recognized = true;
      }

      // UNSUPPORTED_INSTRUCTIONS( ReturnInst,      I );
      UNSUPPORTED_INSTRUCTIONS( InvokeInst,      I );
      UNSUPPORTED_INSTRUCTIONS( IndirectBrInst,  I );
      // UNSUPPORTED_INSTRUCTIONS( UnreachableInst, I );

      if( const llvm::BranchInst* br = llvm::dyn_cast<llvm::BranchInst>(I) ) {
        if( br->isConditional() ) {
          llvm::Value* c = br->getCondition();
          typename EHandler::expr cond = getTerm( c, m);
          if( succ == 0 ) {
            blockTerms.push_back( cond );
          }else{
            typename EHandler::expr notCond = eHandler->mkNot( cond );
            blockTerms.push_back( notCond );
          }
        }else{cfrontend_warning( "I am unconditional branch!!" );}
        assert( !recognized );recognized = true;
      }
      if( const llvm::SwitchInst* t = llvm::dyn_cast<llvm::SwitchInst>(I) ) {
        cfrontend_warning( "I am switch!!" );
        assert( !recognized );recognized = true;
      }
      if( const llvm::CallInst* str = llvm::dyn_cast<llvm::CallInst>(I) ) {
        if( const llvm::DbgValueInst* dVal =
            llvm::dyn_cast<llvm::DbgValueInst>(I) ) {
          // Ignore debug instructions
        }else{ cfrontend_warning( "I am caller!!" ); }
        assert( !recognized );recognized = true;
      }
      // Store in term map the result of current instruction
      if( record ) {
        if( eHandler->isLocalVar( I ) ) {
          typename EHandler::expr lp     = eHandler->getLocalVarNext( I );
          typename EHandler::expr assign = eHandler->mkEq( lp, term );
          blockTerms.push_back( assign );
        }else{
          const llvm::Value* v = I;
          eHandler->insertMap( v, term, m);
        }
      }
      if( !recognized ) cfrontend_error( "----- failed to recognize!!" );
    }
    // print_double_line( std::cout );
    //iterate over instructions and build
    return eHandler->mkAnd( blockTerms );
  }

  template <typename EHandler>
  void BuildSMTProgram<EHandler>::initBlockCount( llvm::Function &f,
                                                  size_t threadId ) {
    numBlocks.clear();
    auto it  = f.begin();
    auto end = f.end();
    for( ; it != end; it++ ) {
      llvm::BasicBlock* b = &(*it);
      auto new_l = program->createFreshLocation(threadId);
      numBlocks[b] = new_l;
      if( b->getName() == "err" ) {
        program->setErrorLocation( threadId, new_l );
      }
    }
    llvm::BasicBlock& b  = f.getEntryBlock();
    unsigned entryLoc = getBlockCount( &b );
    program->setStartLocation( threadId, entryLoc );
    auto finalLoc = program->createFreshLocation(threadId);
    program->setFinalLocation( threadId, finalLoc );
  }


  template <typename EHandler>
  bool BuildSMTProgram<EHandler>::runOnFunction( llvm::Function &f ) {
    threadCount++;
    std::string name = (std::string)f.getName();
    thread_id_t threadId = program->addThread( name );
    initBlockCount( f, threadId );

    // collect local variables

    //iterate over debug instructions

    // TODO: remove nameExprMap from to avoid ugliness of accessing
    // maps with z3 expressions
    std::map< std::string, typename EHandler::expr> nameExprMap;
    std::map< std::string, typename EHandler::expr> nameNextExprMap;
    std::map<const llvm::Value*, std::string> localNameMap;
    localNameMap.clear();
    buildLocalNameMap( f, localNameMap );
    eHandler->resetLocalVars();
    for(auto it=localNameMap.begin(), end = localNameMap.end(); it!=end;++it) {
      const llvm::Value* v = it->first;
      std::string name = it->second;
      std::string nameNext = name + "__p";
      if( config.verbose("mkthread") ) {
        std::cout << "local variable discovered: "<< name << "\n";
      }
      if( nameExprMap.find(name) == nameExprMap.end() ) {
        typename EHandler::expr lv  = eHandler->mkVar( name  );
        typename EHandler::expr lvp = eHandler->mkVar( nameNext );
        program->addLocal( threadId, lv, lvp );
        // TODO: why the following code does not work with z3 expr
        auto p = std::pair<std::string,typename EHandler::expr>( name, lv );
        nameExprMap.insert(p); // nameExprMap[name] = lv;
        auto n = std::pair<std::string,typename EHandler::expr>(nameNext,lvp);
        nameNextExprMap.insert(n); // nameNextExprMap[nameNext] = lvNext;
      }
      auto curr_it = nameExprMap.find(name);
      auto next_it = nameNextExprMap.find(nameNext);
      eHandler->addLocalVar( v, curr_it->second, next_it->second );
    }

    ValueExprMap exprMap;

    resetPendingInsertEdge();

    for( auto it = f.begin(), end = f.end(); it != end; it++ ) {
      llvm::BasicBlock* src = &(*it);
      if( config.verbose("mkthread") ) src->print( llvm::outs() );
      unsigned srcLoc = getBlockCount( src );
      llvm::TerminatorInst* c= (*it).getTerminator();
      unsigned num = c->getNumSuccessors();
      if( num == 0 ) {
        typename EHandler::expr e = translateBlock( src, 0, exprMap );
        auto dstLoc = program->getFinalLocation( threadId );
        program->addBlock( threadId, srcLoc, dstLoc, e );
      }else{
        for( unsigned i=0; i < num; ++i ) {
          // ith branch may be tranlate differently
          typename EHandler::expr e = translateBlock( src, i, exprMap );
          llvm::BasicBlock* dst = c->getSuccessor(i);
          unsigned dstLoc = getBlockCount( dst );
          program->addBlock( threadId, srcLoc, dstLoc, e );
        }
      }
    }
    applyPendingInsertEdges( threadId );

    return false;
  }

  //--------------------------------------------------------------------------
  template <class ExprHandler>
  SimpleMultiThreadedProgram<typename ExprHandler::expr>*
  Cfrontend::pass( ExprHandler* eHandler, std::string& filename ) {

    std::unique_ptr<llvm::Module> module;
    llvm::SMDiagnostic err;
    llvm::LLVMContext& context = llvm::getGlobalContext();

    module = llvm::parseIRFile( filename, err, context);
    if( module.get() == 0 ) {
      config.out() << "Bitcode parsing failed!!";
    }
    typename ExprHandler::expr e_true = eHandler->mkTrue();

    auto program =
      new SimpleMultiThreadedProgram<typename ExprHandler::expr>(e_true);

    llvm::PointerType* iptr = llvm::Type::getInt32PtrTy( context );

    for( auto iter_glb= module->global_begin(),end_glb = module->global_end();
         iter_glb != end_glb; ++iter_glb ) {
      llvm::GlobalVariable* glb = iter_glb;
      if( glb->getType() != iptr ) {
        cfrontend_error((std::string)(glb->getName()) << " is not int32 type!");
      }
      std::string gvar = (std::string)(glb->getName());
      std::string gvarp = gvar + "__p";
      typename ExprHandler::expr v  = eHandler->mkVar( gvar  );
      typename ExprHandler::expr vp = eHandler->mkVar( gvarp );
      program->addGlobal( v, vp );
      eHandler->addGlobalVar( glb, v, vp ); // for efficient access
    }

    llvm::PassManager passMan;
    llvm::PassRegistry& reg = *llvm::PassRegistry::getPassRegistry();
    llvm::initializeAnalysis(reg);
    llvm::initializeDependenceAnalysisPass(reg);
    // what is this datalayout thing? is it important??
    // llvm::DataLayout *dl = NULL;
    // const std::string& moduleDataLayout = module.get()->getDataLayout();
    // if( !moduleDataLayout.empty() )
    //   dl = new llvm::DataLayout( moduleDataLayout );
    // if( dl ) passMan.add( dl );
    // passMan.add( llvm::createInternalizePass() );

    llvm::FunctionPass* depPass = llvm::createDependenceAnalysisPass();
    llvm::DependenceAnalysis* dPass = (llvm::DependenceAnalysis*)(depPass);

    passMan.add( llvm::createPromoteMemoryToRegisterPass() );
    passMan.add( new SplitAtAssumePass() );
    if( config.isOutLLVMcfg() )
      passMan.add( llvm::createCFGPrinterPass() );
    passMan.add( depPass );
    passMan.add( new BuildSMTProgram<ExprHandler>( eHandler, config, program ) );
    passMan.run( *module.get() );
    // if( const llvm::DependenceAnalysis* dPass = llvm::dyn_cast<llvm::DependenceAnalysis>(depPass) )
    auto& OS = llvm::outs();
    for( auto it= module->begin(),end_it = module->end(); it != end_it; ++it ) {
      llvm::Function* f = it;
      for( auto SI = inst_begin(f), SE = inst_end(f);SI != SE; ++SI){
        if(llvm::isa<llvm::StoreInst>(*SI) || llvm::isa<llvm::LoadInst>(*SI)) {
          for( auto DI = SI, DE = inst_end(f); DI != DE; ++DI ) {
            if(llvm::isa<llvm::StoreInst>(*DI)||llvm::isa<llvm::LoadInst>(*DI)){
              if( auto D = dPass->depends(&*SI, &*DI, true) ) {
                D->dump( llvm::outs() );
                for (unsigned l = 1; l <= D->getLevels(); l++) {
                  if( D->isSplitable(l) ) {
                    OS << "da analyze - split level = " << l
                       << ", iteration = " << *dPass->getSplitIteration(*D, l)
                       << "!\n";
                  }
                }
              }
              else
                OS << "none!\n";
            }
          }
        }
      }
    }
    return program;
  }

#ifdef Z3_PATH // checks availability of z3
  SimpleMultiThreadedProgram<z3::expr>*
  Cfrontend::z3Pass( z3::context& ctx ,std::string& filename ) {
    Z3ExprHandler* handler = new Z3ExprHandler( ctx );
    auto program = pass( handler, filename );
    delete handler; // needed to be deleted
    return program;
  }
#endif

}


//----------------------------------------------------------------------------

/*
{,
  {,{../ex-c/ex1.c,/root/research/mycode/cfrontend/build},
   {,{../ex-c/ex1.c,/root/research/mycode/cfrontend/build},
    {,{../ex-c/ex1.c,/root/research/mycode/cfrontend/build}},
    foo,
    foo,,,
    {,,NULL,,,,,,,NULL,
        {{,NULL,NULL,int,,,,,,},
         {,NULL,NULL,int,,,,,,},
         {,NULL,NULL,int,,,,,,}},
        ,NULL,NULL,NULL}
    ,,,,,NULL,,,foo,NULL,NULL,{},}
   ,,,},
 i,
 {,{../ex-c/ex1.c,/root/research/mycode/cfrontend/build}},
 ,
 {,NULL,NULL,unsigned int,,,,,,}
 ,
 ,
}
*/
