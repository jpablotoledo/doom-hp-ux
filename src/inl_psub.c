/* INLINE STUFF */
/* used to be in r_main.c */


/* R_PointInSubsector*/

#ifdef DIYINLINE
static
#endif
subsector_t*
R_PointInSubsector
( fixed_t	x,
  fixed_t	y )
{
    node_t*	node;
    int		side;
    int		nodenum;

    /* single subsector is a special case*/
    if (!numnodes)
	return subsectors;

    nodenum = numnodes-1;

    while (! (nodenum & NF_SUBSECTOR) )
    {
	node = &nodes[nodenum];
	side = R_PointOnSide (x, y, node);
	nodenum = node->children[side];
    }

    return &subsectors[nodenum & ~NF_SUBSECTOR];
}
