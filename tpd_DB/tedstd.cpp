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
//                                                                          =
//   This file is a part of Toped project (C) 2001-2007 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Sun May 22 15:43:49 BST 2005
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Basic definitions and file handling of TDT data base
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include "tpdph.h"
#include <math.h>
#include <sstream>
#include "tedstd.h"
#include "ttt.h"
#include "outbox.h"
#include "tedesign.h"

//-----------------------------------------------------------------------------
// class PSegment
//-----------------------------------------------------------------------------
PSegment::PSegment(TP p1,TP p2)
{
   _A = p2.y() - p1.y();
   _B = p1.x() - p2.x();
   _C = -(_A*p1.x() + _B*p1.y());
   _angle = 0;
}

byte PSegment::crossP(PSegment seg, TP& crossp)
{
   // segments will coinside if    A1/A2 == B1/B2 == C1/C2
   // segments will be parallel if A1/A2 == B1/B2 != C1/C2
   if (0 == (_A*seg._B - seg._A*_B)) return 1;
   real X,Y;
   if ((0 != _A) && (0 != seg._B)) {
      X = - ((_C - (_B/seg._B) * seg._C) / (_A - (_B/seg._B) * seg._A));
      Y = - ((seg._C - (seg._A/_A) * _C) / (seg._B - (seg._A/_A) * _B));
   }
   else if ((0 != _B) && (0 != seg._A)) {
      X = - (seg._C - (seg._B/_B) * _C) / (seg._A - (seg._B/_B) * _A);
      Y = - (_C - (_A/seg._A) * seg._C) / (_B - (_A/seg._A) * seg._B);
   }
   else assert(0);
   crossp.setX((int4b)rint(X));
   crossp.setY((int4b)rint(Y));
   return 0;
}

PSegment* PSegment::ortho(TP p)
{
   PSegment* seg = DEBUG_NEW PSegment(-_B, _A, _B*p.x() - _A*p.y());
   return seg;
}

PSegment* PSegment::parallel(TP p)
{
   PSegment* seg = DEBUG_NEW PSegment( _A, _B, -_A*p.x() - _B*p.y());
   return seg;
}


//-----------------------------------------------------------------------------
// class TEDfile
//-----------------------------------------------------------------------------
laydata::TEDfile::TEDfile(const char* filename, laydata::TdtLibDir* tedlib)  // reading
{
   _numread = 0;_position = 0;_design = NULL;
   _TEDLIB = tedlib;
	std::string fname(convertString(filename));
	if (NULL == (_file = fopen(fname.c_str(), "rb"))) {
      std::string news = "File \"";
      news += filename; news += "\" not found or unaccessable";
      tell_log(console::MT_ERROR,news);
      _status = false; return;
   }
   try
   {
      getFHeader();
   }
   catch (EXPTNreadTDT)
   {
      fclose(_file);
      _status = false;
      return;
   }
   _status = true;
}

void laydata::TEDfile::getFHeader()
{
   // Get the leading string
   std::string _leadstr = getString();
   if (TED_LEADSTRING != _leadstr) throw EXPTNreadTDT("Bad leading record");
   // Get format revision
   getRevision();
   // Get file time stamps
   getTime(/*timeCreated, timeSaved*/);
//   checkIntegrity();
}

void laydata::TEDfile::read(int libRef)
{
   if (tedf_DESIGN != getByte()) throw EXPTNreadTDT("Expecting DESIGN record");
   std::string name = getString();
   real         DBU = getReal();
   real          UU = getReal();
   tell_log(console::MT_DESIGNNAME, name);
   if (libRef > 0)
      _design = DEBUG_NEW TdtLibrary(name, DBU, UU, libRef);
   else
      _design = DEBUG_NEW TdtDesign(name,_created, _lastUpdated, DBU,UU);
   _design->read(this);
   //Design end marker is read already in TdtDesign so don't search it here
   //byte designend = getByte();
}

laydata::TEDfile::TEDfile(std::string& filename, laydata::TdtLibDir* tedlib)
{ //writing
   _design = (*tedlib)();
   _revision=TED_CUR_REVISION;_subrevision=TED_CUR_SUBREVISION;
   _TEDLIB = tedlib;
	std::string fname(convertString(filename));
   if (NULL == (_file = fopen(fname.c_str(), "wb"))) {
      std::string news = "File \"";
      news += filename.c_str(); news += "\" can not be created";
      tell_log(console::MT_ERROR,news);
      return;
   }
   putString(TED_LEADSTRING);
   putRevision();
   putTime();
   static_cast<laydata::TdtDesign*>(_design)->write(this);
   fclose(_file);
}

void laydata::TEDfile::cleanup()
{
   if (NULL != _design) delete _design;
}

byte laydata::TEDfile::getByte()
{
   byte result;
   byte length = sizeof(byte);
   if (1 != (_numread = fread(&result, length, 1, _file)))
      throw EXPTNreadTDT("Wrong number of bytes read");
   _position += length;
   return result;
}

word laydata::TEDfile::getWord()
{
   word result;
   byte length = sizeof(word);
   if (1 != (_numread = fread(&result, length, 1, _file)))
      throw EXPTNreadTDT("Wrong number of bytes read");
   _position += length;
   return result;
}

int4b laydata::TEDfile::get4b()
{
   int4b result;
   byte length = sizeof(int4b);
   if (1 != (_numread = fread(&result, length, 1, _file)))
      throw EXPTNreadTDT("Wrong number of bytes read");
   _position += length;
   return result;
}

real laydata::TEDfile::getReal() {
   real result;
   byte length = sizeof(real);
   if (1 != (_numread = fread(&result, length, 1, _file)))
      throw EXPTNreadTDT("Wrong number of bytes read");
   _position += length;
   return result;
}

std::string laydata::TEDfile::getString()
{
   std::string str;
   byte length = getByte();
   char* strc = DEBUG_NEW char[length+1];
   _numread = fread(strc, length, 1, _file);
   strc[length] = 0x00;
   if (_numread != 1)
   {
      delete[] strc;
      throw EXPTNreadTDT("Wrong number of bytes read");
   }
   _position += length; str = strc;
   delete[] strc;
   return str;
}

TP laydata::TEDfile::getTP()
{
   int4b x = get4b();
   int4b y = get4b();
   return TP(x,y);
}

CTM laydata::TEDfile::getCTM()
{
   real _a  = getReal();
   real _b  = getReal();
   real _c  = getReal();
   real _d  = getReal();
   real _tx = getReal();
   real _ty = getReal();
   return CTM(_a, _b, _c, _d, _tx, _ty);
}

void laydata::TEDfile::getTime()
{
   tm broken_time;
   if (tedf_TIMECREATED  != getByte()) throw EXPTNreadTDT("Expecting TIMECREATED record");
   broken_time.tm_mday = get4b();
   broken_time.tm_mon  = get4b();
   broken_time.tm_year = get4b();
   broken_time.tm_hour = get4b();
   broken_time.tm_min  = get4b();
   broken_time.tm_sec  = get4b();
   broken_time.tm_isdst = -1;
   _created = mktime(&broken_time);
   if (tedf_TIMEUPDATED  != getByte()) throw EXPTNreadTDT("Expecting TIMEUPDATED record");
   broken_time.tm_mday = get4b();
   broken_time.tm_mon  = get4b();
   broken_time.tm_year = get4b();
   broken_time.tm_hour = get4b();
   broken_time.tm_min  = get4b();
   broken_time.tm_sec  = get4b();
   broken_time.tm_isdst = -1;
   _lastUpdated = mktime(&broken_time);
}

void laydata::TEDfile::getRevision()
{
   if (tedf_REVISION  != getByte()) throw EXPTNreadTDT("Expecting REVISION record");
   _revision = getWord();
   _subrevision = getWord();
   std::ostringstream ost;
   ost << "TDT format revision: " << _revision << "." << _subrevision;
   tell_log(console::MT_INFO,ost.str());
   if ((_revision != TED_CUR_REVISION) || (_subrevision > TED_CUR_SUBREVISION))
      throw EXPTNreadTDT("The TDT revision is not maintained by this version of Toped");
}

void laydata::TEDfile::putWord(const word data) {
   fwrite(&data,2,1,_file);
}

void laydata::TEDfile::put4b(const int4b data) {
   fwrite(&data,4,1,_file);
}

void laydata::TEDfile::putReal(const real data) {
   fwrite(&data, sizeof(real), 1, _file);
}


void laydata::TEDfile::putTime()
{
   time_t ctime = static_cast<laydata::TdtDesign*>(_design)->created();
   tm* broken_time = localtime(&ctime);
   putByte(tedf_TIMECREATED);
   put4b(broken_time->tm_mday);
   put4b(broken_time->tm_mon);
   put4b(broken_time->tm_year);
   put4b(broken_time->tm_hour);
   put4b(broken_time->tm_min);
   put4b(broken_time->tm_sec);
   //
   _lastUpdated = time(NULL);
   static_cast<laydata::TdtDesign*>(_design)->_lastUpdated = _lastUpdated;
   broken_time = localtime(&_lastUpdated);
   putByte(tedf_TIMEUPDATED);
   put4b(broken_time->tm_mday);
   put4b(broken_time->tm_mon);
   put4b(broken_time->tm_year);
   put4b(broken_time->tm_hour);
   put4b(broken_time->tm_min);
   put4b(broken_time->tm_sec);
}

void laydata::TEDfile::putRevision()
{
   putByte(tedf_REVISION);
   putWord(_revision);
   putWord(_subrevision);
}

void laydata::TEDfile::putTP(const TP* p)
{
   put4b(p->x()); put4b(p->y());
}

void laydata::TEDfile::putCTM(const CTM matrix)
{
   putReal(matrix.a());
   putReal(matrix.b());
   putReal(matrix.c());
   putReal(matrix.d());
   putReal(matrix.tx());
   putReal(matrix.ty());
}

void laydata::TEDfile::putString(std::string str)
{
//   byte len = str.length();
//   fwrite(&len, 1,1, _file);
   putByte(str.length());
   fputs(str.c_str(), _file);
}

void laydata::TEDfile::registerCellWritten(std::string cellname)
{
   _childnames.insert(cellname);
}

bool laydata::TEDfile::checkCellWritten(std::string cellname)
{
   if (_childnames.end() == _childnames.find(cellname))
      return false;
   else
      return true;
}

laydata::CellDefin laydata::TEDfile::linkCellRef(std::string cellname)
{
   // register the name of the referenced cell in the list of children
   _childnames.insert(cellname);
   CellList::const_iterator striter = _design->_cells.find(cellname);
   laydata::CellDefin celldef = NULL;
   // link the cells instances with their definitions
   if (_design->_cells.end() == striter)
   {
   //   if (_design->checkCell(name))
   //   {
      // search the cell in the libraries because it's not in the DB
      if (!_TEDLIB->getLibCellRNP(cellname, celldef))
      {
         // Attention! In this case we've parsed a cell reference, before
         // the cell is defined. This might means:
         //   1. Cell is referenced, but not defined - i.e. library cell, but
         //      library is not loaded
         //   2. Circular reference ! Cell1 contains a reference of Cell2,
         //      that in turn contains a reference of Cell1. This is not allowed
         // We can not make a decision yet, because the entire file has not been
         // parsed yet. That is why we are assigning a default cell to the
         // referenced structure here in order to continue the parsing, and when
         // the entire file is parced the cell references without a proper pointer
         // to the structure need to be flaged as warning in case 1 and as error
         // in case 2.
         celldef = _TEDLIB->addDefaultCell(cellname, false);
      }
      else
         celldef->parentFound();
   }
   else
   {
      celldef = striter->second;
      assert(NULL != celldef);
      celldef->parentFound();
   }
   return celldef;
}

void laydata::TEDfile::getCellChildNames(NameSet& cnames) {
   // Be very very careful with the copy constructors and assignment of the
   // standard C++ lib containers. Here it seems OK.
   cnames = _childnames;
   //for (NameSet::const_iterator CN = _childnames.begin();
   //                              CN != _childnames.end() ; CN++)
   //   cnames->instert(*CN);
   _childnames.clear();
}

bool laydata::pathConvert(pointlist& plist, word numpoints, int4b begext, int4b endext )
{
   TP P1 = plist[0];
   // find the first neighboring point which is not equivalent to P1
   int fnbr = 1;
   while ((fnbr < numpoints) && (P1 == plist[fnbr]))
      fnbr++;
   // get out with error, because the wire has effectively a single point and there is
   // no way on earth to find out in which direction it should be expanded
   if (fnbr == numpoints) return false;
   TP P2 = plist[fnbr];

   double sdX = P2.x() - P1.x();
   double sdY = P2.y() - P1.y();
   // The sign - a bit funny way - described in layout canvas
   int sign = ((sdX * sdY) >= 0) ? 1 : -1;
   double length = sqrt(sdY*sdY + sdX*sdX);
   assert(length);
   int4b y0 = (int4b) rint(P1.y() - sign*((begext*sdY)/length));
   int4b x0 = (int4b) rint(P1.x() - sign*((begext*sdX)/length));
//
   P2 = plist[numpoints-1];
   // find the first neighboring point which is not equivalent to P1
   fnbr = numpoints - 2;
   while ((P2 == plist[fnbr]) && (fnbr > 0))
      fnbr--;
   // assert, because if it was found above, it should exists!
   assert(fnbr >= 0);
   P1 = plist[fnbr];

   P1 = plist[numpoints-2];
   sdX = P2.x() - P1.x();
   sdY = P2.y() - P1.y();
   sign = ((sdX * sdY) >= 0) ? 1 : -1;
   length = sqrt(sdY*sdY + sdX*sdX);
   int4b yn = (int4b) rint(P2.y() + sign*((endext*sdY)/length));
   int4b xn = (int4b) rint(P2.x() + sign*((endext*sdX)/length));

   plist[0].setX(x0);
   plist[0].setY(y0);
   plist[numpoints-1].setX(xn);
   plist[numpoints-1].setY(yn);

   return true;
}


//=============================================================================

laydata::WireContour::WireContour(const int4b* ldata, unsigned lsize, word width) :
   _ldata(ldata), _lsize(lsize), _width(width)
{
   endPnts(0,1, true);
   for (unsigned i = 1; i < _lsize - 1; i++)
   {
      switch (chkCollinear(i-1, i, i+1))
      {
         case 0: // points not in one line
            mdlPnts(i-1,  i, i+1 ); break;
         case 1: //i-1 and i coincide
            endPnts( i, i+1, true); break;
         case 2: // i and i+1 coincide
            endPnts(i-1,  i,false); break;
         case 3: // collinear points
            colPnts(i-1,  i, i+1 ); break;
         case 4: // 3 points in one line with i2 in the middle
            mdlPnts(i-1,  i, i+1 ); break;
         case 5: // 3 coinciding points
                                    break;
         default: assert(false);
      }
   }
   endPnts(_lsize -2, _lsize -1, false);
}

/*!
 * Dumps the generated wire contour in the @plist vector. For optimal performance the
 * vector object shall be properly allocated using something line reserve(csize())
 * before calling this method. The method will cope with @plist vectors which already
 * contain some data. It will just add the contour at the end of the @plist.
 */
void laydata::WireContour::getVectorData(pointlist& plist)
{
   for (PointList::const_iterator CP = _cdata.begin(); CP != _cdata.end(); CP++)
   {
      plist.push_back(*CP);
   }
}

/*!
 * Dumps the generated wire contour in the @contour array. The array must be allocated
 * before calling this function. The size of the array can be taken from the function
 * csize()
 */
void laydata::WireContour::getArrayData(int4b* contour)
{
   word index = 0;
   for (PointList::const_iterator CP = _cdata.begin(); CP != _cdata.end(); CP++)
   {
      contour[index++] = CP->x();
      contour[index++] = CP->y();
   }
}

DBbox laydata::WireContour::getCOverlap()
{
   PointList::const_iterator CP = _cdata.begin();
   DBbox ovl(*CP);
   while (CP != _cdata.end())
   {
      ovl.overlap(*CP); CP++;
   }
   return ovl;
}

void laydata::WireContour::mdlPnts(word i1, word i2, word i3)
{
   double    w = _width/2;
   i1 *= 2; i2 *= 2; i3 *= 2;
   double  x32 = _ldata[i3  ] - _ldata[i2  ];
   double  x21 = _ldata[i2  ] - _ldata[i1  ];
   double  y32 = _ldata[i3+1] - _ldata[i2+1];
   double  y21 = _ldata[i2+1] - _ldata[i1+1];
   double   L1 = sqrt(x21*x21 + y21*y21); //the length of segment 1
   double   L2 = sqrt(x32*x32 + y32*y32); //the length of segment 2
   double denom = x32 * y21 - x21 * y32;
   if ((0 == denom) || (0 == L1) || (0 == L2)) return;
   // the corrections
   double xcorr = w * ((x32 * L1 - x21 * L2) / denom);
   double ycorr = w * ((y21 * L2 - y32 * L1) / denom);
   _cdata.push_front(TP((int4b) rint(_ldata[i2  ] - xcorr),(int4b) rint(_ldata[i2+1] + ycorr)));
   _cdata.push_back (TP((int4b) rint(_ldata[i2  ] + xcorr),(int4b) rint(_ldata[i2+1] - ycorr)));
}

void laydata::WireContour::endPnts(word i1, word i2, bool first)
{
   double     w = _width/2;
   i1 *= 2; i2 *= 2;
   double denom = first ? (_ldata[i2  ] - _ldata[i1  ]) : (_ldata[i1  ] - _ldata[i2  ]);
   double   nom = first ? (_ldata[i2+1] - _ldata[i1+1]) : (_ldata[i1+1] - _ldata[i2+1]);
   double xcorr, ycorr; // the corrections
   if ((0 == nom) && (0 == denom)) return;
   double signX = (  nom > 0) ? (first ? 1.0 : -1.0) : (first ? -1.0 : 1.0);
   double signY = (denom > 0) ? (first ? 1.0 : -1.0) : (first ? -1.0 : 1.0);
   if      (0 == denom) // vertical
   {
      xcorr =signX * w ; ycorr = 0        ;
   }
   else if (0 == nom  )// horizontal |----|
   {
      xcorr = 0        ; ycorr = signY * w;
   }
   else
   {
      double sl   = nom / denom;
      double sqsl = signY*sqrt( sl*sl + 1);
      xcorr = rint(w * (sl / sqsl));
      ycorr = rint(w * ( 1 / sqsl));
   }
   word it = first ? i1 : i2;
   _cdata.push_front(TP((int4b) rint(_ldata[it  ] - xcorr),(int4b) rint(_ldata[it+1] + ycorr)));
   _cdata.push_back (TP((int4b) rint(_ldata[it  ] + xcorr),(int4b) rint(_ldata[it+1] - ycorr)));
}

byte laydata::WireContour::chkCollinear(word i1, word i2, word i3)
{
   if ( 0 != orientation(i1, i2, i3)) return 0; // points not in one line
   if (_ldata[i1] == _ldata[i3]) return 3;
   float lambda1 = getLambda  (i3, i2, i1);
   float lambda2 = getLambda  (i1, i2, i3);
   if ((0 == lambda1) && (0 == lambda2)) return 5; // 3 coinciding points
   if ((0 <  lambda1) || (0 <  lambda2)) return 3; // collinear points
   if (0 == lambda1) return 1; //i2 and i3 coincide
   if (0 == lambda2) return 2; //i2 and i1 coincide
   return 4; // 3 points in one line sequenced with i2 in the middle
}

void laydata::WireContour::colPnts(word i1, word i2, word i3)
{
   TP extPnt = mdlCPnt(i1, i2);
   // Now - this is cheating! We're altering temporary one the central line
   // points and the reason is - to use the existing methods which deal with
   // point indexes
   TP swap(_ldata[2*i2], _ldata[2*i2 + 1]);
   const_cast<int4b*>(_ldata)[2*i2  ] = extPnt.x();
   const_cast<int4b*>(_ldata)[2*i2+1] = extPnt.y();
   endPnts(i1, i2,false);
   endPnts(i2 ,i3,true );
   const_cast<int4b*>(_ldata)[2*i2  ] = swap.x();
   const_cast<int4b*>(_ldata)[2*i2+1] = swap.y();
}

TP laydata::WireContour::mdlCPnt(word i1, word i2)
{
   i1 *= 2; i2 *= 2;
   double    w = _width / 2;
   double   x21 = _ldata[i2]   - _ldata[i1]  ;
   double   y21 = _ldata[i2+1] - _ldata[i1+1];
   double    L1 = sqrt(x21*x21 + y21*y21); //the length of the segment
   assert(L1 != 0.0);
   double xcorr = (w * x21)  / L1;
   double ycorr = (w * y21)  / L1;
   return TP((int4b) rint(_ldata[i2] + xcorr), (int4b) rint(_ldata[i2+1] + ycorr));
}

int laydata::WireContour::orientation(word i1, word i2, word i3)
{
   i1 *= 2; i2 *= 2; i3 *=2;
   // twice the "oriented" area of the enclosed triangle
   real area = (real(_ldata[i1]) - real(_ldata[i3])) * (real(_ldata[i2+1]) - real(_ldata[i3+1]))
             - (real(_ldata[i2]) - real(_ldata[i3])) * (real(_ldata[i1+1]) - real(_ldata[i3+1]));
   if (0 == area) return 0;
   else
      return (area > 0) ? 1 : -1;
}

float laydata::WireContour::getLambda(word i1, word i2, word ii)
{
   i1 *= 2; i2 *= 2; ii *=2;
   float denomX = _ldata[i2  ] - _ldata[ii  ];
   float denomY = _ldata[i2+1] - _ldata[ii+1];
   float lambda;
   if      (0 != denomX) lambda = real(_ldata[ii  ] - _ldata[i1  ]) / denomX;
   else if (0 != denomY) lambda = real(_ldata[ii+1] - _ldata[i1+1]) / denomY;
   // point coincides with the lp vertex of the segment
   else lambda = 0;
   return lambda;
}

//=============================================================================
/*!
 * Takes the original wire central line @parray, makes the appropriate transformations
 * using the @translation and stores the resulting wire in _ldata. Then it creates the
 * WireContour object and initializes it with the transformed data.
 */
laydata::WireContourAux::WireContourAux(const int4b* parray, unsigned lsize, const word width, const CTM& translation)
{
   _ldata = DEBUG_NEW int[2 * lsize];
   for (unsigned i = 0; i < lsize; i++)
   {
      TP cpoint(parray[2*i], parray[2*i+1]);
      cpoint *= translation;
      _ldata[2*i  ] = cpoint.x();
      _ldata[2*i+1] = cpoint.y();
   }
   _wcObject = DEBUG_NEW laydata::WireContour(_ldata, lsize, width);
}

/*!
 * Accelerates the WireContour usage with poointlist input data. Converts the @plist
 * into array format and stores the result in _ldata. Then creates the
 * WireContour object and initializes it with the _ldata array.
 */
laydata::WireContourAux::WireContourAux(const pointlist& plist, const word width)
{
   word psize = plist.size();
   _ldata = DEBUG_NEW int[2 * psize];
   for (unsigned i = 0; i < psize; i++)
   {
      _ldata[2*i  ] = plist[i].x();
      _ldata[2*i+1] = plist[i].y();
   }
   _wcObject = DEBUG_NEW laydata::WireContour(_ldata, psize, width);
}

laydata::WireContourAux::WireContourAux(const pointlist& plist, const word width, const TP extraP)
{
   word psize = plist.size() + 1;
   _ldata = DEBUG_NEW int[2 * psize];
   for (unsigned i = 0; i < (psize - 1); i++)
   {
      _ldata[2*i  ] = plist[i].x();
      _ldata[2*i+1] = plist[i].y();
   }
   _ldata[2*psize - 2] = extraP.x();
   _ldata[2*psize - 1] = extraP.y();

   _wcObject = DEBUG_NEW laydata::WireContour(_ldata, psize, width);
}

/*!
 * Dumps the wire central line and the contour generated by _wxObject in @plist
 * vector in a format which can be used directly by the methods of the basic
 * renderer. The @plist must be empty.
 * The format is: plist[0].x() returns the number of the central line points;
 * plist[0].y() returns the number of the wire contour points. The central
 * line points start from plist[1]. The contour points - follow.
 */
void laydata::WireContourAux::getRenderingData(pointlist& plist)
{
   assert(_wcObject);
   assert(0 == plist.size());
   word lsize = _wcObject->lsize();
   word csize = _wcObject->csize();
   plist.reserve(lsize + csize + 1);
   plist.push_back(TP(lsize, csize));
   for (int i = 0; i < lsize; i++)
      plist.push_back(TP(_ldata[2*i], _ldata[2*i+1]));
   _wcObject->getVectorData(plist);
}

void laydata::WireContourAux::getLData(pointlist& plist)
{
   assert(_wcObject);
   assert(0 == plist.size());
   word lsize = _wcObject->lsize();
   plist.reserve(lsize);
   for (int i = 0; i < lsize; i++)
      plist.push_back(TP(_ldata[2*i], _ldata[2*i+1]));
}

void laydata::WireContourAux::getCData(pointlist& plist)
{
   assert(_wcObject);
   assert(0 == plist.size());
   plist.reserve(_wcObject->csize());
   _wcObject->getVectorData(plist);
}


laydata::WireContourAux::~WireContourAux()
{
   delete _wcObject;
   delete [] _ldata;
}
