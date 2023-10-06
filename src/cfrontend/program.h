//---------------------------------------------
// program object returned after analyzing llvm
// we wish to avoid a complex data structure
// comming out of llvm

#ifndef CFRONTEND_PROGRAM_H
#define CFRONTEND_PROGRAM_H

#include "cfrontend/utils.h"

#include <set>
#include <vector>
#include <stack>

namespace cfrontend {

// #########################################################################
// SimpleMultiThreadedProgram
template <class expr>
class SimpleMultiThreadedProgram
{
public:
  class Thread;
  class Location;
  class Block;

  typedef std::pair<expr,expr> Var;
  typedef std::vector<Var> Vars;

  typedef typename std::vector<Thread*>::size_type size_type;
  typedef size_type thread_id_type;

  typedef typename std::vector<Location*>::size_type location_id_type;
  typedef typename std::vector<Block*>::size_type block_id_type;

  SimpleMultiThreadedProgram (size_type size_, expr& init_)
    : threads(size_), init(init_) {
    for (auto i = 0; i < size_; ++i) {
      threads[i] = new Thread(i, globals );
    }
  }

  SimpleMultiThreadedProgram (expr& init)
    : init(init) {}

  ~SimpleMultiThreadedProgram () {
    for (auto thr : threads) {
      for(auto loc : thr->locations) {
        delete loc;
      }

      for(auto blk : thr->blocks) {
        delete blk;
      }

      delete thr;
    }
  }

  // -----------------------------------------------------------------------
  // Program construction functions

  thread_id_type addThread() {
    threads.push_back( new Thread( size(), globals) );
    return threads.size()-1;
  }

  thread_id_type addThread( std::string& name ) {
    threads.push_back( new Thread( size(), globals, name ) );
    return threads.size()-1;
  }

  // the second parameter represents the next value of the vaiable
  void addGlobal (expr& e, expr& ep) {
    assert( e != ep );
    assert( e.is_app() && ep.is_app() );
    // the following check does not work for z3::expr because it redfines
    // the == operator, we need to have our own c++ wrapper
    // assert( !exists( globals, e ) && !exists( globals, ep ) );
    globals.push_back(std::make_pair(e,ep));
  }

  void addLocal (thread_id_type tid, expr& e, expr& ep) {
    assert( tid < size() );
    assert( e.is_app() && ep.is_app() );
    // assert( !exists( locals[t], e ) && !exists( localsP[t], ep ) );
    threads[tid]->locals.push_back(std::make_pair(e,ep));
  }

  location_id_type createFreshLocation (thread_id_type tid) {
    assert (0 <= tid && tid < size());
    auto t = threads[tid];
    auto lid = t->locations.size();
    auto l = new Location(tid, lid);
    t->locations.push_back(l);
    return lid;
  }

  void setStartLocation (thread_id_type tid, location_id_type lid) {
    threads[tid]->startLocation = threads[tid]->locations[lid];
  }

  void setFinalLocation (thread_id_type tid, location_id_type lid) {
    threads[tid]->finalLocation = threads[tid]->locations[lid];
  }

  void setErrorLocation (thread_id_type tid, location_id_type lid) {
    threads[tid]->errorLocation = threads[tid]->locations[lid];
  }

  location_id_type getStartLocation (thread_id_type tid ) const {
    return threads[tid]->startLocation.lid;
  }

  location_id_type getFinalLocation (thread_id_type tid ) const {
    return threads[tid]->finalLocation->lid;
  }

  location_id_type getErrorLocation (thread_id_type tid ) const {
    return threads[tid]->errorLocation.lid;
  }

  void setInitialStates (expr& init) {
    this->init = init;
  }

  void addBlock (thread_id_type tid, location_id_type src,
                 location_id_type dst, const expr& e) {
    auto t = threads[tid];
    assert( src < t->locations.size() && dst < t->locations.size() );
    auto srcP = t->locations[src];
    auto dstP = t->locations[dst];
    //assert( src.tid == dst.tid );
    auto b = new Block(t->blocks.size(), srcP, dstP, e, t->locals, t->globals);
    t->blocks.push_back(b);

    srcP->succ.insert(b);
    dstP->pred.insert(b);
  }

  // inserts a block ep after the block e between src and dst
  // it constructs new location l such that
  //                 src -ep-> dst   => src -ep-> l -e-> dst
  void insertAfterBlock( thread_id_type tid, location_id_type src,
                         location_id_type dst, const expr& e) {
    location_id_type l = createFreshLocation( tid );
    // fresh location
    Block* b = NULL;
    Location* srcLoc = threads[tid]->locations[src];
    Location* dstLoc = threads[tid]->locations[dst];
    for (auto blk : threads[tid]->blocks) {
      if( blk->getSource() == srcLoc && dstLoc == blk->getDestination() ) {
        if( b != NULL )
          cfrontend_error( "insertAfterBlock does not support two blocks!");
        b = blk;
      }
    }
    b->setDestination( threads[tid]->locations[l] );
    addBlock( tid, l, dst, e);
  }

  // -----------------------------------------------------------------------
  // Access functions

  size_type size() const { return threads.size(); }

  typedef typename std::vector<Thread*>::const_iterator thread_iterator;

  thread_iterator begin () const { return threads.cbegin(); }
  thread_iterator end   () const { return threads.cend();   }

  Thread* const& operator[] (size_type n) const {
    return threads[n];
  }

  const expr getInit () const { return init; }

  const Location* getLocation (thread_id_type tid, location_id_type loc) const {
    return threads[tid]->locations[loc];
  }

  // -----------------------------------------------------------------------
  // Helper and utility functions
  void printDot (std::ostream& os) {
    os << "digraph program" << "{\n";
    os << "init [shape=box label=\"initial states : " << init << "\"];\n";

    for (auto thr : threads) {
      os << "subgraph thr_" << thr->tid << "{\n";

      for (auto loc : thr->locations) {
        os << "\"" << thr->tid << "." << loc->lid << "\";\n";
      }

      for (auto blk : thr->blocks) {
        os << "\"b" << thr->tid << "." << blk->bid
           << "\" [shape=box label=\"" << blk->bid << ": "
           << blk->rho <<"\"];\n";
        os << "\""  << thr->tid << "." << blk->src->lid << "\" -> "
           << "\"b" << thr->tid << "." << blk->bid      << "\" -> "
           << "\""  << thr->tid << "." << blk->dst->lid << "\";\n";
      }

      os << "}\n";
    }

    os << "}\n";
  }

private:
  std::vector<Thread*> threads;
  Vars globals;
  expr init;
};

// #########################################################################
// Thread
template <class expr>
class SimpleMultiThreadedProgram<expr>::Thread
{
  friend class SimpleMultiThreadedProgram<expr>;
private:
  Thread (thread_id_type tid_, Vars& globals_)
    : tid(tid_), globals(globals_), name("") {}

  Thread (thread_id_type tid_, Vars& globals_, std::string& name_ )
    : tid(tid_), globals(globals_), name(name_) {}

  thread_id_type tid;
  Vars& globals;
  Vars locals;
  Location* startLocation;
  Location* finalLocation;
  Location* errorLocation;
  std::vector<Location*> locations;
  std::vector<Block*> blocks;
  std::string name;

public:
  thread_id_type getThreadId () const { return tid; }
  const Location* getStartLocation () const { return startLocation; }
  const Location* getFinalLocation () const { return finalLocation; }
  const Location* getErrorLocation () const { return errorLocation; }

  typedef typename Vars::size_type var_size_type;
  typedef typename Vars::const_iterator var_iterator;

  var_size_type globals_size () const { return globals.size(); }
  var_size_type locals_size  () const { return locals.size();  }
  
  var_iterator globals_begin () const { return globals.cbegin(); }
  var_iterator globals_end   () const { return globals.cend();   }
  var_iterator locals_begin  () const { return locals.cbegin();  }
  var_iterator locals_end    () const { return locals.cend();    }
  
  typedef typename std::vector<Location*>::const_iterator loc_iterator;
  typedef typename std::vector<Block*>::const_iterator blk_iterator;

  loc_iterator loc_begin () const { return locations.cbegin(); }
  loc_iterator loc_end   () const { return locations.cend();   }
  blk_iterator blk_begin () const { return blocks.cbegin();    }
  blk_iterator blk_end   () const { return blocks.cend();      }
};

// #########################################################################
// Location
template <class expr>
class SimpleMultiThreadedProgram<expr>::Location
{
  friend class SimpleMultiThreadedProgram<expr>;
private:
  thread_id_type   tid;
  location_id_type lid;
  std::set<Block*> succ;
  std::set<Block*> pred;

  Location (thread_id_type tid, location_id_type lid) : tid(tid), lid(lid) {}

public:
  thread_id_type   getThreadId   () const { return tid; }
  location_id_type getLocationId () const { return lid; }

  const std::set<Block*>& getSuccessors () const { return succ; }
};

// #########################################################################
// Block
template <class expr>
class SimpleMultiThreadedProgram<expr>::Block
{
  friend class SimpleMultiThreadedProgram<expr>;
public:
  block_id_type getBlockId  () const { return bid; }
  Location* getSource       () const { return src; }
  Location* getDestination  () const { return dst; }
  expr getRho               ()       { return rho; }
  const Vars& getUpdateVars ()       { return updateVars; }
  void setSource      ( Location* src_ ) { src = src_; }
  void setDestination ( Location* dst_ ) { dst = dst_; }

private:
  block_id_type bid;

  Location* src;
  Location* dst;

  //Label l;  //Default src Loc
  expr rho;
  Vars updateVars;

  // Block (block_id_type bid, Location src, Location dst, Label l, expr& rho)
  //   : bid(bid), src(src), dst(dst), l(l), rho(rho ) {}

  // Block (block_id_type bid, Location src, Location dst, expr& rho)
  //   : Block(bid, src, dst, src, rho) {}

  Block (block_id_type bid, Location* src, Location* dst, const expr& rho,
         const Vars& locals, const Vars& globals)
    : bid(bid), src(src), dst(dst), rho(rho) {
    // Collect all program variables which are changed by the transition formula
    std::stack<expr> stack;
    stack.push(rho);

    while (!stack.empty()) {
      auto e = stack.top();
      stack.pop();

      if (e.is_app()) {
        auto n = e.num_args();

        if (n == 0) {
          // test if const is primed program variable
          for (auto var : globals) {
            if (eq(e,var.second)) {
              updateVars.push_back(var);
              break;
            }
          }

          for (auto var : locals) {
            if (eq(e, var.second)) {
              updateVars.push_back(var);
              break;
            }
          }
        } else {
          for (unsigned i = 0; i < n; ++i) {
            stack.push(e.arg(i));
          }
        }
      }
    }

    // std::cout << rho << " modifies" << std::endl;
    // for (auto var : updateVars) {
    //   std::cout << "\t" << var.second << std::endl;
    // }
  }
};

}

#endif
