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
//   This file is a part of Toped project (C) 2001-2009 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Sun Jan 11 2008
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: OpenGL renderer
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include "tpdph.h"
#include "tenderer.h"
#include "viewprop.h"
#include "tedat.h"

// Themporary - to clarify the possible proper usage of VBO's
//#define USE_VBOS

GLUtriangulatorObj   *TeselPoly::tenderTesel = NULL;

//=============================================================================
//
TeselChunk::TeselChunk(const TeselVertices& data, GLenum type, unsigned offset)
{
   _size = data.size();
   _index_seq = DEBUG_NEW unsigned[_size];
   word li = 0;
   for(TeselVertices::const_iterator CVX = data.begin(); CVX != data.end(); CVX++)
      _index_seq[li++] = *CVX + offset;
   _type = type;
}

TeselChunk::TeselChunk(const TeselChunk* data, unsigned offset)
{
   _size = data->size();
   _type = data->type();
   _index_seq = new unsigned[_size];
   const unsigned* copy_seq = data->index_seq();
   for(unsigned i = 0; i < _size; i++)
      _index_seq[i] = copy_seq[i] + offset;
}

TeselChunk::TeselChunk(const int* data, unsigned size, unsigned offset)
{ // used for wire tesselation explicitly
   _size = size;
   _type = GL_QUAD_STRIP;
   assert(0 ==(size % 2));
   _index_seq = DEBUG_NEW unsigned[_size];
   word findex = 0;
   word bindex = _size;
   for (word i = 0; i < _size / 2; i++)
   {
      _index_seq[2*i  ] = (findex++) + offset;
      _index_seq[2*i+1] = (--bindex) + offset;
   }
}

TeselChunk::~TeselChunk()
{
   delete [] _index_seq;
}
//=============================================================================
//

TeselTempData::TeselTempData(unsigned offset) :_the_chain(NULL), _cindexes(),
           _all_ftrs(0), _all_ftfs(0), _all_ftss(0), _offset(offset)
{}

TeselTempData::TeselTempData(TeselChain* tc) : _the_chain(tc), _cindexes(),
           _all_ftrs(0), _all_ftfs(0), _all_ftss(0), _offset(0)
{}

void TeselTempData::storeChunk()
{
   TeselChunk* achunk = DEBUG_NEW TeselChunk(_cindexes, _ctype, _offset);
   _the_chain->push_back(achunk);
   switch (_ctype)
   {
      case GL_TRIANGLE_FAN   : _all_ftfs++; break;
      case GL_TRIANGLE_STRIP : _all_ftss++; break;
      case GL_TRIANGLES      : _all_ftrs++; break;
      default: assert(0);
   }
}

//=============================================================================
// TeselPoly

TeselPoly::TeselPoly(const int4b* pdata, unsigned psize)
{
   TeselTempData ttdata( &_tdata );
   // Start tessellation
   gluTessBeginPolygon(tenderTesel, &ttdata);
   GLdouble pv[3];
   pv[2] = 0;
   word* index_arr = DEBUG_NEW word[psize];
   for (unsigned i = 0; i < psize; i++ )
   {
      pv[0] = pdata[2*i]; pv[1] = pdata[2*i+1];
      index_arr[i] = i;
      gluTessVertex(tenderTesel,pv, &(index_arr[i]));
   }
   gluTessEndPolygon(tenderTesel);
   delete [] index_arr;
   _all_ftrs = ttdata.num_ftrs();
   _all_ftfs = ttdata.num_ftfs();
   _all_ftss = ttdata.num_ftss();
}


GLvoid TeselPoly::teselBegin(GLenum type, GLvoid* ttmp)
{
   TeselTempData* ptmp = static_cast<TeselTempData*>(ttmp);
   ptmp->newChunk(type);
}

GLvoid TeselPoly::teselVertex(GLvoid *pindex, GLvoid* ttmp)
{
   TeselTempData* ptmp = static_cast<TeselTempData*>(ttmp);
   ptmp->newIndex(*(static_cast<word*>(pindex)));
}

GLvoid TeselPoly::teselEnd(GLvoid* ttmp)
{
   TeselTempData* ptmp = static_cast<TeselTempData*>(ttmp);
   ptmp->storeChunk();
}


TeselPoly::~TeselPoly()
{
   for (TeselChain::const_iterator CTC = _tdata.begin(); CTC != _tdata.end(); CTC++)
      delete (*CTC);
}

//=============================================================================
// TenderObj


//=============================================================================

void TenderPoly::TeselData(TeselChain* tdata, unsigned offset)
{
   assert(tdata);
   for (TeselChain::const_iterator CTC = tdata->begin(); CTC != tdata->end(); CTC++)
   {
      TeselChunk* achunk = DEBUG_NEW TeselChunk(*CTC, offset);
      _tdata.push_back(achunk);
   }
}

TenderPoly::~TenderPoly()
{
   for (TeselChain::const_iterator CTC = _tdata.begin(); CTC != _tdata.end(); CTC++)
      delete (*CTC);
}

//=============================================================================
TenderWire::TenderWire(int4b* pdata, unsigned psize, const word width,
                       bool center_line_only) : TenderPoly(NULL, 0), 
                       _ldata(pdata), _lsize(psize), _center_line_only(center_line_only)
{
   if (!_center_line_only)
      precalc(width);
}

void TenderWire::precalc(word width)
{
   _csize = 2 * _lsize;
   _cdata = DEBUG_NEW int[2 * _csize];
   DBbox* ln1 = endPnts(width, 0,1, true);
   word index = 0;
   word rindex = 2 * _csize - 1;
   assert (ln1);
   _cdata[ index++] = ln1->p1().x();
   _cdata[ index++] = ln1->p1().y();
   _cdata[rindex--] = ln1->p2().y();
   _cdata[rindex--] = ln1->p2().x();
   delete ln1;
   for (unsigned i = 1; i < _lsize - 1; i++)
   {
      ln1 = mdlPnts(width, i-1,i,i+1);
      assert(ln1);
      _cdata[ index++] = ln1->p1().x();
      _cdata[ index++] = ln1->p1().y();
      _cdata[rindex--] = ln1->p2().y();
      _cdata[rindex--] = ln1->p2().x();
      delete ln1;
   }
   ln1 = endPnts(width, _lsize -2, _lsize - 1,false);
   assert(ln1);
   _cdata[ index++] = ln1->p1().x();
   _cdata[ index++] = ln1->p1().y();
   _cdata[rindex--] = ln1->p2().y();
   _cdata[rindex--] = ln1->p2().x();
   delete ln1;
   assert(index == _csize);
   assert((rindex + 1) == _csize);
}

DBbox* TenderWire::endPnts(const word width, word i1, word i2, bool first)
{
   double     w = width/2;
   i1 *= 2; i2 *= 2;
   double denom = first ? (_ldata[i2  ] - _ldata[i1  ]) : (_ldata[i1  ] - _ldata[i2  ]);
   double   nom = first ? (_ldata[i2+1] - _ldata[i1+1]) : (_ldata[i1+1] - _ldata[i2+1]);
   double xcorr, ycorr; // the corrections
   assert((0 != nom) || (0 != denom));
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
   return DEBUG_NEW DBbox((int4b) rint(_ldata[it  ] - xcorr),
                          (int4b) rint(_ldata[it+1] + ycorr),
                          (int4b) rint(_ldata[it  ] + xcorr),
                          (int4b) rint(_ldata[it+1] - ycorr) );
}

DBbox* TenderWire::mdlPnts(const word width, word i1, word i2, word i3)
{
   double    w = width/2;
   i1 *= 2; i2 *= 2; i3 *= 2;
   double  x32 = _ldata[i3  ] - _ldata[i2  ];
   double  x21 = _ldata[i2  ] - _ldata[i1  ];
   double  y32 = _ldata[i3+1] - _ldata[i2+1];
   double  y21 = _ldata[i2+1] - _ldata[i1+1];
   double   L1 = sqrt(x21*x21 + y21*y21); //the length of segment 1
   double   L2 = sqrt(x32*x32 + y32*y32); //the length of segment 2
   double denom = x32 * y21 - x21 * y32;
   assert (denom);
   assert (L2);
   // the corrections
   double xcorr = w * ((x32 * L1 - x21 * L2) / denom);
   double ycorr = w * ((y21 * L2 - y32 * L1) / denom);
   return DEBUG_NEW DBbox((int4b) rint(_ldata[i2  ] - xcorr),
                          (int4b) rint(_ldata[i2+1] + ycorr),
                          (int4b) rint(_ldata[i2  ] + xcorr),
                          (int4b) rint(_ldata[i2+1] - ycorr) );
}

/** For wire tessellation we can use the common polygon tessellation procedure.
    This could be a huge overhead though. The thing is that we've
    already been trough the precalc procedure and we know that wire object is
    very specific non-convex polygon. Using this knowledge the tessallation
    is getting really trivial. All we have to do is to list the contour points
    indexes in pairs - one from the front, and the other from the back of the
    array. Then this can be drawn as GL_QUAD_STRIP
*/
void TenderWire::Tessel(unsigned offset)
{
   TeselChunk* tc = DEBUG_NEW TeselChunk(_cdata, _csize, offset);
   _tdata.push_back(tc);
}

TenderWire::~TenderWire()
{
   if (_cdata) delete [] _cdata;
}

//=============================================================================
// class TenderLine
TenderLine::TenderLine(TenderObj* obj, const SGBitSet* psel)
{
   _partial = (NULL != psel);
   if (_partial)
   { // shape is partially selected
      // get the number of selected segments first
      _lsize = 0;
      word allpoints = obj->csize();
      for (unsigned i = 0; i < allpoints; i++)
         if (psel->check(i) && psel->check((i+1)%allpoints)) _lsize +=2;
      // now copy the segment points
      _ldata = DEBUG_NEW int4b [2*_lsize];
      word curpoint = 0;
      for (unsigned i = 0; i < allpoints; i++)
         if (psel->check(i) && psel->check((i+1)%allpoints))
      {
         _ldata[2*curpoint  ] = obj->cdata()[2*i  ];
         _ldata[2*curpoint+1] = obj->cdata()[2*i+1];
         curpoint++;
         _ldata[2*curpoint  ] = obj->cdata()[2*((i+1)%allpoints)  ];
         _ldata[2*curpoint+1] = obj->cdata()[2*((i+1)%allpoints)+1];
         curpoint++;
      }
      assert(curpoint == _lsize);
   }
   else
   {
      _lsize = obj->csize();
      _ldata = obj->cdata();
   }
}

TenderLine::TenderLine(TenderPoly* obj, const SGBitSet* psel)
{
   _partial = (NULL != psel);
   if (_partial)
   { // shape is partially selected
      // get the number of selected segments first
      _lsize = 0;
      word allpoints = obj->csize();
      for (unsigned i = 0; i < allpoints; i++)
         if (psel->check(i) && psel->check((i+1)%allpoints)) _lsize +=2;
      // now copy the segment points
      _ldata = DEBUG_NEW int4b [2*_lsize];
      word curpoint = 0;
      for (unsigned i = 0; i < allpoints; i++)
         if (psel->check(i) && psel->check((i+1)%allpoints))
         {
            _ldata[2*curpoint  ] = obj->cdata()[2*i  ];
            _ldata[2*curpoint+1] = obj->cdata()[2*i+1];
            curpoint++;
            _ldata[2*curpoint  ] = obj->cdata()[2*((i+1)%allpoints)  ];
            _ldata[2*curpoint+1] = obj->cdata()[2*((i+1)%allpoints)+1];
            curpoint++;
         }
      assert(curpoint == _lsize);
   }
   else
   {
      _lsize = obj->csize();
      _ldata = obj->cdata();
   }
}

TenderLine::TenderLine(TenderWire* obj, const SGBitSet* psel)
{
   _partial = (NULL != psel);
   if (_partial)
   { // shape is partially selected
      // get the number of selected segments first
      _lsize = 0;
      word allpoints = obj->lsize();
      for (unsigned i = 0; i < allpoints - 1; i++)
         if (psel->check(i) && psel->check(i+1)) _lsize +=2;
      if (psel->check(0)            ) _lsize +=2;
      if (psel->check(allpoints-1)  ) _lsize +=2;
      // now copy the segment points
      _ldata = DEBUG_NEW int4b [2*_lsize];
      word curpoint = 0;
      for (unsigned i = 0; i < allpoints - 1; i++)
         if (psel->check(i) && psel->check((i+1)%allpoints))
         {
            _ldata[2*curpoint  ] = obj->ldata()[2*i  ];
            _ldata[2*curpoint+1] = obj->ldata()[2*i+1];
            curpoint++;
            _ldata[2*curpoint  ] = obj->ldata()[2*(i+1)  ];
            _ldata[2*curpoint+1] = obj->ldata()[2*(i+1)+1];
            curpoint++;
         }
      if (psel->check(0)            )
      {
         _ldata[2*curpoint  ] = obj->cdata()[0];
         _ldata[2*curpoint+1] = obj->cdata()[1];
         curpoint++;
         _ldata[2*curpoint  ] = obj->cdata()[4*allpoints - 2];
         _ldata[2*curpoint+1] = obj->cdata()[4*allpoints - 1];
         curpoint++;
      }
      if (psel->check(allpoints-1)  )
      {
         _ldata[2*curpoint  ] = obj->cdata()[2*allpoints - 2];
         _ldata[2*curpoint+1] = obj->cdata()[2*allpoints - 1];
         curpoint++;
         _ldata[2*curpoint  ] = obj->cdata()[2*allpoints    ];
         _ldata[2*curpoint+1] = obj->cdata()[2*allpoints + 1];
         curpoint++;
      }
      assert(curpoint == _lsize);
   }
   else
   {
      _lsize = obj->lsize();
      _ldata = obj->ldata();
   }
}

TenderLine::~TenderLine()
{
   if (_partial)
      delete []_ldata;
}

//=============================================================================
// class TenderRB
TenderRB::TenderRB(const CTM& tmatrix, const DBbox& obox) : _tmatrix (tmatrix),
                   _obox(obox)
{}

void TenderRB::draw()
{
   glPushMatrix();
   real openGLmatrix[16];
   _tmatrix.oglForm(openGLmatrix);
   glMultMatrixd(openGLmatrix);
   //
   glRecti(_obox.p1().x(), _obox.p1().y(), _obox.p2().x(), _obox.p2().y());
   //
   glPopMatrix();
}

//=============================================================================
// class TenderTVB
TenderTVB::TenderTVB() :
   _num_ln_points(0u), _num_ll_points(0u),_num_ls_points(0u),
   _num_ln(0), _num_ll(0), _num_ls(0)
{}

void TenderTVB::add(TenderObj* tobj, const SGBitSet* psel)
{
   TenderLine* cline = DEBUG_NEW TenderLine(tobj, psel);
   if (NULL == psel)
   {
      _ll_data.push_back(cline);
      _num_ll_points += cline->lsize();
      _num_ll++;
   }
   else
   {
      _ls_data.push_back(cline);
      _num_ls_points += cline->lsize();
      _num_ls++;
   }
}

void TenderTVB::add(TenderPoly* tobj, const SGBitSet* psel)
{
   TenderLine* cline = DEBUG_NEW TenderLine(tobj, psel);
   if (NULL == psel)
   {
      _ll_data.push_back(cline);
      _num_ll_points += cline->lsize();
      _num_ll++;
   }
   else
   {
      _ls_data.push_back(cline);
      _num_ls_points += cline->lsize();
      _num_ls++;
   }
}

void TenderTVB::add(TenderWire* tobj, const SGBitSet* psel)
{
   TenderLine* cline = DEBUG_NEW TenderLine(tobj, psel);
   if (NULL == psel)
   {
      _ln_data.push_back(cline);
      _num_ln_points += cline->lsize();
      _num_ln++;
   }
   else
   {
      _ls_data.push_back(cline);
      _num_ls_points += cline->lsize();
      _num_ls++;
   }
}

void TenderTVB::draw_lloops()
{
   if  (0 == _num_ll) return;
   unsigned long arr_size = 2 * _num_ll_points;
   int* point_array = DEBUG_NEW int[arr_size];
   GLsizei* size_array = DEBUG_NEW int[_num_ll];
   GLsizei* first_array = DEBUG_NEW int[_num_ll];
   unsigned long pntindx = 0;
   unsigned      szindx  = 0;

   for (SliceLines::const_iterator CSH = _ll_data.begin(); CSH != _ll_data.end(); CSH++)
   {
      unsigned clsize = (*CSH)->lsize();
      assert(clsize);
      first_array[szindx] = pntindx/2;
      size_array[szindx++] = clsize;
      memcpy(&(point_array[pntindx]), (*CSH)->ldata(), 2 * sizeof(int4b) * clsize);
      pntindx += 2 * clsize;
   }
   assert(pntindx == arr_size);
   assert(szindx == _num_ll);
   glVertexPointer(2, GL_INT, 0, point_array);
   glMultiDrawArrays(GL_LINE_LOOP, first_array, size_array, szindx);

   delete [] point_array;
   delete [] size_array;
   delete [] first_array;
}

void TenderTVB::draw_lines()
{
   if  (0 == _num_ln) return;
   unsigned long arr_size = 2 * _num_ln_points;
   int* point_array = DEBUG_NEW int[arr_size];
   GLsizei* size_array = DEBUG_NEW int[_num_ln];
   GLsizei* first_array = DEBUG_NEW int[_num_ln];
   unsigned long pntindx = 0;
   unsigned      szindx  = 0;

   for (SliceLines::const_iterator CSH = _ln_data.begin(); CSH != _ln_data.end(); CSH++)
   {
      unsigned clsize = (*CSH)->lsize();
      assert(clsize);
      first_array[szindx] = pntindx/2;
      size_array[szindx++] = clsize;
      memcpy(&(point_array[pntindx]), (*CSH)->ldata(), 2 * sizeof(int4b) * clsize);
      pntindx += 2 * clsize;
   }
   assert(pntindx == arr_size);
   assert(szindx == _num_ln);
   glVertexPointer(2, GL_INT, 0, point_array);
   glMultiDrawArrays(GL_LINE_STRIP, first_array, size_array, szindx);

   delete [] point_array;
   delete [] size_array;
   delete [] first_array;
}

void TenderTVB::draw_lsegments()
{
   if  (0 == _num_ls) return;
   unsigned long arr_size = 2 * _num_ls_points;
   int* point_array = DEBUG_NEW int[arr_size];
   GLsizei* size_array = DEBUG_NEW int[_num_ls];
   GLsizei* first_array = DEBUG_NEW int[_num_ls];
   unsigned long pntindx = 0;
   unsigned      szindx  = 0;

   for (SliceLines::const_iterator CSH = _ls_data.begin(); CSH != _ls_data.end(); CSH++)
   {
      unsigned clsize = (*CSH)->lsize();
      assert(clsize);
      first_array[szindx] = pntindx/2;
      size_array[szindx++] = clsize;
      memcpy(&(point_array[pntindx]), (*CSH)->ldata(), 2 * sizeof(int4b) * clsize);
      pntindx += 2 * clsize;
   }
   assert(pntindx == arr_size);
   assert(szindx == _num_ls);
   glVertexPointer(2, GL_INT, 0, point_array);
   glMultiDrawArrays(GL_LINES, first_array, size_array, szindx);

   delete [] point_array;
   delete [] size_array;
   delete [] first_array;
}

//=============================================================================
// class TenderTV
TenderTV::TenderTV(CTM& translation, bool filled, unsigned array_offset) :
    _tmatrix(translation),
    _num_cont_points (0u), _num_line_points(0u), _num_polygon_points(0u),
    _all_conts(0)  , _all_lines(0)  , _all_fqus(0)                ,
    _all_fqss(0)   , _all_ftrs(0)   , _all_ftfs(0)                ,
    _all_ftss(0)   , _filled(filled), _array_offset(array_offset) ,
    _sza_cont(NULL), _sza_line(NULL), _sza_fqus(NULL)             ,
    _fst_cont(NULL), _fst_line(NULL), _fst_fqus(NULL)
{}

TenderObj* TenderTV::box (int4b* pdata)
{
   TenderObj* cobj = DEBUG_NEW TenderObj(pdata, 4);
   if (_filled)
   {
      _fqus_data.push_back(cobj);
      _all_fqus++;
   }
   else
   {
      _cont_data.push_back(cobj);
      _num_cont_points += 4;
      _all_conts++;
   }
   return cobj;
}

TenderPoly* TenderTV::poly (int4b* pdata, unsigned psize, TeselPoly* tchain)
{
   TenderPoly* cobj = DEBUG_NEW TenderPoly(pdata, psize);
   _cont_data.push_back(cobj);
   _num_cont_points += cobj->csize();
   _all_conts++;
   if (_filled)
   {
      cobj->TeselData(tchain->tdata(), _num_polygon_points);
      _fpolygon_data.push_back(cobj);
      _num_polygon_points += cobj->csize();
      _all_ftrs += tchain->num_ftrs();
      _all_ftfs += tchain->num_ftfs();
      _all_ftss += tchain->num_ftss();
   }
   return cobj;
}

TenderWire* TenderTV::wire (int4b* pdata, unsigned psize, word width, bool center_line_only)
{
   TenderWire* cobj = DEBUG_NEW TenderWire(pdata, psize, width, center_line_only);
   _line_data.push_back(cobj);
   _num_line_points += cobj->lsize();
   _all_lines++;
   if (!center_line_only)
   {
      _cont_data.push_back(cobj);
      _all_conts += 1;
      _num_cont_points += cobj->csize();
       if (_filled)
       {
         cobj->Tessel(_num_polygon_points);
         _fpolygon_data.push_back(cobj);
         _num_polygon_points += cobj->csize();
         _all_fqss += 1;
       }
   }
   return cobj;
}

void TenderTV::collect(int* point_array)
{
   unsigned long line_arr_size = 2 * _num_line_points;
   unsigned long fqus_arr_size = 8 * _all_fqus; //2 * 4
   unsigned long cont_arr_size = 2 * _num_cont_points;
   // initialise the indexing
   unsigned long pntindx = 0;
   if  (_all_lines > 0)
   {
      unsigned  szindx  = 0;
      _fst_line = DEBUG_NEW int[_all_lines];
      _sza_line = DEBUG_NEW int[_all_lines];
      // copy all the data in the VBO and in the same time update the
      // contour size array and the array containg first indexes
      for (SliceWires::const_iterator CSH = _line_data.begin(); CSH != _line_data.end(); CSH++)
      { // shapes in the current translation (layer within the cell)
         unsigned clsize = (*CSH)->lsize();
         assert(clsize);
         _fst_line[szindx] = pntindx/2;
         _sza_line[szindx++] = clsize;
         memcpy(&(point_array[_array_offset + pntindx]), (*CSH)->ldata(), 2 * sizeof(int4b) * clsize);
         pntindx += 2 * clsize;
      }
      assert(pntindx == line_arr_size);
      assert(szindx  == _all_lines);
   }

   if  (_all_fqus > 0)
   {
      unsigned  szindx  = 0;
      _fst_fqus = DEBUG_NEW int[_all_fqus];
      _sza_fqus = DEBUG_NEW int[_all_fqus];
      for (SliceObjects::const_iterator CSH = _fqus_data.begin(); CSH != _fqus_data.end(); CSH++)
      { // shapes in the current translation (layer within the cell)
         unsigned clsize = (*CSH)->csize();
         assert(clsize);
         _fst_fqus[szindx] = pntindx/2;
         _sza_fqus[szindx++] = clsize;
         memcpy(&(point_array[_array_offset + pntindx]), (*CSH)->cdata(), 2 * sizeof(int4b) * clsize);
         pntindx += 2 * clsize;
      }
      assert(pntindx == line_arr_size + fqus_arr_size);
      assert(szindx  == _all_fqus);
   }

   if  (_all_conts > 0)
   {
      unsigned  szindx  = 0;
      _fst_cont = DEBUG_NEW int[_all_conts];
      _sza_cont = DEBUG_NEW int[_all_conts];
      for (SliceObjects::const_iterator CSH = _cont_data.begin(); CSH != _cont_data.end(); CSH++)
      { // shapes in the current translation (layer within the cell)
         unsigned clsize = (*CSH)->csize();
         assert(clsize);
         _fst_cont[szindx] = pntindx/2;
         _sza_cont[szindx++] = clsize;
         memcpy(&(point_array[_array_offset + pntindx]), (*CSH)->cdata(), 2 * sizeof(int4b) * clsize);
         pntindx += 2 * clsize;
      }
      assert(pntindx == line_arr_size + fqus_arr_size + cont_arr_size);
      assert(szindx  == _all_conts );
   }
}

void TenderTV::draw(GLuint bufnum)
{
   glBindBuffer(GL_ARRAY_BUFFER, bufnum);
   glVertexPointer(2, GL_INT, 0, (GLvoid*)(sizeof(int4b) * _array_offset));
   if  (_all_lines > 0)
   {
      assert(_fst_line);
      assert(_sza_line);
      glMultiDrawArrays(GL_LINE_STRIP, _fst_line, _sza_line, _all_lines);
   }
   if  (_all_fqus > 0)
   {
      assert(_fst_fqus);
      assert(_sza_fqus);
      glMultiDrawArrays(GL_LINE_LOOP, _fst_fqus, _sza_fqus, _all_fqus);
      glMultiDrawArrays(GL_QUADS, _fst_fqus, _sza_fqus, _all_fqus);
   }
   if (_all_conts > 0)
   {
      assert(_fst_cont);
      assert(_sza_cont);
      glMultiDrawArrays(GL_LINE_LOOP, _fst_cont, _sza_cont, _all_conts);
   }
}

void TenderTV::collectNdraw_contours()
{
   if  (0 == _all_conts) return;
   unsigned long arr_size = 2 * _num_cont_points;
#ifdef USE_VBOS
   // Organise the VBO ...
   GLuint ogl_buffer;
   glGenBuffers(1, &ogl_buffer);
   glBindBuffer(GL_ARRAY_BUFFER, ogl_buffer);
   glBufferData(GL_ARRAY_BUFFER, arr_size * sizeof(int4b), NULL, GL_DYNAMIC_DRAW);
   int* point_array = (int*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
#else
   int* point_array = DEBUG_NEW int[arr_size];
#endif
   // ... and the additional arrays
   GLsizei* size_array = DEBUG_NEW int[_all_conts];
   GLsizei* first_array = DEBUG_NEW int[_all_conts];
   // initialise the indexing
   unsigned long pntindx = 0;
   unsigned      szindx  = 0;
   // copy all the data in the VBO and in the same time update the
   // contour size array and the array containg first indexes
   for (SliceObjects::const_iterator CSH = _cont_data.begin(); CSH != _cont_data.end(); CSH++)
   { // shapes in the current translation (layer within the cell)
      unsigned clsize = (*CSH)->csize();
      assert(clsize);
      first_array[szindx] = pntindx/2;
      size_array[szindx++] = clsize;
//#ifdef USE_VBOS
//      glBufferSubData(GL_ARRAY_BUFFER, pntindx * sizeof(int4b), 2 * sizeof(int4b) * clsize, (*CSH)->cdata());
//#else
      memcpy(&(point_array[pntindx]), (*CSH)->cdata(), 2 * sizeof(int4b) * clsize);
//#endif
      pntindx += 2 * clsize;
   }
   assert(pntindx == arr_size);
   assert(szindx == _all_conts);
#ifdef USE_VBOS
   // Draw the VBO
   glUnmapBuffer(GL_ARRAY_BUFFER);
   glVertexPointer(2, GL_INT, 0, 0);
#else
   glVertexPointer(2, GL_INT, 0, point_array);
#endif
   glMultiDrawArrays(GL_LINE_LOOP, first_array, size_array, szindx);
   // Release the VBO memory in the GPU
#ifdef USE_VBOS
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glDeleteBuffers(1, &ogl_buffer);
#else
   delete [] point_array;
#endif
   // Clean-up the CPU memory
   delete [] size_array;
   delete [] first_array;
}

void TenderTV::collectNdraw_lines()
{
   if  (0 == _all_lines) return;
   unsigned long arr_size = 2 * _num_line_points;
   int* point_array = DEBUG_NEW int[arr_size];
   GLsizei* size_array = DEBUG_NEW int[_all_lines];
   GLsizei* first_array = DEBUG_NEW int[_all_lines];
   unsigned long pntindx = 0;
   unsigned      szindx  = 0;

   for (SliceWires::const_iterator CSH = _line_data.begin(); CSH != _line_data.end(); CSH++)
   { // shapes in the current translation (layer within the cell)
      unsigned clsize = (*CSH)->lsize();
      assert(clsize);
      first_array[szindx] = pntindx/2;
      size_array[szindx++] = clsize;
      memcpy(&(point_array[pntindx]), (*CSH)->ldata(), 2 * sizeof(int4b) * clsize);
      pntindx += 2 * clsize;
   }
   assert(pntindx == arr_size);
   assert(szindx == _all_lines);
   glVertexPointer(2, GL_INT, 0, point_array);
   glMultiDrawArrays(GL_LINE_STRIP, first_array, size_array, szindx);

   delete [] point_array;
   delete [] size_array;
   delete [] first_array;
}

void TenderTV::collectNdraw_fqus()
{
   if  (0 == _all_fqus) return;
   unsigned long arr_size = 8 * _all_fqus; //2 * 4
   int* point_array = DEBUG_NEW int[arr_size];
   GLsizei* size_array = DEBUG_NEW int[_all_fqus];
   GLsizei* first_array = DEBUG_NEW int[_all_fqus];
   unsigned long pntindx = 0;
   unsigned      szindx  = 0;

   for (SliceObjects::const_iterator CSH = _fqus_data.begin(); CSH != _fqus_data.end(); CSH++)
   { // shapes in the current translation (layer within the cell)
      unsigned clsize = (*CSH)->csize();
      assert(clsize);
      first_array[szindx] = pntindx/2;
      size_array[szindx++] = clsize;
      memcpy(&(point_array[pntindx]), (*CSH)->cdata(), 2 * sizeof(int4b) * clsize);
      pntindx += 2 * clsize;
   }
   assert(pntindx == arr_size);
   assert(szindx == _all_fqus);
   glVertexPointer(2, GL_INT, 0, point_array);
   glMultiDrawArrays(GL_QUADS, first_array, size_array, szindx);

   delete [] point_array;
   delete [] size_array;
   delete [] first_array;
}

void TenderTV::draw_fpolygons()
{
   unsigned long arr_size = 2 * _num_polygon_points;
   if  (0 == arr_size) return;
   unsigned          pntindx       = 0;
   unsigned          sz_ftrs_indx  = 0;
   unsigned          sz_ftfs_indx  = 0;
   unsigned          sz_ftss_indx  = 0;
   unsigned          sz_fqss_indx  = 0;
   GLsizei*          sz_ftrs_array = NULL;
   GLsizei*          sz_ftfs_array = NULL;
   GLsizei*          sz_ftss_array = NULL;
   GLsizei*          sz_fqss_array = NULL;
   const GLvoid**    ix_ftrs_array = NULL;
   const GLvoid**    ix_ftfs_array = NULL;
   const GLvoid**    ix_ftss_array = NULL;
   const GLvoid**    ix_fqss_array = NULL;

   int* point_array = DEBUG_NEW int[arr_size];
   if (_all_ftrs)
   {
      sz_ftrs_array = DEBUG_NEW       GLsizei[_all_ftrs];
      ix_ftrs_array = DEBUG_NEW const GLvoid*[_all_ftrs];
   }
   if (_all_ftfs)
   {
      sz_ftfs_array = DEBUG_NEW       GLsizei[_all_ftfs];
      ix_ftfs_array = DEBUG_NEW const GLvoid*[_all_ftfs];
   }
   if (_all_ftss)
   {
      sz_ftss_array = DEBUG_NEW       GLsizei[_all_ftss];
      ix_ftss_array = DEBUG_NEW const GLvoid*[_all_ftss];
   }
   if (_all_fqss)
   {
      sz_fqss_array = DEBUG_NEW       GLsizei[_all_fqss];
      ix_fqss_array = DEBUG_NEW const GLvoid*[_all_fqss];
   }

   for (SlicePolygons::const_iterator CSH = _fpolygon_data.begin(); CSH != _fpolygon_data.end(); CSH++)
   { // shapes in the current translation (layer within the cell)
      TeselChain* tdata = (*CSH)->tdata();
      for (TeselChain::const_iterator TCH = tdata->begin(); TCH != tdata->end(); TCH++)
      {
         TeselChunk* cchunk = *TCH;
         switch (cchunk->type())
         {
            case GL_TRIANGLES      :
            {
               assert(sz_ftrs_array); assert(ix_ftrs_array);
               sz_ftrs_array[sz_ftrs_indx  ] = cchunk->size();
               ix_ftrs_array[sz_ftrs_indx++] = cchunk->index_seq();
               break;
            }
            case GL_TRIANGLE_FAN   :
            {
               assert(sz_ftfs_array); assert(ix_ftfs_array);
               sz_ftfs_array[sz_ftfs_indx  ] = cchunk->size();
               ix_ftfs_array[sz_ftfs_indx++] = cchunk->index_seq();
               break;
            }
            case GL_TRIANGLE_STRIP :
            {
               assert(sz_ftss_array); assert(ix_ftss_array);
               sz_ftss_array[sz_ftss_indx  ] = cchunk->size();
               ix_ftss_array[sz_ftss_indx++] = cchunk->index_seq();
               break;
            }
            case GL_QUAD_STRIP     :
            {
               assert(sz_fqss_array); assert(ix_fqss_array);
               sz_fqss_array[sz_fqss_indx  ] = cchunk->size();
               ix_fqss_array[sz_fqss_indx++] = cchunk->index_seq();
               break;
            }
            default: assert(0);
         }
      }
      memcpy(&(point_array[pntindx]), (*CSH)->cdata(), 2 * sizeof(int4b) * (*CSH)->csize());
      pntindx += 2 * (*CSH)->csize();
   }
   assert(pntindx == arr_size);
   assert(sz_ftrs_indx == _all_ftrs);
   assert(sz_ftfs_indx == _all_ftfs);
   assert(sz_ftss_indx == _all_ftss);
   assert(sz_fqss_indx == _all_fqss);

   glVertexPointer(2, GL_INT, 0, point_array);
   if (sz_ftrs_indx > 0)
   {
      glMultiDrawElements(GL_TRIANGLES, sz_ftrs_array, GL_UNSIGNED_INT,
                          ix_ftrs_array, sz_ftrs_indx);
      delete[] ix_ftrs_array;
      delete[] sz_ftrs_array;
   }
   if (sz_ftfs_indx > 0)
   {
      glMultiDrawElements(GL_TRIANGLE_FAN, sz_ftfs_array, GL_UNSIGNED_INT,
                          ix_ftfs_array, sz_ftfs_indx);
      delete[] ix_ftfs_array;
      delete[] sz_ftfs_array;
   }
   if (sz_ftss_indx > 0)
   {
      glMultiDrawElements(GL_TRIANGLE_STRIP, sz_ftss_array, GL_UNSIGNED_INT,
                          ix_ftss_array, sz_ftss_indx);
      delete[] ix_ftss_array;
      delete[] sz_ftss_array;
   }
   if (sz_fqss_indx > 0)
   {
      glMultiDrawElements(GL_QUAD_STRIP, sz_fqss_array, GL_UNSIGNED_INT,
                          ix_fqss_array, sz_fqss_indx);
      delete[] ix_fqss_array;
      delete[] sz_fqss_array;
   }
   delete [] point_array;
}

TenderTV::~TenderTV()
{
   for (SliceWires::const_iterator CSO = _line_data.begin(); CSO != _line_data.end(); CSO++)
      if ((*CSO)->center_line_only()) delete (*CSO);
   for (SliceObjects::const_iterator CSO = _cont_data.begin(); CSO != _cont_data.end(); CSO++)
      delete (*CSO);
   if (NULL != _sza_cont)
   {
      delete [] _sza_cont; _sza_cont = NULL;
   }
   if (NULL != _sza_line)
   {
      delete [] _sza_line; _sza_line = NULL;
   }
   if (NULL != _sza_fqus)
   {
      delete [] _sza_fqus; _sza_fqus = NULL;
   }
   if (NULL != _fst_cont)
   {
      delete [] _fst_cont; _fst_cont = NULL;
   }
   if (NULL != _fst_fqus)
   {
      delete [] _fst_fqus; _fst_fqus = NULL;
   }
}

//=============================================================================
void TenderLay::newSlice(CTM& ctrans, bool fill)
{
   unsigned array_offset = 2 * (_num_cont_points + _num_line_points + _num_fqus_points);
   _cslice = DEBUG_NEW TenderTV(ctrans, fill, array_offset);
   _layData.push_back(_cslice);
}

void TenderLay::ppSlice()
{
   if (NULL != _cslice)
   {
      _num_cont_points += _cslice->num_cont_points();
      _num_line_points += _cslice->num_line_points();
      _num_fqus_points += _cslice->num_fqus_points();
   }
}

// void TenderLay::collect(bool fill, GLuint cont_parray, GLuint line_parray, GLuint fqus_parray)
void TenderLay::collect(bool fill, GLuint parray)
{
   int* cpoint_array;
   // don't forget to post-precess the last slices...
   ppSlice();

   glBindBuffer(GL_ARRAY_BUFFER, parray);
   glBufferData(GL_ARRAY_BUFFER,
                2 * (_num_cont_points + _num_line_points + _num_fqus_points ) * sizeof(int4b),
                NULL,
                GL_DYNAMIC_DRAW);
   cpoint_array = (int*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
   for (TenderTVList::const_iterator TLAY = _layData.begin(); TLAY != _layData.end(); TLAY++)
      (*TLAY)->collect(cpoint_array);
   glUnmapBuffer(GL_ARRAY_BUFFER);

}

// void TenderLay::draw(bool fill, GLuint cont_parray, GLuint line_parray, GLuint fqus_parray)
void TenderLay::draw(bool fill, GLuint parray)
{
   for (TenderTVList::const_iterator TLAY = _layData.begin(); TLAY != _layData.end(); TLAY++)
   {
      TenderTV* ctv = (*TLAY);
      glPushMatrix();
      real openGLmatrix[16];
      ctv->tmatrix()->oglForm(openGLmatrix);
      glMultMatrixd(openGLmatrix);
      ctv->draw(parray);
      glPopMatrix();
   }
}

void TenderLay::collectNdraw(bool fill)
{
   for (TenderTVList::const_iterator TLAY = _layData.begin(); TLAY != _layData.end(); TLAY++)
   {
      TenderTV* ctv = (*TLAY);
      glPushMatrix();
      real openGLmatrix[16];
      ctv->tmatrix()->oglForm(openGLmatrix);
      glMultMatrixd(openGLmatrix);
      //
      ctv->collectNdraw_contours();
      ctv->collectNdraw_lines();
      if (fill)
      {
         ctv->collectNdraw_fqus();
         ctv->draw_fpolygons();
      }
      //
      glPopMatrix();
   }
}

TenderLay::~TenderLay()
{
   for (TenderTVList::const_iterator TLAY = _layData.begin(); TLAY != _layData.end(); TLAY++)
      delete (*TLAY);
}

//=============================================================================
Tenderer::Tenderer( layprop::DrawProperties* drawprop, real UU ) :
      _drawprop(drawprop), _UU(UU), _clayer(NULL),
      _ogl_buffers(NULL)
{}

void Tenderer::setLayer(word layer)
{
   // Reference layer is processed differently (pushCell), so make sure
   // that we haven't got here with layer 0 by accident
   assert(layer);
   if (NULL != _clayer)
      _clayer->ppSlice();
   if (_data.end() != _data.find(layer))
   {
      _clayer = _data[layer];
   }
   else
   {
      _clayer = DEBUG_NEW TenderLay();
      _data[layer] = _clayer;
   }
   _clayer->newSlice(_ctrans, _drawprop->isFilled(layer));
   // @TODO! current fill on/off should be determined here!
}

void Tenderer::setSdataContainer(word layer)
{
   _sslice = DEBUG_NEW TenderTVB();
   _sdata[layer] = _sslice;
}

void Tenderer::pushCell(const CTM& trans, const DBbox& overlap, bool active, bool selected)
{
   _ctrans = trans * _drawprop->topCTM();
   _oboxes.push_back(DEBUG_NEW TenderRB(_ctrans, overlap));
   if (selected)
      _osboxes.push_back(DEBUG_NEW TenderRB(_ctrans, overlap));
   _drawprop->pushCTM(_ctrans);
   if (active)
      _atrans = trans;
}

void Tenderer::box  (int4b* pdata, const SGBitSet* psel)
{
   assert(_sslice);
   TenderObj* dobj = _clayer->box(pdata);
   _sslice->add(dobj, psel);
}

void Tenderer::poly (int4b* pdata, unsigned psize, TeselPoly* tpoly, const SGBitSet* psel)
{
   assert(_sslice);
   TenderPoly* dpoly = _clayer->poly(pdata, psize, tpoly);
   _sslice->add(dpoly, psel);
}

void Tenderer::wire (int4b* pdata, unsigned psize, word width)
{
   // first check whether to draw only the center line
   DBbox wsquare = DBbox(TP(0,0),TP(width,width));
   bool center_line_only = !wsquare.visible(topCTM() * ScrCTM());
   _clayer->wire(pdata, psize, width, center_line_only);
}

void Tenderer::wire (int4b* pdata, unsigned psize, word width, const SGBitSet* psel)
{
   assert(_sslice);
   TenderWire* dwire = _clayer->wire(pdata, psize, width, false);
   _sslice->add(dwire, psel);
}

void Tenderer::Grid(const real step, const std::string color)
{
   int gridstep = (int)rint(step / _UU);
   if ( abs((int)(_drawprop->ScrCTM().a() * gridstep)) > GRID_LIMIT)
   {
      _drawprop->setGridColor(color);
      // set first grid step to be multiply on the step
      TP bl = TP(_drawprop->clipRegion().p1().x(),_drawprop->clipRegion().p2().y());
      TP tr = TP(_drawprop->clipRegion().p2().x(),_drawprop->clipRegion().p1().y());
      int signX = (bl.x() > 0) ? 1 : -1;
      int X_is = (int)((rint(abs(bl.x()) / gridstep)) * gridstep * signX);
      int signY = (tr.y() > 0) ? 1 : -1;
      int Y_is = (int)((rint(abs(tr.y()) / gridstep)) * gridstep * signY);

      glEnableClientState(GL_VERTEX_ARRAY);
      word arr_size = ( (((tr.x() - X_is + 1) / gridstep) + 1) * (((bl.y() - Y_is + 1) / gridstep) + 1) );
      int* point_array = DEBUG_NEW int[arr_size * 2];
      int index = 0;
      for (int i = X_is; i < tr.x()+1; i += gridstep)
      {
         for (int j = Y_is; j < bl.y()+1; j += gridstep)
         {
            point_array[index++] = i;
            point_array[index++] = j;
         }
      }
      assert(index <= (arr_size*2));
      glVertexPointer(2, GL_INT, 0, point_array);
      glDrawArrays(GL_POINTS, 0, arr_size);
      delete [] point_array;
      glDisableClientState(GL_VERTEX_ARRAY);
   }
}

void Tenderer::collect()
{
   // Organise the VBOs ...
   GLuint num_ogl_buffers = _data.size();
   _ogl_buffers = new GLuint [num_ogl_buffers];
   glGenBuffers(num_ogl_buffers, _ogl_buffers);
   unsigned current_buffer = 0;
   for (DataLay::const_iterator CLAY = _data.begin(); CLAY != _data.end(); CLAY++)
   {
      CLAY->second->collect(_drawprop->isFilled(CLAY->first),
                            _ogl_buffers[current_buffer++]);
   }
}

void Tenderer::draw()
{
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   unsigned current_buffer = 0;
   for (DataLay::const_iterator CLAY = _data.begin(); CLAY != _data.end(); CLAY++)
   {
      word curlayno = CLAY->first;
      _drawprop->setCurrentColor(curlayno);
      CLAY->second->draw(_drawprop->getCurrentFill(),
                         _ogl_buffers[current_buffer++]);
   }
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   GLuint num_ogl_buffers = _data.size();
   glDeleteBuffers(num_ogl_buffers, _ogl_buffers);
}

void Tenderer::collectNdraw()
{
   glEnableClientState(GL_VERTEX_ARRAY);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   for (DataLay::const_iterator CLAY = _data.begin(); CLAY != _data.end(); CLAY++)
   {
      word curlayno = CLAY->first;
      _drawprop->setCurrentColor(curlayno);
      CLAY->second->collectNdraw(_drawprop->getCurrentFill());
   }
   // now the overlapping boxes of the cell references
   _drawprop->setCurrentColor(0);
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//   glDisable(GL_POLYGON_STIPPLE);   //- for solid fill
   for (TenderRBL::const_iterator CBOX = _oboxes.begin(); CBOX != _oboxes.end(); CBOX++)
   {
      (*CBOX)->draw();
   }
   _drawprop->setLineProps(true);
   for (TenderRBL::const_iterator CBOX = _osboxes.begin(); CBOX != _osboxes.end(); CBOX++)
   {
      (*CBOX)->draw();
   }
   _drawprop->setLineProps(false);
   // and finally -  the selected objects
   glPushMatrix();
   real openGLmatrix[16];
   _atrans.oglForm(openGLmatrix);
   glMultMatrixd(openGLmatrix);
   for (DataSel::const_iterator CLAY = _sdata.begin(); CLAY != _sdata.end(); CLAY++)
   {
      _drawprop->setCurrentColor(CLAY->first);
      _drawprop->setLineProps(true);
      TenderTVB* ctv = CLAY->second;
      //
      ctv->draw_lloops();
      ctv->draw_lines();
      ctv->draw_lsegments();
      //
      _drawprop->setLineProps(false);
   }
   glPopMatrix();
   glDisableClientState(GL_VERTEX_ARRAY);
}

Tenderer::~Tenderer()
{
   for (DataLay::const_iterator CLAY = _data.begin(); CLAY != _data.end(); CLAY++)
      delete (CLAY->second);

   for (TenderRBL::const_iterator CRBL = _oboxes.begin(); CRBL != _oboxes.end(); CRBL++)
      delete (*CRBL);
}

//=============================================================================
//
//
HiResTimer::HiResTimer()
{
#ifdef WIN32
   // Get system frequency (number of ticks per second) of timer
   if (!QueryPerformanceFrequency(&_freq) || !QueryPerformanceCounter(&_inittime))
   {
      tell_log(console::MT_INFO,"Problem with timer");
   }
#else
   gettimeofday(&_start_time, NULL);
#endif
}

void HiResTimer::report(char* message)
{
   char time_message[256];
#ifdef WIN32
   LARGE_INTEGER curtime;
   if (!QueryPerformanceCounter(&curtime)) 
      return ;
  // Convert number of ticks to milliseconds
   int millisec = (curtime.QuadPart-_inittime.QuadPart) / (_freq.QuadPart / 1000);
   int sec = millisec / 1000;
   millisec = millisec - sec * 1000;
   sprintf (time_message, "%s:   %i sec. %06i msec.",message, sec, millisec);

#else

   gettimeofday(&_end_time, NULL);
   timeval result;
   result.tv_sec = _end_time.tv_sec - _start_time.tv_sec;
   result.tv_usec = _end_time.tv_usec - _start_time.tv_usec;
   if (result.tv_usec < 0)
   {
      result.tv_sec -= 1;
      result.tv_usec += 1000000;
   }
   sprintf (time_message, "%s:   %i sec. %06i msec.",message, result.tv_sec, result.tv_usec);
#endif
   tell_log(console::MT_INFO,time_message);
}
