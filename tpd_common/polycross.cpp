//===========================================================================
//                                                                          =
//   This program is free software; you can redistribute it and/or modify   =
//   it under the terms of the GNU General Public License as published by   =
//   the Free Software Foundation; either version 2 of the License, or      =
//   (at your option) any later version.                                    =
// ------------------------------------------------------------------------ =
//                  TTTTT    OOO    PPPP    EEEE    DDDD                    =
//                  T T T   O   O   P   P   E       D   D                   =
//                    T    O     O  PPPP    EEE     D    D                  =
//                    T     O   O   P       E       D   D                   =
//                    T      OOO    P       EEEEE   DDDD                    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Tue Mar 21 2006
//         Author: s_krustev@yahoo.com
//      Copyright: (C) 2001-2006 by Svilen Krustev
//    Description: Modified Bentley-Ottman algorithm
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include <math.h>
#include <algorithm>
#include "polycross.h"
#include "outbox.h"

#define BO2_DEBUG
#define BO_printseg(SEGM) printf("thread %i : polygon %i, segment %i, \
lP (%i,%i), rP (%i,%i)  \n" , SEGM->threadID(), SEGM->polyNo() , SEGM->edge(), \
SEGM->lP()->x(), SEGM->lP()->y(), SEGM->rP()->x(),SEGM->rP()->y());

//-----------------------------------------------------------------------------
// The declaratoin of the avl related functions. They are declared originally
// in avl.h and redeclared here in C++ manner with extern "C" clause
extern "C" {
   avl_table* avl_create (avl_comparison_func *, const void *, libavl_allocator *);
   void **avl_probe (struct avl_table *, void *);
   void*      avl_delete (avl_table *, const void *);
   void*      avl_t_first (avl_traverser *, avl_table *);
}

//==============================================================================
/**
 * Determines the lexicographical order of two points comparing X first. 
 * @param p1 - first point for comparison
 * @param p2 - second point for comparison
 * @return +1 -> p1  > p2
           -1 -> p1  < p2
            0 -> p1 == p2
 */
int polycross::xyorder(const TP* p1, const TP* p2) {
   // first check that the pointers are the same
   if (p1 == p2) return 0;
   // test X coord first
   if (p1->x() > p2->x()) return  1;
   if (p1->x() < p2->x()) return -1;
   // and then y
   if (p1->y() > p2->y()) return  1;
   if (p1->y() < p2->y()) return -1;
   return 0;
}

int polycross::orientation(const TP* p1, const TP* p2, const TP* p3)
{
   // twice the "orientated" area of the enclosed triangle
   float area = (p1->x() - p3->x()) * (p2->y() - p3->y()) -
         (p2->x() - p3->x()) * (p1->y() - p3->y());
   if (0 == area) return 0;
   else
      return (area > 0) ? 1 : -1;
}

/**
 * Cheeck whether point p lies inside the segment defined by p1 and p2. The
function assumes that p lies on the line defined by p1 and p2
 * @param p1 first point defining the input segment
 * @param p2 second point defining the input segment
 * @param p the point to be checked
 * @return < 0  the point is outside the segment
           = 0  the point coinsides with one of the segment endpoints
           > 0  the point is inside the segment

 */
float polycross::getLambda( const TP* p1, const TP* p2, const TP* p)
{
   float denomX = p2->x() - p->x();
   float denomY = p2->y() - p->y();
   float lambda;
   if      (0 != denomX) lambda = (p->x() - p1->x()) / denomX;
   else if (0 != denomY) lambda = (p->y() - p1->y()) / denomY;
   // point coincides with the lp vertex of the segment
   else lambda = 0;
   return lambda;
}

//==============================================================================
// VPoint
polycross::VPoint::VPoint(const TP* point, VPoint* prev) : _cp(point), _prev(prev)
{
   if (_prev) _prev->_next = this;
}

bool polycross::VPoint::inside(const pointlist& plist)
{
   TP p0, p1;
   byte cc = 0;
   unsigned size = plist.size();
   for (unsigned i = 0; i < size ; i++)
   {
      p0 = plist[i]; p1 = plist[(i+1) % size];
      if (((p0.y() <= _cp->y()) && (p1.y() >=  _cp->y()))
            ||((p0.y() >=  _cp->y()) && (p1.y() <= _cp->y())) )
      {
         float tngns = (float) (_cp->y() - p0.y())/(p1.y() - p0.y());
         if (_cp->x() <= p0.x() + tngns * (p1.x() - p0.x()))
            cc++;
      }
   }
   return (cc & 0x01) ? true : false;
}

polycross::VPoint* polycross::VPoint::follower(bool& direction, bool) {
   return direction ? _next : _prev;
}


//==============================================================================
// class CPoint

void polycross::CPoint::linkage(VPoint*& prev) {
   _prev = prev;
   if (prev) prev->set_next(this);
   prev = this;
}

polycross::VPoint* polycross::CPoint::follower(bool& direction, bool modify) {
   if (modify) direction = !direction;
   polycross::VPoint* flw = direction ? _link->_next : _link->_prev;
   _visited++; _link->_visited++;
   return flw;
}

//==============================================================================
// class CPoint
polycross::VPoint* polycross::BPoint::follower(bool& direction, bool modify) {
   if (modify) {
      direction = !direction;
      return direction ? _next : _prev;
   }
   else return _link;
}

//==============================================================================
// SortLine

bool polycross::SortLine::operator() (CPoint* cp1, CPoint* cp2) {
   if (direction == xyorder(cp1->cp(), cp2->cp()))  return true;
   else                                             return false;
}

//==============================================================================
// polysegment

polycross::polysegment::polysegment(const TP* p1, const TP* p2, int num, char plyn) {
   if (xyorder(p1, p2) < 0) { _lP = p1; _rP = p2; }
   else                     { _lP = p2; _rP = p1; }
   _edge = num; _polyNo = plyn;_threadID = 0;
}

/**
 * The method creates a new #CPoint object from the input pnt and adds it to the
crosspoints array. The input point is assumed to be unique. Method is called by
Event::insertCrossPoint() only.
 * @param pnt the new cross point - assumed to be unique
 * @return the #CPoint that will be linked to its counterpart by the caller
 */
polycross::CPoint* polycross::polysegment::insertCrossPoint(const TP* pnt) {
   CPoint* cp = new CPoint(pnt);
   crosspoints.push_back(cp);
   return cp;
}

unsigned polycross::polysegment::normalize(const TP* p1, const TP* p2) {
   _lP = p1; _rP = p2;
   unsigned numcross = crosspoints.size();
   if (crosspoints.size() > 1) {
      SortLine functor(p1,p2);
      std::sort(crosspoints.begin(), crosspoints.end(), functor);
   }
   return numcross;
}

void polycross::polysegment::dump_points(polycross::VPoint*& vlist) {
   vlist = new VPoint(_lP, vlist);
   for (unsigned i = 0; i < crosspoints.size(); i++)
   {
      crosspoints[i]->linkage(vlist);
#ifdef BO2_DEBUG
      printf("( %i , %i )\n", crosspoints[i]->cp()->x(), crosspoints[i]->cp()->y());
#endif
   }
}

polycross::BPoint* polycross::polysegment::insertBindPoint(const TP* pnt)
{
   BPoint* cp = new BPoint(pnt);
   crosspoints.push_back(cp);
   return cp;
}

//==============================================================================
// class segmentlist

polycross::segmentlist::segmentlist(const pointlist& plst, byte plyn) {
   _originalPL = &plst;
   unsigned plysize = plst.size();
   _segs.reserve(plysize);
   for (unsigned i = 0; i < plysize; i++)
      _segs.push_back(new polysegment(&(plst[i]),&(plst[(i+1)%plysize]),i, plyn));
}

polycross::BPoint* polycross::segmentlist::insertbindpoint(unsigned segno, const TP* point) {
    return _segs[segno]->insertBindPoint(point);
}

polycross::segmentlist::~segmentlist() {
   for (unsigned i = 0; i < _segs.size(); i++)
      delete _segs[i];
   _segs.clear();
}

unsigned polycross::segmentlist::normalize(const pointlist& plst) {
   unsigned numcross = 0;
   unsigned plysize = plst.size();
   for (unsigned i = 0; i < plysize; i++)
      numcross += _segs[i]->normalize(&(plst[i]),&(plst[(i+1)%plysize]));
   return numcross;
}

polycross::VPoint* polycross::segmentlist::dump_points()
{
   VPoint* vlist = NULL;
#ifdef BO2_DEBUG
      printf("---------------Crossing points found-----------\n");
#endif
   for (unsigned i = 0; i < _segs.size(); i++)
      _segs[i]->dump_points(vlist);
   polycross::VPoint* lastV = vlist;
   VPoint* centinel = NULL;
   while (vlist->prev())
   {
      if (-1 == vlist->visited()) centinel = vlist;
      vlist = vlist->prev();
   }
   lastV->set_next(vlist);
   vlist->set_prev(lastV);
   if (NULL != centinel)
   {  // for binding points
      VPoint* vwork = centinel;
      do
      {
         if (-1 == vwork->visited())
         {
            //here visited == 0 means only that the object is Cpoint.
            VPoint* tbdel = NULL;
            if ((*vwork->cp()) == (*vwork->prev()->cp()))
            {
               tbdel = vwork->prev();
               vwork->set_prev(vwork->prev()->prev());
               vwork->prev()->set_next(vwork);
            }
            else if ((*vwork->cp()) == (*vwork->next()->cp()))
            {
               tbdel = vwork->next();
               vwork->set_next(vwork->next()->next());
               vwork->next()->set_prev(vwork);
            }
            vwork = vwork->next();
            if (tbdel) delete tbdel;
         }
         else vwork = vwork->next();
      } while (centinel != vwork);
   }
   return vlist;
}

//==============================================================================
// TEvent

/**
 * The method is using orientation() function to do the job. Segments that belong to
the same polygon are not checked and method returns NULL. \n
In case segments intersect, the method calls insertCrossPoint() method. 
 * @param above 
 * @param below 
 * @param eventQ Event queue that will take the new crossing event.
 * @param iff represent the conditional point - if the crossing point found has
the same coordinates as iff point, then a new event will be added to the event
queue. This is used for touching points, i.e. iff can be only a vertex of the
input segments
 * @return the corssing point between segments if it exists. Otherwise - NULL
 */
TP* polycross::TEvent::checkIntersect(polysegment* above, polysegment* below,
                                       XQ& eventQ, const TP* iff)
{
   TP* CrossPoint = NULL;
   // check that the cross candidates are not a sentinel segment
   // check for polygon edges belonging to the same polygon
   if ((0 == below->polyNo()) || (0 == above->polyNo()) ||
        (above->polyNo() == below->polyNo())) return NULL;
   // Now test for intersection point exsistence
   float lsign, rsign, rlmul;
   
   //check that both below endpoints are on the same side of above segment
   lsign = orientation(above->lP(), above->rP(), below->lP());
   rsign = orientation(above->lP(), above->rP(), below->rP());
   // do the case when both segments lie on the same line
   if ((lsign == 0) && (rsign == 0))
   {
      if ((iff == NULL) && ((CrossPoint = oneLineSegments(above, below, eventQ))))
         insertCrossPoint(CrossPoint, above, below, eventQ, true);
      return CrossPoint;
   }
   rlmul = lsign*rsign;
   if      (0  < rlmul)  return NULL;// not crossing
   if (0 == rlmul)
   {
      // possibly touching segments
      CrossPoint = joiningSegments(above, below, lsign, rsign);
      if ( (NULL != CrossPoint) && ((NULL == iff) || (*CrossPoint == *iff)) )
         insertCrossPoint(CrossPoint, above, below, eventQ);
      return CrossPoint;
   }

   //now check that both above endpoints are on the same side of below segment
   lsign = orientation(below->lP(), below->rP(), above->lP());
   rsign = orientation(below->lP(), below->rP(), above->rP());
   // lsign == rsign == 0 case here should not be possible
   if ((lsign == 0) && (rsign == 0))
      throw EXPTNpolyCross("Segments shouldn't coincide at this point");
   rlmul = lsign*rsign;
   if      (0  < rlmul) return NULL;
   if (0 == rlmul)
   {
      // possibly touching segments
      CrossPoint = joiningSegments(below, above, lsign, rsign);
      if ( (NULL != CrossPoint) && ((NULL == iff) || (*CrossPoint == *iff)) )
         insertCrossPoint(CrossPoint, above, below, eventQ);
      return CrossPoint;
   }
   
   // at this point - the only possibility is that they intersect
   // so - create a cross event
   CrossPoint = getCross(above, below);
   if (NULL == iff)
      insertCrossPoint(CrossPoint, above, below, eventQ);
   return CrossPoint;
}

/**
 * Method should be called with one of lps/rps parameters equal to 0. Crossing
point is returned only if it is not an end point of both segments.

 * @param above 
 * @param below 
 * @param lps 
 * @param rps 
 * @return 
 */
TP* polycross::TEvent::joiningSegments(polysegment* above, polysegment* below, float lps, float rps)
{
   if (0 == lps)
   {
      if (0 >= getLambda(above->lP(), above->rP(), below->lP())) return NULL;
      else return new TP(*(below->lP()));
   }
   else if(0 == rps)
   {
      if (0 >= getLambda(above->lP(), above->rP(), below->rP())) return NULL;
      else return new TP(*(below->rP()));
   }
   assert(false);
}

TP* polycross::TEvent::oneLineSegments(polysegment* above, polysegment* below, XQ& eventQ)
{
   float lambdaL = getLambda(above->lP(), above->rP(), below->lP());
   float lambdaR = getLambda(above->lP(), above->rP(), below->rP());
   // coinsiding vertexes - register crossing point only if segments do not overlap
   // (apart from the common point)
   if (0 == lambdaL == lambdaR)
      // coinsiding segments
      return NULL;
   if ((0 == lambdaL) && (0 > lambdaR))
   {
      float lambda2;
      if (*(below->lP()) == *(above->lP()))
         lambda2 = getLambda(below->lP(), below->rP(), above->rP());
      else 
         lambda2 = getLambda(below->lP(), below->rP(), above->lP());
      if (0 > lambda2)
         // segment below extends segment above
         return new TP(*(below->lP()));
      else
         return NULL;
   }
   if ((0 == lambdaR) && (0 > lambdaL))
   {
      float lambda2;
      if (*(below->rP()) == *(above->lP()))
         lambda2 = getLambda(below->lP(), below->rP(), above->rP());
      else
         lambda2 = getLambda(below->lP(), below->rP(), above->lP());
      if (0 > lambda2)
         // segment above extends segment below
         return new TP(*(below->rP()));
      else
         return NULL;
   }
   bool swaped = false;
   // first of all cases when neither of the lines is enclosing the other one
   // here we are generating "hidden" cross point, right in the middle of their
   // coinsiding segment
   if ((lambdaL < 0) && (lambdaR > 0))
      return NULL;//getMiddle(lP(), below->rP());
   if ((lambdaL > 0) && (lambdaR < 0))
      return NULL;//getMiddle(below->lP(), rP());
   if ((lambdaL < 0) && (lambdaR < 0))
   {
      // below edge points are outside the above segment, this however
      // could mean that below fully encloses above
      lambdaL = getLambda(below->lP(), below->rP(), above->lP());
      lambdaR = getLambda(below->lP(), below->rP(), above->rP());
      if ((lambdaL < 0) && (lambdaR < 0)) return NULL;
      swaped = true;
   }
   // now the cases when one of the lines encloses fully the other one
   polysegment* outside = swaped ? below : above;
   polysegment* inside  = swaped ? above : below;
   // get the original polygon to which enclosed segment belongs to
   const pointlist* insideP = (1 == (swaped ?  above->polyNo() : below->polyNo())) ?
         eventQ.opl1() : eventQ.opl2();
   // ... and get the location of the inside segment in that sequence
   int numv = insideP->size();
   unsigned indxLP = (*(inside->lP()) == (*insideP)[inside->edge()]) ?
         inside->edge() : (inside->edge() + 1) % numv;
   unsigned indxRP = (*(inside->rP()) == (*insideP)[inside->edge()]) ?
         inside->edge() : (inside->edge() + 1) % numv;
   // make sure they are not the same
   if (indxLP == indxRP)
      throw EXPTNpolyCross("Can't locate properly the inside segment");

   // we'll pickup the neighbouring point(s) of the inside segment and will
   // recalculate the lps/rps for them.
   bool indxpos = indxLP > indxRP;
   float lps, rps;
   do
   {
      // the code below just increments/decrements the indexes in the point sequence
      // they look so weird, to keep the indexes within [0:_numv-1] boundaries
      if (0 == lps)
      {
         if (indxpos) indxLP = ++indxLP % numv;
         else (0==indxLP) ? indxLP = numv-1 : indxLP--;
      }
      if (0 == rps)
      {
         if (!indxpos) indxRP = ++indxRP % numv;
         else (0==indxRP) ? indxRP = numv-1 : indxRP--;
      }
      // calculate lps/rps with the new points
      lps = orientation(outside->lP(), outside->rP(), &((*insideP)[indxLP]));
      rps = orientation(outside->lP(), outside->rP(), &((*insideP)[indxRP]));
   } while (0 == (lps * rps));
   // When segments are crossing (because of the direction of their neightbours),
   // then a crossing point is introduced in the midle of the internal segment
   if   (lps*rps > 0) return NULL;
   else return getMiddle(inside->lP(), inside->rP());
}

/**
 * Here a crossing point #_cp is calculated from the input segments. The
function assumes that the input two segments definitely cross each other.
It is called only from TEvent::checkIntersect() in case the check for crossing
segments is positive.\n
Crossing point is calculated from general line equations Ax+By+C=0. A care
is taken for division by 0 case when A or B coefficients are 0. It 
is important to note as well that the calculations must be done in floting
point expressions, otherwise the hell might brake loose.
 * @param above first input segment
 * @param below second input segment
 * @return the crossing point between the input segments
 */
TP* polycross::TEvent::getCross(polysegment* above, polysegment* below)
{
   real A1 = above->rP()->y() - above->lP()->y();
   real A2 = below->rP()->y() - below->lP()->y();
   real B1 = above->lP()->x() - above->rP()->x();
   real B2 = below->lP()->x() - below->rP()->x();
   real C1 = -(A1*above->lP()->x() + B1*above->lP()->y());
   real C2 = -(A2*below->lP()->x() + B2*below->lP()->y());
   assert((A1 != 0) || (A2 != 0));
   assert((B1 != 0) || (B2 != 0));
   // segments will coinside if    A1/A2 == B1/B2 == C1/C2
   // segments will be paraller if A1/A2 == B1/B2 != C1/C2
   real X,Y;
   if ((0 != A1) && (0 != B2))
   {
      X = - ((C1 - (B1/B2) * C2) / (A1 - (B1/B2) * A2));
      Y = - ((C2 - (A2/A1) * C1) / (B2 - (A2/A1) * B1));
   }
   else if ((0 != B1) && (0 != A2))
   {
      X = - (C2 - (B2/B1) * C1) / (A2 - (B2/B1) * A1);
      Y = - (C1 - (A1/A2) * C2) / (B1 - (A1/A2) * B2);
   }
   else
      throw EXPTNpolyCross("Input segments don't have a crossing point");
   return new TP((int)rint(X),(int)rint(Y));
}


TP* polycross::TEvent::getMiddle(const TP* p1, const TP* p2)
{
   int4b x = (p2->x() + p1->x()) / 2;
   int4b y = (p2->y() + p1->y()) / 2;
   return new TP(x,y);
}

/**
 * After a crossing point has been discovered, this method inserts it in each
of the input segments after what they are linked to each other as well. This
data will be used later, outside the Bentley-Ottmann algorithm to form the
new polygons. The method also creates a cross event (#TcEvent), but only if
dontswap is false.
* @param CP the crossing point
 * @param above first segment
 * @param below second segment
 * @param eventQ the event queue that will assept the possible cross event
 * @param dontswap if *true* a cross event is not created
 */
void polycross::TEvent::insertCrossPoint(TP* CP, polysegment* above,
                                 polysegment* below, XQ& eventQ, bool dontswap)
{
   assert(NULL != CP);
   CPoint* cpsegA = above->insertCrossPoint(CP);
   CPoint* cpsegB = below->insertCrossPoint(CP);
   cpsegA->linkto(cpsegB);
   cpsegB->linkto(cpsegA);
#ifdef BO2_DEBUG
   printf("**Cross point (%i,%i) inserted between segments below :\n", CP->x(), CP->y());
   BO_printseg(above)
   BO_printseg(below)
#endif
   if (!dontswap)
   eventQ.addCrossEvent(CP, above->threadID(), below->threadID());
}

//==============================================================================
// TbEvent

polycross::TbEvent::TbEvent (polysegment* seg1, polysegment* seg2, byte shapeID):
      TEvent( shapeID )
{
   if (seg1->lP() != seg2->lP())
      throw EXPTNpolyCross("Invalid input segments in thread begin");
   int ori = orientation(seg2->lP(), seg2->rP(), seg1->rP());
   if (0 == ori)
      throw EXPTNpolyCross("Invalid input segments in thread begin");
   if (ori > 0)
   {
      _aseg = seg1; _bseg = seg2;
   }
   else
   {
      _aseg = seg2; _bseg = seg1;
   }
   _evertex = seg1->lP();
}

void polycross::TbEvent::sweep(XQ& eventQ, YQ& sweepline, ThreadList& threadl)
{
   // create the threads
   SegmentThread* athr = sweepline.beginThread(_aseg);
   SegmentThread* bthr = sweepline.beginThread(_bseg);
   if ((athr->threadAbove() == bthr) || (bthr->threadBelow() == athr))
      throw EXPTNpolyCross("Invalid segment sort in thread begin");
   threadl.push_back(_aseg->threadID());
   threadl.push_back(_bseg->threadID());
#ifdef BO2_DEBUG
   printf("Begin 2 threads :\n");
   BO_printseg(_aseg)
   BO_printseg(_bseg)
   sweepline.report();
#endif
   // in all cases check:
   checkIntersect(athr->threadAbove()->cseg(), _aseg, eventQ);
   checkIntersect(_bseg, bthr->threadBelow()->cseg(), eventQ);
   // check that the new threads are neighbours
   if (!((athr->threadBelow() == bthr) && (bthr->threadAbove() == athr)))
   {
      // not neighbours - check the rest
      checkIntersect(_aseg, athr->threadBelow()->cseg(), eventQ);
      checkIntersect(bthr->threadAbove()->cseg(), _bseg, eventQ);
   }
   // check the case when left points are also crossing points
   // this is also the case when one of the input segments coincides with
   // an existing segment
   checkIntersect(_aseg, bthr->threadBelow()->cseg(), eventQ, _aseg->lP());
   checkIntersect(athr->threadAbove()->cseg(), _bseg, eventQ, _bseg->lP());
}


void polycross::TbEvent::sweep2bind(YQ& sweepline, BindCollection& bindColl)
{
   // create the threads
   SegmentThread* athr = sweepline.beginThread(_aseg);
   SegmentThread* bthr = sweepline.beginThread(_bseg);
   if ((athr->threadAbove() == bthr) || (bthr->threadBelow() == athr))
      throw EXPTNpolyCross("Invalid segment sort in thread begin - bind");
   // action is taken only for points in the second polygon
   if (1 == _aseg->polyNo() == _bseg->polyNo()) return;
   // assuming that the polygons are not crossing ...
   if (!((athr->threadBelow() == bthr) && (bthr->threadAbove() == athr)))
      throw EXPTNpolyCross("Crossing input polygons in bind algo - begin");
   // if left point is above and the neighbour above is from polygon 1
   if ((_aseg->lP()->y() >= _aseg->rP()->y()) &&
        (1 == athr->threadAbove()->cseg()->polyNo()))
      bindColl.update_BL(athr->threadAbove()->cseg(), _aseg->edge(), _aseg->lP());
   // if the left point is below and the neighbour below is from polygon 1
   if ((_bseg->lP()->y() <= _bseg->rP()->y()) &&
        (1 == bthr->threadBelow()->cseg()->polyNo()))
      bindColl.update_BL(bthr->threadBelow()->cseg(), _bseg->edge(), _bseg->lP());
}

//==============================================================================
// TeEvent

polycross::TeEvent::TeEvent (polysegment* seg1, polysegment* seg2, byte shapeID):
      TEvent( shapeID )
{
   if (seg1->rP() != seg2->rP())
      throw EXPTNpolyCross("Invalid input segments in thread end");
   int ori = orientation(seg2->lP(), seg2->rP(), seg1->lP());
   if (0 == ori)
      throw EXPTNpolyCross("Invalid input segments in thread end");
   if (ori > 0)
   {
      _aseg = seg1; _bseg = seg2;
   }
   else
   {
      _aseg = seg2; _bseg = seg1;
   }
   _evertex = seg1->rP();

}

void polycross::TeEvent::sweep (XQ& eventQ, YQ& sweepline, ThreadList& threadl)
{
   threadl.push_back(_aseg->threadID());
   threadl.push_back(_bseg->threadID());
   SegmentThread* athr = sweepline.getThread(_aseg->threadID());
   SegmentThread* bthr = sweepline.getThread(_bseg->threadID());
   if ((athr->threadAbove() == bthr) || (bthr->threadBelow() == athr))
      throw EXPTNpolyCross("Invalid segment sort in thread end");
   if ((athr->threadBelow() == bthr) && (bthr->threadAbove() == athr))
   {
      // neighbours - check the case when right points are also crossing points
      checkIntersect(_aseg, bthr->threadBelow()->cseg(), eventQ, _aseg->rP());
      checkIntersect(athr->threadAbove()->cseg(), _bseg, eventQ, _bseg->rP());
      // and new neighbours
      checkIntersect(athr->threadAbove()->cseg(), bthr->threadBelow()->cseg(), eventQ);

   }
   else
   {
      // not neighbours - check both neighbours
      checkIntersect(athr->threadAbove()->cseg(), athr->threadBelow()->cseg(), eventQ);
      checkIntersect(bthr->threadAbove()->cseg(), bthr->threadBelow()->cseg(), eventQ);
   }
   sweepline.endThread(_aseg->threadID());
   sweepline.endThread(_bseg->threadID());
#ifdef BO2_DEBUG
   printf("End 2 threads\n");
   BO_printseg(_aseg)
   BO_printseg(_bseg)
   sweepline.report();
#endif
}

void polycross::TeEvent::sweep2bind(YQ& sweepline, BindCollection& bindColl)
{
   SegmentThread* athr = sweepline.getThread(_aseg->threadID());
   SegmentThread* bthr = sweepline.getThread(_bseg->threadID());
   if ((athr->threadAbove() == bthr) || (bthr->threadBelow() == athr))
      throw EXPTNpolyCross("Invalid segment sort in thread end - bind");
   // action is taken only for points in the second polygon
   if (2 == _aseg->polyNo() == _bseg->polyNo())
   {
      // assuming that the polygons are not crossing ...
      if (!((athr->threadBelow() == bthr) && (bthr->threadAbove() == athr)))
         throw EXPTNpolyCross("Crossing input polygons in bind algo - end");
      // if right point is above and the neighbour above is from polygon 1
      if ((_aseg->lP()->y() <= _aseg->rP()->y()) &&
            (1 == athr->threadAbove()->cseg()->polyNo()))
         bindColl.update_BL(athr->threadAbove()->cseg(), _aseg->edge(), _aseg->rP());
      // if the right point is below and the neighbour below is from polygon 1
      if ((_bseg->lP()->y() >= _bseg->rP()->y()) &&
            (1 == bthr->threadBelow()->cseg()->polyNo()))
         bindColl.update_BL(bthr->threadBelow()->cseg(), _bseg->edge(), _bseg->rP());
   }
   // delete the threads
   sweepline.endThread(_aseg->threadID());
   sweepline.endThread(_bseg->threadID());
}

//==============================================================================
// TmEvent

polycross::TmEvent::TmEvent (polysegment* seg1, polysegment* seg2, byte shapeID):
      TEvent( shapeID )
{
   if       (seg1->rP() == seg2->lP())
   {
      _tseg1 = seg1; _tseg2 = seg2;
      _evertex = seg1->rP();
   }
   else if (seg2->rP() == seg1->lP())
   {
      _tseg1 = seg2; _tseg2 = seg1;
      _evertex = seg2->rP();
   }
   else
      throw EXPTNpolyCross("Invalid input segments in thread modify");

}

void polycross::TmEvent::sweep (XQ& eventQ, YQ& sweepline, ThreadList& threadl)
{
#ifdef BO2_DEBUG
   printf("Modify thread\n");
   BO_printseg(_tseg1)
   BO_printseg(_tseg2)
#endif
   if (0 == _tseg1->threadID())
      EXPTNpolyCross("Sorted segment expected here");
   SegmentThread* thr = sweepline.modifyThread(_tseg1->threadID(), _tseg2);

   // check for intersections of the neighbours with the new segment
   // and whether or not threads should be swapped
   TP* CP;
   if((CP = checkIntersect(thr->threadAbove()->cseg(), thr->cseg(), eventQ)))
   {
      if ((*CP) == *(_tseg2->lP()))
      {
         polysegment* aseg = thr->threadAbove()->cseg();
         int ori1 = orientation(aseg->lP(), aseg->rP(), _tseg1->lP());
         int ori2 = orientation(aseg->lP(), aseg->rP(), _tseg2->rP());
         if ((ori1 == ori2) || (0 == ori1 * ori2))
            threadl.push_back(_tseg2->threadID());
      }
   }

   if ((CP = checkIntersect(thr->cseg(), thr->threadBelow()->cseg(), eventQ)))
   {
      if ((*CP) == *(_tseg2->lP()))
      {
         polysegment* bseg = thr->threadBelow()->cseg();
         int ori1 = orientation(bseg->lP(), bseg->rP(), _tseg1->lP());
         int ori2 = orientation(bseg->lP(), bseg->rP(), _tseg2->rP());
         if ((ori1 == ori2) || (0 == ori1 * ori2))
            threadl.push_back(_tseg2->threadID());
      }
   }
}

void polycross::TmEvent::sweep2bind(YQ& sweepline, BindCollection& bindColl)
{
   if (0 == _tseg1->threadID())
      EXPTNpolyCross("Sorted segment expected here - bind");
   SegmentThread* thr = sweepline.modifyThread(_tseg1->threadID(), _tseg2);
   
   // action is taken only for points in the second polygon
   if (1 == _tseg1->polyNo() == _tseg2->polyNo()) return;
   
   // first for _tseg1 
   // if right point is above and the neighbour above is from polygon 1
   if ((_tseg1->lP()->y() <= _tseg1->rP()->y()) &&
        (1 == thr->threadAbove()->cseg()->polyNo()))
      bindColl.update_BL(thr->threadAbove()->cseg(), _tseg1->edge(), _tseg1->rP());
   // if the right point is below and the neighbour below is from polygon 1
   if ((_tseg1->lP()->y() >= _tseg1->rP()->y()) &&
        (1 == thr->threadBelow()->cseg()->polyNo()))
      bindColl.update_BL(thr->threadBelow()->cseg(), _tseg1->edge(), _tseg1->rP());
   
   // then for _tseg2
   // if left point is above and the neighbour above is from polygon 1
   if ((_tseg2->lP()->y() >= _tseg2->rP()->y()) &&
        (1 == thr->threadAbove()->cseg()->polyNo()))
      bindColl.update_BL(thr->threadAbove()->cseg(), _tseg2->edge(), _tseg2->lP());
   // if the left point is below and the neighbour below is from polygon 1
   if ((_tseg2->lP()->y() <= _tseg2->rP()->y()) &&
        (1 == thr->threadBelow()->cseg()->polyNo()))
      bindColl.update_BL(thr->threadBelow()->cseg(), _tseg2->edge(), _tseg2->lP());
}

//==============================================================================
// TcEvent

void polycross::TcEvent::sweep(XQ& eventQ, YQ& sweepline, ThreadList& threadl)
{
   if ((threadl.end() != std::find(threadl.begin(), threadl.end(),_threadAboveID)) ||
       (threadl.end() != std::find(threadl.begin(), threadl.end(),_threadBelowID)))
   {
#ifdef BO2_DEBUG
      printf("SKIP swapping threads %i and % i \n", _threadAboveID, _threadBelowID);
#endif
      return;
   }
#ifdef BO2_DEBUG
      printf("SWAPPING threads % i and %i\n", _threadAboveID, _threadBelowID);
#endif
   SegmentThread* below = sweepline.swapThreads(_threadAboveID, _threadBelowID);
   SegmentThread* above = below->threadAbove();
#ifdef BO2_DEBUG
   sweepline.report();
#endif
   // check for intersections with the new neighbours
   checkIntersect(below->cseg(), below->threadBelow()->cseg(), eventQ);
   checkIntersect(above->threadAbove()->cseg(), above->cseg(), eventQ);
}

bool polycross::TcEvent::operator == (const TcEvent& event) const
{
   return ((_threadAboveID == event._threadAboveID) &&
         (_threadBelowID == event._threadBelowID)   );
}

//==============================================================================
// EventVertex

void polycross::EventVertex::addEvent(TEvent* tevent, EventTypes etype)
{
   assert(NULL != tevent);
   Events& simevents = _events[etype];
   if (_crossE == etype)
   {
      // Don't double the cross events -> do we really need this after all?
      for (Events::const_iterator CE=simevents.begin();
            CE != simevents.end(); CE++)
         if (*(static_cast<TcEvent*>(*CE)) == *(static_cast<TcEvent*>(tevent)))
            return;
   }
#ifdef BO2_DEBUG
      printf("++New event added in vertex ( %i , %i ) on top of the pending %u +++++\n",
             _evertex->x(), _evertex->y(), unsigned(simevents.size()));
#endif
   simevents.push_back(tevent);
}


void polycross::EventVertex::sweep(YQ& sweepline, XQ& eventq)
{
#ifdef BO2_DEBUG
   printf("______________ POINT = ( %i , %i ) ___________\n", _evertex->x(), _evertex->y());
#endif
   for( int cetype = _endE; cetype <= _crossE; cetype++)
   {
      if (_events.end() != _events.find(cetype))
      {
         Events& simEvents = _events[cetype];
         while (!simEvents.empty())
         {
            TEvent* cevent = simEvents.front(); simEvents.pop_front();
            cevent->sweep(eventq, sweepline, _threadsSweeped);
         }
      }
   }
}

void polycross::EventVertex::sweep2bind(YQ& sweepline, BindCollection& bindColl)
{
#ifdef BO2_DEBUG
   printf("______________ POINT = ( %i , %i ) ___________\n", _evertex->x(), _evertex->y());
#endif
   for( int cetype = _endE; cetype <= _crossE; cetype++)
   {
      if (_events.end() != _events.find(cetype))
      {
         Events& simEvents = _events[cetype];
         while (!simEvents.empty())
         {
            TEvent* cevent = simEvents.front(); simEvents.pop_front();
            cevent->sweep2bind(sweepline, bindColl);
         }
      }
   }
}

//===========================================================================
// Segment Thread
polycross::polysegment* polycross::SegmentThread::set_cseg(polysegment* cs)
{
   polysegment* oldseg = _cseg;
   _cseg = cs;
   return oldseg;
}

//==============================================================================
// YQ
polycross:: YQ::YQ(DBbox& overlap)
{
   TP* bl = new TP(overlap.p1().x()-1, overlap.p1().y()-1);
   TP* br = new TP(overlap.p2().x()+1, overlap.p1().y()-1);
   TP* tl = new TP(overlap.p1().x()-1, overlap.p2().y()+1);
   TP* tr = new TP(overlap.p2().x()+1, overlap.p2().y()+1);
   _bottomSentinel = new BottomSentinel(new polysegment(bl, br, -1, 0));
   _cthreads[-2] = _bottomSentinel;
   _topSentinel = new TopSentinel(new polysegment(tl, tr, -1, 0));
   _cthreads[-1] = _topSentinel;
   _bottomSentinel->set_threadAbove(_topSentinel);
   _topSentinel->set_threadBelow(_bottomSentinel);
   _lastThreadID = 0;
}

/**
 * Insert a new segment thread in the YQ
 * @param startseg the first segment of the thread that became current
 * @return the created thread
 */
polycross::SegmentThread* polycross::YQ::beginThread(polycross::polysegment* startseg)
{
   if (0 != startseg->threadID())
      EXPTNpolyCross("Unsorted segment expected here");
   SegmentThread* above = _bottomSentinel;
   while (sCompare(startseg, above->cseg()) > 0)
      above = above->threadAbove();
   
   SegmentThread* below = above->threadBelow();
   SegmentThread* newthread = new SegmentThread(startseg, below, above);
   above->set_threadBelow(newthread);
   below->set_threadAbove(newthread);
   _cthreads[++_lastThreadID] = newthread;
   startseg->set_threadID(_lastThreadID);
   return newthread;
}

/**
 * Removes a segment thread from YQ and relinks its neighbours accordingly
 * @param threadID The ID of the thread to be removed
 * @return the thread that use to be below the one removed
 */
polycross::SegmentThread* polycross::YQ::endThread(unsigned threadID)
{
   // get the thread from the _cthreads
   Threads::iterator threadP = _cthreads.find(threadID);
   if (_cthreads.end() == threadP)
      EXPTNpolyCross("Segment thread not found in YQ - end");
   SegmentThread* thread = threadP->second;
   // relink above/below threads
   SegmentThread* nextT = thread->threadAbove();
   if (NULL == nextT)
      EXPTNpolyCross("Unable to remove the segment thread properly");
   nextT->set_threadBelow(thread->threadBelow());
   SegmentThread* prevT = thread->threadBelow();
   if (NULL == prevT)
      EXPTNpolyCross("Unable to remove the segment thread properly");
   prevT->set_threadAbove(thread->threadAbove());
   // erase it
   _cthreads.erase(threadP);
   return prevT;
}

/**
 * Replaces the current segment in a segment thread
 * @param threadID the target segment thread
 * @param newsegment the new segment - to replace the existing one
 * @return the segment thread
 */
polycross::SegmentThread* polycross::YQ::modifyThread(unsigned threadID, polysegment* newsegment)
{
   // get the thread from the _cthreads
   Threads::iterator threadP = _cthreads.find(threadID);
   if (_cthreads.end() == threadP)
      EXPTNpolyCross("Segment thread not found in YQ - modify");
   SegmentThread* thread = threadP->second;
   newsegment->set_threadID(threadID);
   polysegment* oldsegment = thread->set_cseg(newsegment);
   oldsegment->set_threadID(0);
   return thread;
}

/**
 * Swap the threads with the corresponding IDs
 * @param tAID The ID of the thread above before the swap
 * @param tBID The ID of the thread below before the swap
 * @return pointer to the thread below after the swap
 */
polycross::SegmentThread* polycross::YQ::swapThreads(unsigned tAID, unsigned tBID)
{
   // get the threads from the _cthreads
   Threads::iterator tiAbove = _cthreads.find(tAID);
   Threads::iterator tiBelow = _cthreads.find(tBID);
   if (_cthreads.end() == tiAbove)
      EXPTNpolyCross("Segment thread not found in YQ - swap");
   if (_cthreads.end() == tiBelow)
      EXPTNpolyCross("Segment thread not found in YQ - swap");
   SegmentThread* tAbove = tiAbove->second;
   SegmentThread* tBelow = tiBelow->second;
   // relink above/below threads
   if (tAbove != tBelow->threadAbove())
      EXPTNpolyCross("Unable to swap the segment threads properly");
   if (tBelow != tAbove->threadBelow())
      EXPTNpolyCross("Unable to swap the segment threads properly");
   // first  fix the pointers of the neighboring threads
   tBelow->threadBelow()->set_threadAbove(tAbove);
   tAbove->threadAbove()->set_threadBelow(tBelow);
   // now swap above pointers ...
   tBelow->set_threadAbove(tAbove->threadAbove());
   tAbove->set_threadAbove(tBelow); // <===!
   // ... and below pointers ...
   tAbove->set_threadBelow(tBelow->threadBelow());
   tBelow->set_threadBelow(tAbove); // <===!
   
   return tAbove;
}

polycross::SegmentThread* polycross::YQ::getThread(unsigned tID)
{
   Threads::iterator ti = _cthreads.find(tID);
   if (_cthreads.end() == ti)
      EXPTNpolyCross("Segment thread not found in YQ - get");
   return ti->second;
}

int polycross::YQ::sCompare(const polysegment* seg0, const polysegment* seg1)
{
   // Current point is always lp of seg0
   if (seg0 == seg1)
      EXPTNpolyCross("Different segments expected here");
   //
   int ori;
   ori = orientation(seg1->lP(), seg1->rP(), seg0->lP());
   if (ori != 0) return ori;
   // if it is a crossing point -> compare the slope
   ori = orientation(seg1->lP(), seg1->rP(), seg0->rP());
   if (ori != 0) return ori;
   // if it is still the same => we have coinciding segments
   int order;
   // like that .... malta cross case fails !
/*   if       (*(seg0->rP()) != *(seg1->rP()))
      order = xyorder(seg0->rP(), seg1->rP());
   else if  (*(seg0->lP()) != *(seg1->lP()))
      order = xyorder(seg1->lP(), seg0->lP());*/
   // or like that - both ways it should work
   if       (*(seg0->lP()) != *(seg1->lP()))
         order = xyorder(seg0->lP(), seg1->lP());
   else if  (*(seg0->rP()) != *(seg1->rP()))
      order = xyorder(seg0->rP(), seg1->rP());
   else
      order = (seg0->edge() > seg1->edge()) ? 1 : -1;
   if (0 == order)
      EXPTNpolyCross("Segments undistinguishable");
   return order;
}


void polycross::YQ::report()
{
   printf("^^^^^^^Threads currently in the YQ - from top to bottom^^^^^^^^^\n");
   SegmentThread* current = _topSentinel->threadBelow();
   while (current != _bottomSentinel)
   {
      BO_printseg(current->cseg());
      current = current->threadBelow();
   }
}

//==============================================================================
// XQ
polycross::XQ::XQ( const segmentlist& seg1, const segmentlist& seg2 ) :
      _overlap(*(seg1[0]->lP()))
{
   _osl1 = &seg1;
   _osl2 = &seg2;
   _xqueue = avl_create(E_compare, NULL, NULL);
   createEvents(seg1,1);
   createEvents(seg2,2);
   _sweepline = new YQ(_overlap);
}

void polycross::XQ::createEvents(const segmentlist& seg, byte shapeID)
{
   unsigned s1, s2;
   TEvent*      evt;
   EventTypes   etype;
   for(s1 = 0, s2 = 1 ; s1 < seg.size(); s1++, ++s2 %= seg.size())
   {
      // determine the type of event from the neighboring segments
      // and create the thread event
      if (seg[s1]->lP() == seg[s2]->lP())
      {
         evt = new TbEvent(seg[s1], seg[s2], shapeID);
         etype = _beginE;
      }
      else if (seg[s1]->rP() == seg[s2]->rP())
      {
         evt = new TeEvent(seg[s1], seg[s2], shapeID);
         etype = _endE;
      }
      else
      {
         evt = new TmEvent(seg[s1], seg[s2], shapeID);
         etype = _modifyE;
      }
      // update overlapping box
      _overlap.overlap(*(seg[s1]->lP()));
      _overlap.overlap(*(seg[s1]->rP()));
      // now create the vertex with the event inside
      EventVertex* vrtx = new EventVertex(evt->evertex());
      // and try to stick it in the AVL tree
      void** retitem =  avl_probe(_xqueue,vrtx);
      if ((*retitem) != vrtx)
         // coinsiding vertexes from different polygons
         delete(vrtx);
      // finally add the event itself
      static_cast<EventVertex*>(*retitem)->addEvent(evt,etype);
   }
}

void polycross::XQ::addCrossEvent(TP* CP, unsigned thr1, unsigned thr2)
{
   TcEvent* evt = new TcEvent(CP, thr1, thr2);
   // now create the vertex with the event inside
   EventVertex* vrtx = new EventVertex(evt->evertex());
   // and try to stick it in the AVL tree
   void** retitem =  avl_probe(_xqueue,vrtx);
   if ((*retitem) != vrtx)
   {
      // coinsiding vertexes from different polygons
      delete(vrtx);
   }
   static_cast<EventVertex*>(*retitem)->addEvent(evt, _crossE);
}

int polycross::XQ::E_compare( const void* v1, const void* v2, void*)
{
   const TP*  p1 = (*(EventVertex*)v1)();
   const TP*  p2 = (*(EventVertex*)v2)();
   return xyorder(p1,p2);
}

void polycross::XQ::sweep()
{
// see the comment around find parameter in the E_compare
#ifdef BO2_DEBUG
      printf("***************** START SWIPING ******************\n");
#endif
   EventVertex* evtlist;
   avl_traverser trav;
   while (NULL != avl_t_first(&trav,_xqueue))
   {
      evtlist = (EventVertex*)trav.avl_node->avl_data;
      evtlist->sweep(*_sweepline, *this);
      avl_delete(_xqueue,evtlist);
   }
}

void polycross::XQ::sweep2bind(BindCollection& bindColl) {
#ifdef BO2_DEBUG
      printf("***************** START BIND SWIPING ******************\n");
#endif
   EventVertex* evtlist;
   avl_traverser trav;
   while (NULL != avl_t_first(&trav,_xqueue))
   {
      evtlist = (EventVertex*)trav.avl_node->avl_data;
      evtlist->sweep2bind(*_sweepline, bindColl);
      avl_delete(_xqueue,evtlist);
   }
}

//==============================================================================
// BindCollection
void polycross::BindCollection::update_BL(polysegment* outseg, unsigned poly1seg,
                                          const TP* poly1pnt)
{
   unsigned poly0seg = outseg->edge();
   // calculate the distance between the point poly1pnt and the segment poly0seg
   // first calculate the coefficients of the poly0seg line equation
   real A = outseg->rP()->y() - outseg->lP()->y();
   real B = outseg->lP()->x() - outseg->rP()->x();
   real C = -(A*outseg->lP()->x() + B*outseg->lP()->y());
   assert((A != 0) || (B != 0));
   // find the point poly0pnt where the perpendicular from poly1pnt to
   // poly0seg crosses the latter
   real line1 = A*poly1pnt->x() + B*poly1pnt->y() + C;
   real denom = A*A + B*B;
   real X = poly1pnt->x() - (A / denom) * line1;
   real Y = poly1pnt->y() - (B / denom) * line1;
   TP* poly0pnt = new TP((int)rint(X),(int)rint(Y));
   // if the point lies inside the segment
   if (getLambda(outseg->lP(), outseg->rP(), poly0pnt) >= 0)
   {
      // get the distance
      real distance = fabs(line1 / sqrt(denom));
      // and if it's shorter than already stored one, for this segment
      // or of no distance calculated fro this segment
      if (is_shorter(poly0seg, distance))
         // store the data
         _blist.push_back(new BindSegment(poly0seg, poly1seg, poly0pnt, poly1pnt, distance));
      else delete poly0pnt;
   }
   else delete poly0pnt;
}

bool polycross::BindCollection::is_shorter(unsigned segno, real dist)
{
   for (BindList::iterator BI = _blist.begin(); BI != _blist.end(); BI++)
   {
      if ((*BI)->poly0seg() == segno)
      {
         if ((*BI)->distance() > dist)
         {
            delete (*BI);
            _blist.erase(BI);
            return true;
         }
         else return false;
      }
   }
   return true;
}

polycross::BindSegment* polycross::BindCollection::get_highest()
{
   BindList::iterator BI = _blist.begin();
   BindSegment* shseg = *BI;
   while (++BI != _blist.end())
      if ((*BI)->poly1pnt()->y() > shseg->poly1pnt()->y()) shseg = *BI;
   return shseg;
}
