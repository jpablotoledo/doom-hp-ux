/* INLINE STUFF -- used to be in p_maputl.c */

/* P_BlockThingsIterator*/

#ifdef DIYINLINE
static
#endif
boolean
P_BlockThingsIterator
( int			x,
  int			y,
  boolean(*func)(mobj_t*) )
{
    mobj_t*		mobj;

    if ( x<0
	 || y<0
	 || x>=bmapwidth
	 || y>=bmapheight)
    {
	return true;
    }


    for (mobj = blocklinks[y*bmapwidth+x] ;
	 mobj ;
	 mobj = mobj->bnext)
    {
	if (!func( mobj ) )
	    return false;
    }
    return true;
}
