// allocators benchmark, MIT licensed.
// - rlyeh 2013

// @todo
// speed: ms
// memory leak: yes/no
// fragmentation
// memory occupancy
// dynamic growth vs fixed pool
// initial heap size
// small allocs tests
// big alloc tests
// alignment: no/4/8/16
// containers: bench most sequential and associative containers (vector, set, deque, map, unordered_map, unordered_set)
//
// portability: win, posix, unix, mac, ios, android
// flexibility: malloc/free, realloc, std::allocator, new/delete
//   bonus-point: sizealloc()
// integrability: header-only = 10, pair = 6, many = 3, 3rdparty = 0
// has-test-suites
// last-activity-on-repo <3mo, <6mo, <1yo, <2yo
// documentation: yes/no
// compactness: LOC
// open-source: yes/no
//  bonus-point: github +1, gitorius +1, bitbucket 0, sourceforge -1 :D, googlecode -1 :D
// license:
//  bonus-point: permissive-licenses: pd/zlib/mit/bsd3

#include <cassert>
#include <list>
#include <vector>
#include <iostream>
#include <iomanip>
#include <time.h>


// [1]
#include <boost/pool/pool_alloc.hpp>

#if 0
// comment out before making an .exe just to be sure these are not defined elsewhere,
// then comment it back and recompile.
void *operator new( size_t size ) { return std::malloc( size ); }
void *operator new[]( size_t size ) { return std::malloc( size ); }
void operator delete( void *ptr ) { return std::free( ptr ); }
void operator delete[]( void *ptr ) { return std::free( ptr ); }
#endif

#include "winnie2/winnie.hpp"
#include "winnie2/winnie.cpp"

// [2]
#include "winnie3/winnie.hpp"

// [2]
#include "ironpeter/iron.hpp"
#include "ironpeter/iron.cpp"

#include "tav/mballoc.hpp"
#include "tav/mballoc.cpp"
#pragma comment(lib,"user32.lib")

// [3]
//#define FSBALLOCATOR_USE_THREAD_SAFE_LOCKING_STD
#include "FSBAllocator/FSBAllocator.hh"

#include "balloc/balloc.hpp"
#include "balloc/balloc.cpp"

#include "threadalloc/threadalloc.hpp"
#include "threadalloc/threadalloc.cpp"

// [3]
#include "MicroAllocator/MicroAllocator.h"
#include "MicroAllocator/MicroAllocator.inl"

//#include "obstack/obstack.hpp"
//#include "obstack/obstack.cpp"

#include "jemalloc/jemalloc.hpp"

#include "ltalloc/ltalloc.hpp"

//#include "elephant/elephant.hpp"
//#include "elephant/elephant.cpp"

#include "dlmalloc/dlmalloc.hpp"

#include "tlsf/tlsf.hpp"
#include "tlsf0/tlsf.hpp"

#include "dumb_tlsf/dumb_tlsf.hpp"
#include "dumb_tlsf/dumb_tlsf.cpp"

namespace boost {
    void throw_exception( const std::exception &e )
    {}
}

#include <thread>
#include <map>
#include <set>
std::map< double, std::set< std::string > > ranking;

#include <tuple>
enum { THREAD_SAFE, SAFE_DELETE, RESET_MEMORY, AVG_SPEED };
std::map< std::string, std::tuple<bool,bool,bool,double> > feat;
double default_allocator_time = 1;


#if defined(NDEBUG) || defined(_NDEBUG)
#define $release(...) __VA_ARGS__
#define $debug(...)
#else
#define $release(...)
#define $debug(...)   __VA_ARGS__
#endif

#define $stringize(...) #__VA_ARGS__
#define $string(...) std::to_string(__VA_ARGS__)

/*
bool &feature( int what )  {
    if( what == THREAD_SAFE ) return

    static bool dummy;
    return dummy = false;
}
*/

// timing - rlyeh, public domain [ref] https://gist.github.com/r-lyeh/07cc318dbeee9b616d5e {
#include <thread>
#include <chrono>
#if !defined(TIMING_USE_OMP) && ( defined(USE_OMP) || defined(_MSC_VER) /*|| defined(__ANDROID_API__)*/ )
#   define TIMING_USE_OMP
#   include <omp.h>
#endif
struct timing {
    static double now() {
#   ifdef TIMING_USE_OMP
        static auto const epoch = omp_get_wtime(); 
        return omp_get_wtime() - epoch;
#   else
        static auto const epoch = std::chrono::steady_clock::now(); // milli ms > micro us > nano ns
        return std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::steady_clock::now() - epoch ).count() / 1000000.0;
#   endif
    }
    template<typename FN>
    static double bench( const FN &fn ) {
        auto took = -now();
        return ( fn(), took + now() );
    }
    static void sleep( double secs ) {
        std::chrono::microseconds duration( (int)(secs * 1000000) );
        std::this_thread::sleep_for( duration );    
    }
};
// } timing


template<typename TEST>
void benchmark_suite( int mode, const std::string &name, const TEST &container ) {

    auto single = [&]() {
        for (int j=0; j<30000; ++j)
        {
            TEST s;
            for (int i=0; i<100; ++i)
                s.push_back(i);
            TEST s2 = s;
        }
    };

    auto multi = [&]() {
        std::thread th1( [=] { single(); } );
        std::thread th2( [=] { single(); } );
        std::thread th3( [=] { single(); } );
        std::thread th4( [=] { single(); } );
        th1.join();
        th2.join();
        th3.join();
        th4.join();
    };

    auto creator = []( TEST &s ) {
        s = TEST();
        for (int j=0; j<30000; ++j)
            for (int i=0; i<100; ++i)
                s.push_back(i);
    };

    auto deleter = []( TEST &s ) {
        s = TEST();
    };

    auto deleter2 = []( TEST::value_type *&s ) {
        TEST::allocator_type alloc;
        alloc.deallocate( s, 1 );
    };

    auto creator2 = []( TEST::value_type *&s ) {
        TEST::allocator_type alloc;
        s = alloc.allocate( 1 );
    };

    std::get<THREAD_SAFE>(feat[ name ]) = false;
    std::get<SAFE_DELETE>(feat[ name ]) = false;
    std::get<RESET_MEMORY>( feat[ name ] ) = false;
    std::get<AVG_SPEED>(feat[name]) = 0;

    if( mode == 0 )
        return;

    double took = 0;

    if( mode & 1 ) {
        std::string id = std::string() + "single: " + name;
        std::cout << id;
        double span = timing::now();
            single();
        took += (timing::now() - span) * 1000000;
            std::cout << " " << int(took) << "us" << std::endl;
#if 1
        int *x = 0;
        creator2(x);
        if( x == 0 ) throw std::exception("bad test");
        if( *x == 0 ) std::get<RESET_MEMORY>( feat[ name ] ) = true;
        deleter2(x);
        if (x == 0) std::get<SAFE_DELETE>(feat[name]) = true;
#endif
    }
    if( mode & 2 ) {
        {
            std::string id = std::string() + "multi: " + name;
            std::cout << id;
            double span = timing::now();
                multi();
            took += (timing::now() - span) * 1000000;
            std::cout << " " << int(took) << "us" << std::endl;
        }
        if (1)
        {
            std::string id = std::string() + "owner: " + name;
            std::cout << id;
            double span = timing::now();
                TEST s;
                std::thread th1( [&](){ creator(s); } ); th1.join();
                std::thread th2( [&](){ deleter(s); } ); th2.join();
#if 1
                int *x = 0;
                std::thread th3( [&](){ creator2(x); } ); th3.join();
                if( x == 0 ) throw std::exception("bad test");
                if( *x == 0 ) std::get<RESET_MEMORY>( feat[ name ] ) = true;
                std::thread th4( [&](){ deleter2(x); } ); th4.join();
                if( x == 0 ) std::get<SAFE_DELETE>( feat[ name ] ) = true;
#endif
            took += (timing::now() - span) * 1000000;
            std::cout << " " << int(took) << "us" << std::endl;
        }

        std::get<THREAD_SAFE>( feat[ name ] ) = true;
    }

    /**/ if (mode & 3) took /= 3;
    else if (mode & 2) took /= 2;
    else               took /= 1;

    std::get<AVG_SPEED>(feat[name]) = took;
    ranking[ took ].insert( name );

    if( name == "std::allocator" )
        default_allocator_time = took;
}

int main() {
    try {

        auto header = []( const std::string &title ) {
            std::cout << std::endl;
            std::cout << "+-" << std::string( title.size(), '-' ) << "-+" << std::endl;
            std::cout << "| " <<              title               << " |X" << std::endl;
            std::cout << "+-" << std::string( title.size(), '-' ) << "-+X" << std::endl;
			std::cout << " " << std::string( title.size() + 4, 'X' ) << std::endl;
            std::cout << std::endl;
        };

        header("running tests");

        enum { none = 0, single = 1, thread = 2, all = ~0 };
        // some suites got single only because... { - they crashed, or; - they deadlocked, or; - they took more than 30 secs to finish }
        //benchmark_suite( all, "elephant::allocator", std::list<int, elf::allocator<int> >());
        benchmark_suite( all, "std::allocator", std::list<int, std::allocator<int> >());
        benchmark_suite(single, "tlsf::allocator", std::list<int, tlsf::allocator<int> >());
        benchmark_suite(single, "tlsf0::allocator", std::list<int, tlsf0::allocator<int> >());
        benchmark_suite(single, "dumb_tlsf::allocator", std::list<int, dumb_tlsf::allocator<int> >());
        benchmark_suite( all, "jemalloc::allocator", std::list<int, jemalloc::allocator<int> >());
        benchmark_suite(single, "winnie::allocator", std::list<int, winnie::allocator<int> >());
        benchmark_suite(single, "FSBAllocator", std::list<int, FSBAllocator<int> >());
        benchmark_suite(single, "FSBAllocator2", std::list<int, FSBAllocator2<int> >());
        benchmark_suite(single, "boost::pool_allocator", std::list<int, boost::pool_allocator<int> >());
        benchmark_suite(single, "boost::fast_pool_allocator", std::list<int, boost::fast_pool_allocator<int> >());
        benchmark_suite( all, "Winnie::CFastPoolAllocator", std::list<int, Winnie::CFastPoolAllocator<int> >());
        benchmark_suite( all, "threadalloc::allocator", std::list<int, threadalloc::allocator<int> >());
        benchmark_suite( all, "micro::allocator", std::list<int, micro::allocator<int> >());
        benchmark_suite(none, "ballocator::allocator", std::list<int, ballocator::allocator<int> >());
        benchmark_suite(single, "iron::allocator", std::list<int, iron::allocator<int> >());
      //benchmark_suite( all, "obstack::allocator", std::list<int, boost::arena::basic_obstack<int> >() );
        benchmark_suite( all, "tav::allocator", std::list<int, tav::allocator<int> >());
        benchmark_suite( all, "lt::allocator", std::list<int, lt::allocator<int> >());
        benchmark_suite( all, "dl::allocator", std::list<int, dl::allocator<int> >());

        header( std::string() + "comparison table " +
            $release("(RELEASE)") + $debug("(DEBUG)") + " (MSC " + $string(_MSC_FULL_VER) + ") " __TIMESTAMP__);

        std::cout << "       " << std::string(40, ' ') + "THS RSM SFD AVG" << std::endl;
        int pos = 1;
        for (const auto &result : ranking) {
            const auto &mark = result.first;
            for (const auto &name : result.second) {
                const auto &values = feat[name];
                if (pos < 10) std::cout << " ";
                /**/ if (pos % 10 == 1) std::cout << pos++ << "st)";
                else if (pos % 10 == 2) std::cout << pos++ << "nd)";
                else if (pos % 10 == 3) std::cout << pos++ << "rd)";
                else                     std::cout << pos++ << "th)";
                std::cout << " " << (name)+std::string(40 - name.size(), ' ');
                std::cout << " " << (std::get<THREAD_SAFE >(values) ? "[x]" : "[ ]");
				std::cout << " " << (std::get<RESET_MEMORY>(values) ? "[x]" : "[ ]");
				std::cout << " " << (std::get<SAFE_DELETE >(values) ? "[x]" : "[ ]");
                std::cout << " " << int(std::get<AVG_SPEED>(values)) << " us";
                double factor = std::get<AVG_SPEED>(values)/default_allocator_time;
                /**/ if( factor > 1.05 ) std::cout << " (x" << std::setprecision(3) << (    factor) << " times slower)";
                else if( factor < 0.95 ) std::cout << " (x" << std::setprecision(3) << (1.0/factor) << " times faster)";
                else                     std::cout << " (performs similar to standard allocator)";
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
        std::cout << "THS: THREAD_SAFE: safe to use in multithreaded scenarios (on is better)" << std::endl;
        std::cout << "RSM: RESET_MEMORY: allocated contents are reset to zero (on is better)" << std::endl;
        std::cout << "SFD: SAFE_DELETE: deallocated pointers are reset to zero (on is better)" << std::endl;
        std::cout << "AVG: AVG_SPEED: average time for each benchmark (lower is better)" << std::endl;

        return 0;

    }
    catch (std::exception &e) {
        std::cout << "exception thrown :( " << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "exception thrown :( no idea" << std::endl;
    }

    std::cout << "trying to invoke debugger..." << std::endl;
    assert(!"trying to invoke debugger...");

    return -1;
}
