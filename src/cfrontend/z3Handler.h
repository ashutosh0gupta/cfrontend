#ifndef CFRONTEND_Z3HANDLER_H
#define CFRONTEND_Z3HANDLER_H

// need to include relevent llvm libraries

#include "cfrontend/utils.h"

namespace cfrontend{
  class Z3ExprHandler{
  public:
    typedef z3::expr expr;
    Z3ExprHandler( z3::context& ctx_ ) : ctx( ctx_ ) {}

    expr mkEmptyExpr() { return expr(ctx); }

    bool isEq( expr& a, expr&b ) {
      return z3::eq(a,b);
      // Z3_ast ap = a;
      // Z3_ast bp = b;
      // return ap == bp;
    }
    //------------------------------------------------------------------------
    // mk expresssions layer
    expr mkIntVal( int n ) { return ctx.int_val(n); }

    expr mkTrue( ) { return ctx.bool_val( true ); }
    expr mkVar( std::string& name ) { return ctx.int_const( name.c_str() ); }

    expr mkNot  ( expr& a ) { return !a; }

    expr mkAnd  ( expr& a, expr& b ) { return a && b; }
    expr mkOr   ( expr& a, expr& b ) { return a || b; }
    // expr mkImplies( expr& a, expr& b ) { return z3::implies( a, b ); }
    expr mkEq   ( expr& a, expr& b ) { return a == b; }
    expr mkNEq  ( expr& a, expr& b ) { return a != b; }
    expr mkGT   ( expr& a, expr& b ) { return a >  b; }
    expr mkGEq  ( expr& a, expr& b ) { return a >= b; }
    expr mkLT   ( expr& a, expr& b ) { return a <  b; }
    expr mkLEq  ( expr& a, expr& b ) { return a <= b; }
    expr mkPlus ( expr& a, expr& b ) { return a + b;  }
    expr mkMinus( expr& a, expr& b ) { return a - b;  }
    expr mkMul  ( expr& a, expr& b ) { return a * b;  }

    // Bit vector operations
    expr mkXor  ( expr& a, expr& b ) { return a ^ b;  }

    expr mkAnd  ( std::vector<expr>& as ) {
      if( as.size() == 0 ){
        return mkTrue();
      }else {
        expr rval = as[0];
        for( unsigned i = 1; i < as.size(); i++ ) {
          rval = mkAnd( rval, as[i] );
        }
        return rval;
      }
    }

    //------------------------------------------------------------------------

    void addGlobalVar( llvm::GlobalVariable* g, expr& eGlb, expr& eGlbNext ) {
      // std::cout << "Adding global variable" << g << "\n";
      auto p = std::pair<llvm::GlobalVariable*,expr>( g, eGlb );
      glbVars.insert( p );
      auto pNext = std::pair<llvm::GlobalVariable*,expr>( g, eGlbNext );
      glbVarsNext.insert( pNext );
    }

    expr getGlobalVar( const llvm::GlobalVariable* g  ) {
      auto it = glbVars.find( g );
      if( it == glbVars.end() )
        cfrontend_error( "global variable not found during translation!" );
      return it->second;
    }

    expr getGlobalVar( const llvm::Value* g  ) {
      if( const llvm::GlobalVariable* gv =
          llvm::dyn_cast<const llvm::GlobalVariable>(g) ) {
        return getGlobalVar( gv );
      }
      cfrontend_error( "global variable not found during translation!" );
      return mkEmptyExpr();
    }

    expr getGlobalVarNext( const llvm::GlobalVariable* g  ) {
      auto it = glbVarsNext.find( g );
      if( it == glbVarsNext.end() )
        cfrontend_error( "global variable not found during translation!" );
      return it->second;
    }

    expr getGlobalVarNext( const llvm::Value* g  ) {
      if( const llvm::GlobalVariable* gv =
          llvm::dyn_cast<const llvm::GlobalVariable>(g) ) {
        return getGlobalVarNext( gv );
      }
      cfrontend_error( "global variable not found during translation!" );
      return mkEmptyExpr();
    }


    void insertMap( const llvm::Value* g, expr& term,
                    std::map<  const llvm::Value*, expr >& map ) {
      // std::cerr << "inserting value map " << g << " " << term << "\n";
      auto p = std::pair<const llvm::Value*,expr>( g, term );
      map.insert( p );
    }

    void resetLocalVars() {
      localVars.clear();
      localVarsNext.clear();
    }

    void addLocalVar( const llvm::Value* g, expr& eLocal, expr& eLocalNext ) {
      auto p = std::pair<const llvm::Value*,expr>( g, eLocal );
      localVars.insert( p );
      auto pNext = std::pair<const llvm::Value*,expr>( g, eLocalNext );
      localVarsNext.insert( pNext );
    }

    bool isLocalVar( const llvm::Value* g  ) {
      auto it = localVars.find( g );
      if( it == localVars.end() ) return false;
      return true;
    }

    expr getLocalVar( const llvm::Value* g  ) {
      auto it = localVars.find( g );
      if( it == localVars.end() )
        cfrontend_error( "a local variable not found!" );
      return it->second;
    }

    expr getLocalVarNext( const llvm::Value* g  ) {
      auto it = localVarsNext.find( g );
      if( it == localVarsNext.end() )
        cfrontend_error( "a local variable not found!" );
      return it->second;
    }

  private:
    z3::context& ctx;
    std::map< const llvm::GlobalVariable*, expr > glbVars;
    std::map< const llvm::GlobalVariable*, expr > glbVarsNext;

    std::map< const llvm::Value*, expr > localVars;
    std::map< const llvm::Value*, expr > localVarsNext;

  };
}

#endif
