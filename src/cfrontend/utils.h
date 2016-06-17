#ifndef CFRONTEND_UTILS_H
#define CFRONTEND_UTILS_H

#include <cassert>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <algorithm>

#ifdef EXTERNAL_VERSION
#define triggered_at ""
#else
#define triggered_at " (triggered at " <<  __FILE__ << ", " << __LINE__ << ")"
#endif

#ifdef DEBUG
#define issue_error  assert( false );
#else
#define issue_error  std::exit( 1 );
#endif

namespace cfrontend{

#define cfrontend_error( S ) { std::cerr << "# cfrontend Error: " << S \
                                         << triggered_at << std::endl; \
                               issue_error }

#define cfrontend_error2( S, T )    { cfrontend_error( S << T ) }
#define cfrontend_error3( S, T1, T2 )    { cfrontend_error( S << T1 << T2 ) }

#define cfrontend_warning( S ) { std::cerr << "# cfrontend Warning: " << S \
                                           << std::endl; }
#define cfrontend_warning2( S, T ) { cfrontend_warning2( S <<  T ) }

#define cfrontend_print( os, S ) { os << ";; "<< S << std::endl; }
#define cfrontend_print2( os, S, T){ cfrontend_print( os, S << T ) }

#define COMPARE_TAIL( A, B, Tail ) ( A < B || ( A == B && ( Tail ) ) )
#define COMPARE_LAST( A, B )       ( A < B )

#define COMPARE_OBJ1( A, B, F ) COMPARE_LAST( A.F, B.F )
#define COMPARE_OBJ2( A, B, F1, F2 ) COMPARE_TAIL( A.F1, B.F1, \
        COMPARE_OBJ1( A, B, F2 ) )
#define COMPARE_OBJ3( A, B, F1, F2, F3 ) COMPARE_TAIL( A.F1, B.F1, \
        COMPARE_OBJ2( A, B, F2, F3 ) )
#define COMPARE_OBJ4( A, B, F1, F2, F3, F4 ) COMPARE_TAIL( A.F1, B.F1, \
        COMPARE_OBJ3( A, B, F2, F3, F4 ) )

#define COMPARE_SELF1( B, F ) COMPARE_LAST( F, B.F )
#define COMPARE_SELF2( B, F1, F2 ) COMPARE_TAIL( F1, B.F1, \
        COMPARE_SELF1( B, F2 ) )
#define COMPARE_SELF3( B, F1, F2, F3 ) COMPARE_TAIL( F1, B.F1, \
        COMPARE_SELF2( B, F2, F3 ) )
#define COMPARE_SELF4( B, F1, F2, F3, F4 ) COMPARE_TAIL( F1, B.F1, \
        COMPARE_SELF3( B, F2, F3, F4 ) )

#define LLCAST(A,B,C) const llvm::A* B = llvm::dyn_cast<llvm::A>(C)

template <class T>
inline bool exists( std::vector<T>& vec, T item ) {
  return std::find( vec.begin(), vec.end(), item ) != vec.end();
}

  inline void print_line( std::ostream& os, char ch, unsigned len ) {
    assert( len  <=  80 );
    for( unsigned i = 0; i < len; i++ ) {
      os << ch;
    }
    os << "\n";
  }

  inline void print_line( std::ostream& os, char ch ) {
    print_line( os, ch, 50 );
  }

  inline void print_line( std::ostream& os ) {
    print_line( os, '-', 50 );
  }

  inline void print_double_line( std::ostream& os ) {
    print_line( os, '=', 50 );
  }

  typedef std::vector<std::string> StringVec;
// template <class T1,class T2>
// inline bool exists( std::map<T1,T2>& m, T1 item ) {
//   return m.find( item ) != m.end();
// }

}
#endif
