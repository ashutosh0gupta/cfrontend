#ifndef CFRONTEND_LLVMUTILS_H
#define CFRONTEND_LLVMUTILS_H

#include "llvm/IR/Instructions.h"
// #include "llvm/IR/Function.h"

namespace cfrontend{

  typedef std::vector<const llvm::Value*> ValueVec;

  bool hasBoolType(const llvm::Value* );
  bool hasDebugInfo( const llvm::Instruction* );

  void removeBranchingOnPHINode( llvm::BranchInst * );

  //--------------------
  // access DEBUG information to get the original names of ssa variables
  void buildLocalNameMap( const llvm::Function&,
                          std::map<const llvm::Value*, std::string>& );
  int readInt( const llvm::ConstantInt* );

  // should not be visible to other files. TODO: should it be removed??
  void printMetaData( std::ostream& os, llvm::MDNode* md );


  // Backword travergal of dependency graph over Phi nodes
  void findPhiTrail( const llvm::PHINode*,
                     std::vector< const llvm::PHINode* >&,
                     std::vector< std::pair<const llvm::PHINode*,unsigned> >&);

  void findPhiTrail( const llvm::Value*,
                     ValueVec&, ValueVec&,
                     std::vector< std::pair<const llvm::PHINode*,unsigned> >&,
                     ValueVec& );

}

#endif
