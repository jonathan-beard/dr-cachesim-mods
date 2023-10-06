#include "requestable.h"



bool 
requestable_t::init( requestable_t *parent )
{
    parent_ = parent;
    return( true );
}
