#include <cstdint>
#include <cstdio>
#include <iostream>
#include <array>
#include <algorithm>
#include <thread>
#include <tuple>
#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <typeinfo>
#include <set>
#include <vector>
#include <map>
#include <utility>
#include <tuple>
#include <functional>
#include <numeric>
#include <fcntl.h>
#include <sys/stat.h>
#include <tuple>
#include <sys/sdt.h>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>



int main( int argc, char **argv )
{
    std::uint64_t pow = strtoull( argv[ 1 ], nullptr, 10 );
    const auto size = (1ULL << pow );
    using type_t = std::uint64_t;
#define TYPE PRIu64    
    type_t *arr = nullptr;
    if( posix_memalign( (void**)&arr, 1<<16, sizeof( type_t ) * size ) != 0 )
    {
        std::perror( "failed to allocate page-aligned memory" );
        exit( EXIT_FAILURE );
    }
    STAP_PROBE( region_coverage, app_start );
    for( int i = 0; i < (size / sizeof( type_t ) ); i++ )
    {
        arr[ i ] = i;
    }
    STAP_PROBE( region_coverage, app_stop );
    

    FILE *fp = fopen( "/tmp/foobar", "w" );
    for( int i = 0; i < (size / sizeof( type_t ) ); i++ )
    {
        fprintf( fp, "%" TYPE "\n", arr[ i ] );
    }
    //get own pid
    //const auto pid = getpid();
    //std::stringstream ss;
    //ss << "cat /proc/" << pid << "/maps > /tmp/maps_log";
    //if( system( ss.str().c_str() ) != 0 )
    //{
    //   std::perror( "failed to capture /proc/smaps" );
    //}
    //free( arr );
    //fclose( fp );
    return( EXIT_SUCCESS );
};
