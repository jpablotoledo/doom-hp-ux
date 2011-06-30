/* INLINE STUFF -- used to be in p_maputl.c */

/* P_BlockLinesIterator*/
/* The validcount flags are used to avoid checking lines*/
/* that are marked in multiple mapblocks,*/
/* so increment validcount before the first call*/
/* to P_BlockLinesIterator, then make one or more calls*/
/* to it.*/

#ifdef DIYINLINE
static
#endif
boolean
P_BlockLinesIterator
( int			x,
  int			y,
  boolean(*func)(line_t*) )
{
    int			offset;
    blockmap_t*		list;
    line_t*		ld;
    vertex_t*		limvert;

    if (x<0
	|| y<0
	|| x>=bmapwidth
	|| y>=bmapheight)
    {
	return true;
    }

    offset = y*bmapwidth+x;

    offset = *(blockmap+offset);

    limvert = vertexes + numvertexes;
    for ( list = blockmaplump+offset ; *list != -1 ; list++)
    {
        if (*list < numlines)
	{
	    ld = &lines[*list];

            if ((ld->v1 < vertexes) || (ld->v2 < vertexes)
             || (ld->v1 >= limvert) || (ld->v2 >= limvert))
            {
                /*fprintf(logfile, "P_BlockLinesIterator: bad line segment!\n");*/
	        return false;
            }

	    if (ld->validcount == validcount)
	        continue; 	/* line has already been checked*/

	    ld->validcount = validcount;

	    if ( !func(ld) )
	        return false;
	}
    }
    return true;	/* everything was checked*/
}
