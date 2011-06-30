/* INLINE STUFF */
/* used to be in r_main.c */

/* R_PointOnSide*/
/* Traverse BSP (sub) tree,*/
/*  check point against partition plane.*/
/* Returns side 0 (front) or 1 (back).*/

#ifdef DIYINLINE
static
#endif
int
R_PointOnSide
( fixed_t	x,
  fixed_t	y,
  const node_t*	node )
{
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	left;
    fixed_t	right;

    if (!node->dx)
    {
	if (x <= node->x)
	    return node->dy > 0;

	return node->dy < 0;
    }
    if (!node->dy)
    {
	if (y <= node->y)
	    return node->dx < 0;

	return node->dx > 0;
    }

    dx = (x - node->x);
    dy = (y - node->y);

    /* Try to quickly decide by looking at sign bits.*/
    if ( (node->dy ^ node->dx ^ dx ^ dy)&0x80000000 )
    {
	if  ( (node->dy ^ dx) & 0x80000000 )
	{
	    /* (left is negative)*/
	    return 1;
	}
	return 0;
    }

    left = FixedMul ( node->dy>>FRACBITS , dx );
    right = FixedMul ( dy , node->dx>>FRACBITS );

    if (right < left)
    {
	/* front side*/
	return 0;
    }
    /* back side*/
    return 1;
}
