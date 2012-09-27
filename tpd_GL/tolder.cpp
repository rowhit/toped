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
//        Created: Sun Sep 16 BST 2012
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: OpenGL Basic renderer
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include "tpdph.h"
#include <sstream>
#include "tolder.h"
#include "viewprop.h"
#include "trend.h"


trend::TolderTV::TolderTV(TrendRef* const refCell, bool filled, bool reusable,
                   unsigned parray_offset, unsigned iarray_offset) :
   TrendTV              (refCell, filled, reusable, parray_offset, iarray_offset)
{
}


void trend::TolderTV::collect(TNDR_GLDATAT* point_array, unsigned int* index_array)
{
   assert(false); // should not be called in this implementation
}

void trend::TolderTV::draw(layprop::DrawProperties* drawprop)
{
   // First - deal with openGL translation matrix
   glPushMatrix();
   glMultMatrixd(_refCell->translation());
   drawprop->adjustAlpha(_refCell->alphaDepth() - 1);
   // ... and here we go ...
   if  (_alobjvx[line] > 0)
   {// Draw the wire centre lines
      for (SliceWires::const_iterator CSH = _line_data.begin(); CSH != _line_data.end(); CSH++)
      {
         (*CSH)->drctDrawCLine();
      }
   }
   if  (_alobjvx[cnvx] > 0)
   {// Draw convex polygons
      for (SliceObjects::const_iterator CSH = _cnvx_data.begin(); CSH != _cnvx_data.end(); CSH++)
      {
         (*CSH)->drctDrawContour();
         (*CSH)->drctDrawFill();
      }
   }
   if  (_alobjvx[ncvx] > 0)
   {// Draw non-convex polygons
      for (SlicePolygons::const_iterator CSH = _ncvx_data.begin(); CSH != _ncvx_data.end(); CSH++)
      {
         (*CSH)->drctDrawContour();
         (*CSH)->drctDrawFill();
      }
   }
   if (_alobjvx[cont] > 0)
   {// Draw the remaining non-filled shapes of any kind
      // TODO
//      glMultiDrawArrays(GL_LINE_LOOP, _firstvx[cont], _sizesvx[cont], _alobjvx[cont]);
   }
   glPopMatrix();
}

void trend::TolderTV::drawTexts(layprop::DrawProperties* drawprop)
{
   glPushMatrix();
   glMultMatrixd(_refCell->translation());
   drawprop->adjustAlpha(_refCell->alphaDepth() - 1);

   for (TrendStrings::const_iterator TSTR = _text_data.begin(); TSTR != _text_data.end(); TSTR++)
      (*TSTR)->draw(_filled);

   glPopMatrix();
}

trend::TolderTV::~TolderTV()
{
}

//=============================================================================
//
// class TolderReTV
//
void trend::TolderReTV::draw(layprop::DrawProperties* drawprop)
{
   TrendRef* sref_cell = _chunk->swapRefCells(_refCell);
   _chunk->draw(drawprop);
   _chunk->swapRefCells(sref_cell);
}

void trend::TolderReTV::drawTexts(layprop::DrawProperties*)
{
   //TODO
}

//=============================================================================
//
// class TolderLay
//
trend::TolderLay::TolderLay():
   TrendLay              (             ),
   _cpoint_array         (        NULL ),
   _cindex_array         (        NULL ),
   _stv_array_offset     (          0u ),
   _slctd_array_offset   (          0u )

{
//   for (int i = lstr; i <= lnes; i++)
//   {
//      _sizslix[i] = NULL;
//      _fstslix[i] = NULL;
//   }
}

void trend::TolderLay::newSlice(TrendRef* const ctrans, bool fill, bool reusable, unsigned slctd_array_offset)
{
   assert( 0 == total_slctdx());
   _slctd_array_offset = slctd_array_offset;
   _stv_array_offset = 2 * _num_total_points;
   newSlice(ctrans, fill, reusable);
}

void trend::TolderLay::newSlice(TrendRef* const ctrans, bool fill, bool reusable)
{
   _cslice = DEBUG_NEW TolderTV(ctrans, fill, reusable, 2 * _num_total_points, _num_total_indexs);
}

bool trend::TolderLay::chunkExists(TrendRef* const ctrans, bool filled)
{
   ReusableTTVMap::iterator achunk;
   if (filled)
   {
      if (_reusableFData.end() == ( achunk =_reusableFData.find(ctrans->name()) ) )
         return false;
   }
   else
   {
      if (_reusableCData.end() == ( achunk =_reusableCData.find(ctrans->name()) ) )
         return false;
   }
   _reLayData.push_back(DEBUG_NEW TolderReTV(achunk->second, ctrans));
   return true;
}

void trend::TolderLay::collect(bool fill, GLuint, GLuint)
{
   assert(false);
//   _cpoint_array = DEBUG_NEW TNDR_GLDATAT[2 * _num_total_points];
//   if (0 != _num_total_indexs)
//      _cindex_array = DEBUG_NEW unsigned int[_num_total_indexs];
//   for (TrendTVList::const_iterator TLAY = _layData.begin(); TLAY != _layData.end(); TLAY++)
//      (*TLAY)->collect(_cpoint_array, _cindex_array);
}

void trend::TolderLay::collectSelected(unsigned int* slctd_array)
{
   assert(false);
//   unsigned      slct_arr_size = _asindxs[lstr] + _asindxs[llps] + _asindxs[lnes];
//   if (0 == slct_arr_size) return;
//
//   // initialise the indexing arrays of selected objects
//   if (0 < _asobjix[lstr])
//   {
//      _sizslix[lstr] = DEBUG_NEW GLsizei[_asobjix[lstr]];
//      _fstslix[lstr] = DEBUG_NEW GLuint[_asobjix[lstr]];
//   }
//   if (0 < _asobjix[llps])
//   {
//      _sizslix[llps] = DEBUG_NEW GLsizei[_asobjix[llps]];
//      _fstslix[llps] = DEBUG_NEW GLuint[_asobjix[llps]];
//   }
//   if (0 < _asobjix[lnes])
//   {
//      _sizslix[lnes] = DEBUG_NEW GLsizei[_asobjix[lnes]];
//      _fstslix[lnes] = DEBUG_NEW GLuint[_asobjix[lnes]];
//   }
//   unsigned size_sindex[3];
//   unsigned index_soffset[3];
//   size_sindex[lstr] = size_sindex[llps] = size_sindex[lnes] = 0u;
//   index_soffset[lstr] = _slctd_array_offset;
//   index_soffset[llps] = index_soffset[lstr] + _asindxs[lstr];
//   index_soffset[lnes] = index_soffset[llps] + _asindxs[llps];
//
//
//   for (SliceSelected::const_iterator SSL = _slct_data.begin(); SSL != _slct_data.end(); SSL++)
//   {
//      TrendSelected* cchunk = *SSL;
//      switch (cchunk->type())
//      {
//         case lstr : // LINES
//         {
//            assert(_sizslix[lstr]);
//            _fstslix[lstr][size_sindex[lstr]  ] = /*sizeof(unsigned) * */index_soffset[lstr];
//            _sizslix[lstr][size_sindex[lstr]++] = cchunk->sDataCopy(slctd_array, index_soffset[lstr]);
//            break;
//         }
//         case llps      : // LINE_LOOP
//         {
//            assert(_sizslix[llps]);
//            _fstslix[llps][size_sindex[llps]  ] = /*sizeof(unsigned) * */index_soffset[llps];
//            _sizslix[llps][size_sindex[llps]++] = cchunk->sDataCopy(slctd_array, index_soffset[llps]);
//            break;
//         }
//         case lnes   : // LINE_STRIP
//         {
//            assert(_sizslix[lnes]);
//            _fstslix[lnes][size_sindex[lnes]  ] = /*sizeof(unsigned) * */index_soffset[lnes];
//            _sizslix[lnes][size_sindex[lnes]++] = cchunk->sDataCopy(slctd_array, index_soffset[lnes]);
//            break;
//         }
//         default: assert(false);break;
//      }
//   }
}

void trend::TolderLay::draw(layprop::DrawProperties* drawprop)
{
   for (TrendTVList::const_iterator TLAY = _layData.begin(); TLAY != _layData.end(); TLAY++)
   {
      (*TLAY)->draw(drawprop);
   }
   for (TrendReTVList::const_iterator TLAY = _reLayData.begin(); TLAY != _reLayData.end(); TLAY++)
   {
      (*TLAY)->draw(drawprop);
   }
}

void trend::TolderLay::drawSelected()
{
//   glBindBuffer(GL_ARRAY_BUFFER, _pbuffer);
   // Check the state of the buffer
//   GLint bufferSize;
//   glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
//   assert(bufferSize == (GLint)(2 * _num_total_points * sizeof(TNDR_GLDATAT)));

//   glEnableClientState(GL_VERTEX_ARRAY);
//   glEnableClientState(GL_INDEX_ARRAY);
//   glVertexPointer(2, TNDR_GLENUMT, 0, /*(GLvoid*)(sizeof(TNDR_GLDATAT) * */_stv_array_offset));

   if (_asobjix[lstr] > 0)
   {
      //TODO
//      for (unsigned i= 0; i < _asobjix[lstr]; i++)
//         glDrawElements(GL_LINE_STRIP, _sizslix[lstr][i], GL_UNSIGNED_INT, VBO_BUFFER_OFFSET(_fstslix[lstr][i]));
   }
   if (_asobjix[llps] > 0)
   {
      //TODO
//      for (unsigned i= 0; i < _asobjix[llps]; i++)
//         glDrawElements(GL_LINE_LOOP, _sizslix[llps][i], GL_UNSIGNED_INT, VBO_BUFFER_OFFSET(_fstslix[llps][i]));
   }
   if (_asobjix[lnes] > 0)
   {
      //TODO
//      for (unsigned i= 0; i < _asobjix[lnes]; i++)
//         glDrawElements(GL_LINES, _sizslix[lnes][i], GL_UNSIGNED_INT, VBO_BUFFER_OFFSET(_fstslix[lnes][i]));
   }
//   glDisableClientState(GL_INDEX_ARRAY);
//   glDisableClientState(GL_VERTEX_ARRAY);
//   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void trend::TolderLay::drawTexts(layprop::DrawProperties* drawprop)
{
   for (TrendTVList::const_iterator TLAY = _layData.begin(); TLAY != _layData.end(); TLAY++)
   {
      (*TLAY)->drawTexts(drawprop);
   }
   for (TrendReTVList::const_iterator TLAY = _reLayData.begin(); TLAY != _reLayData.end(); TLAY++)
   {
      (*TLAY)->drawTexts(drawprop);
   }
}

trend::TolderLay::~TolderLay()
{
   delete [] _cpoint_array;
   if (NULL != _cindex_array)  delete [] _cindex_array;

//   if (NULL != _sizslix[lstr]) delete [] _sizslix[lstr];
//   if (NULL != _sizslix[llps]) delete [] _sizslix[llps];
//   if (NULL != _sizslix[lnes]) delete [] _sizslix[lnes];
//
//   if (NULL != _fstslix[lstr]) delete [] _fstslix[lstr];
//   if (NULL != _fstslix[llps]) delete [] _fstslix[llps];
//   if (NULL != _fstslix[lnes]) delete [] _fstslix[lnes];
}

//=============================================================================
//
// class TolderRefLay
//
trend::TolderRefLay::TolderRefLay() :
   TrendRefLay    (      ),
   _cpoint_array  ( NULL ),
   _sizesvx       ( NULL ),
   _firstvx       ( NULL ),
   _sizslix       ( NULL ),
   _fstslix       ( NULL )
{
}

void trend::TolderRefLay::collect(GLuint)
{
   assert(false);
////   _pbuffer = pbuf;
////   glBindBuffer(GL_ARRAY_BUFFER, _pbuffer);
////   glBufferData(GL_ARRAY_BUFFER              ,
////                2 * total_points() * sizeof(TNDR_GLDATAT) ,
////                NULL                         ,
////                GL_DYNAMIC_DRAW               );
////   cpoint_array = (TNDR_GLDATAT*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
//   _cpoint_array = DEBUG_NEW TNDR_GLDATAT[2 * total_points()];
//
//
//   // initialise the indexing
//   unsigned pntindx = 0;
//   unsigned  szindx  = 0;
//   if (0 < (_alvrtxs + _asindxs))
//   {
//      _firstvx = DEBUG_NEW GLsizei[_alobjvx + _asobjix];
//      _sizesvx = DEBUG_NEW GLsizei[_alobjvx + _asobjix];
//      if (0 < _asobjix)
//      {
//         _fstslix = DEBUG_NEW GLsizei[_asobjix];
//         _sizslix = DEBUG_NEW GLsizei[_asobjix];
//      }
//   }
//   // collect the cell overlapping boxes
//   for (RefBoxList::const_iterator CSH = _cellRefBoxes.begin(); CSH != _cellRefBoxes.end(); CSH++)
//   {
//      if (1 < (*CSH)->alphaDepth())
//      {
//         _firstvx[szindx  ] = pntindx/2;
//         _sizesvx[szindx++] = (*CSH)->cDataCopy(_cpoint_array, pntindx);
//      }
//   }
//   for (RefBoxList::const_iterator CSH = _cellSRefBoxes.begin(); CSH != _cellSRefBoxes.end(); CSH++)
//   {
//      _fstslix[szindx-_alobjvx] = _firstvx[szindx] = pntindx/2;
//      _sizslix[szindx-_alobjvx] = _sizesvx[szindx] = (*CSH)->cDataCopy(_cpoint_array, pntindx);
//      szindx++;
//   }
//   assert(pntindx == 2 * (_alvrtxs + _asindxs));
//   assert(szindx  ==     (_alobjvx + _asobjix));
//
}

void trend::TolderRefLay::draw(layprop::DrawProperties* drawprop)
{
   //TODO
}

trend::TolderRefLay::~TolderRefLay()
{
   if (NULL != _cpoint_array) delete [] _cpoint_array;
   if (NULL != _sizesvx) delete [] (_sizesvx);
   if (NULL != _firstvx) delete [] (_firstvx);
   if (NULL != _sizslix) delete [] (_sizslix);
   if (NULL != _fstslix) delete [] (_fstslix);
}

//=============================================================================
//
// class Tolder
//
trend::Tolder::Tolder( layprop::DrawProperties* drawprop, real UU ) :
    TrendBase            (drawprop, UU),
    _sindex_array        (       NULL )
{
   _refLayer = DEBUG_NEW TolderRefLay();
}


void trend::Tolder::grid( const real, const std::string )
{
   //TODO
}

void trend::Tolder::setLayer(const LayerDef& laydef, bool has_selected)
{
   // Reference layer is processed differently (pushCell), so make sure
   // that we haven't got here with REF_LAY by accident
   assert(REF_LAY_DEF != laydef);
   if (NULL != _clayer)
   { // post process the current layer
      _clayer->ppSlice();
      _cslctd_array_offset += _clayer->total_slctdx();
   }
   if (_data.end() != _data.find(laydef))
   {
      _clayer = _data[laydef];
   }
   else
   {
      _clayer = DEBUG_NEW TolderLay();
      _data.add(laydef, _clayer);
   }
   if (has_selected)
      _clayer->newSlice(_cellStack.top(), _drawprop->layerFilled(laydef), false, _cslctd_array_offset);
   else
      _clayer->newSlice(_cellStack.top(), _drawprop->layerFilled(laydef), false);
}

void trend::Tolder::setGrcLayer(bool, const LayerDef&)
{
   //TODO
}

bool trend::Tolder::chunkExists(const LayerDef& laydef, bool has_selected)
{
   // Reference layer is processed differently (pushCell), so make sure
   // that we haven't got here with REF_LAY by accident
   assert(REF_LAY_DEF != laydef);
   if (NULL != _clayer)
   { // post process the current layer
      _clayer->ppSlice();
      _cslctd_array_offset += _clayer->total_slctdx();
   }
   if (_data.end() != _data.find(laydef))
   {
      _clayer = _data[laydef];
      if (_clayer->chunkExists(_cellStack.top(), _drawprop->layerFilled(laydef) ) ) return true;
   }
   else
   {
      _clayer = DEBUG_NEW TolderLay();
      _data.add(laydef, _clayer);
   }
   if (has_selected)
      _clayer->newSlice(_cellStack.top(), _drawprop->layerFilled(laydef), true, _cslctd_array_offset);
   else
      _clayer->newSlice(_cellStack.top(), _drawprop->layerFilled(laydef), true);
   return false;
}

bool trend::Tolder::collect()
{
   // First filter-out the layers that doesn't have any objects on them and
   // post process the last slices in the layers
   //
   DataLay::Iterator CCLAY = _data.begin();
   unsigned num_total_buffers = 0;
   unsigned num_total_slctdx = 0; // Initialise the number of total selected indexes
   unsigned num_total_strings = 0;
   while (CCLAY != _data.end())
   {
      CCLAY->ppSlice();
      num_total_strings += CCLAY->total_strings();
      if ((0 == CCLAY->total_points()) && (0 == CCLAY->total_strings()))
      {
         delete (*CCLAY);
         // Note! Careful here with the map iteration and erasing! Erase method
         // of map<> template doesn't return an iterator (unlike the list<>).
         // Despite the temptation to assume that the iterator will be valid after
         // the erase, it must be clear that erasing will invalidate the iterator.
         // If this is implemented more trivially using "for" cycle the code shall
         // crash, although it seems to work on certain platforms. Only seems -
         // it doesn't always crash, but it iterates in a weird way.
         // The implementation below seems to be the cleanest way to do this,
         // although it relies on my understanding of the way "++" operator should
         // be implemented
         _data.erase(CCLAY++());
      }
      else if (0 != CCLAY->total_points())
      {
         num_total_slctdx += CCLAY->total_slctdx();
         num_total_buffers++;
//         if (0 < CCLAY->total_indexs())
//            num_total_buffers++;
         ++CCLAY;
      }
      else
         ++CCLAY;
   }
   if (0 < _refLayer->total_points())  num_total_buffers++; // reference boxes
   if (0 < num_total_slctdx      )     num_total_buffers++;  // selected
   // Check whether we have to continue after traversing
   if (0 == num_total_buffers)
   {
      if (0 == num_total_strings)  return false;
      else                         return true;
   }
//   //--------------------------------------------------------------------------
//   // collect the point arrays
//   for (DataLay::Iterator CLAY = _data.begin(); CLAY != _data.end(); CLAY++)
//   {
//      if (0 == CLAY->total_points())
//      {
//         assert(0 != CLAY->total_strings());
//         continue;
//      }
//      CLAY->collect(_drawprop->layerFilled(CLAY()), 0, 0);
//   }
//   //--------------------------------------------------------------------------
//   // collect the indexes of the selected objects
//   if (0 < num_total_slctdx)
//   {// selected objects buffer
//      _sindex_array = DEBUG_NEW unsigned int[num_total_slctdx];
//      for (DataLay::Iterator CLAY = _data.begin(); CLAY != _data.end(); CLAY++)
//      {
//         if (0 == CLAY->total_slctdx())
//            continue;
//         CLAY->collectSelected(_sindex_array);
//      }
//   }
//   //--------------------------------------------------------------------------
//   // collect the reference boxes
//   if (0 < _refLayer->total_points())
//   {
//      _refLayer->collect(0);
//   }
   //
   // that's about it...
   return true;
}

bool trend::Tolder::grcCollect()
{
   //TODO
   return true;
}

void trend::Tolder::draw()
{
   for (DataLay::Iterator CLAY = _data.begin(); CLAY != _data.end(); CLAY++)
   {// for every layer
      _drawprop->setCurrentColor(CLAY());
      _drawprop->setCurrentFill(true); // force fill (ignore block_fill state)
      _drawprop->setLineProps(false);
      if (0 != CLAY->total_slctdx())
      {// redraw selected contours only
         _drawprop->setLineProps(true);
//         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _sbuffer);
         glPushMatrix();
         glMultMatrixd(_activeCS->translation());
         CLAY->drawSelected();
         glPopMatrix();
//         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
         _drawprop->setLineProps(false);
      }
      // draw everything
      if (0 != CLAY->total_points())
         CLAY->draw(_drawprop);
      // draw texts
      if (0 != CLAY->total_strings())
      {
         CLAY->drawTexts(_drawprop);
      }
   }
   // draw reference boxes
   if (0 < _refLayer->total_points())   _refLayer->draw(_drawprop);
   checkOGLError("draw");
}

void trend::Tolder::grcDraw()
{
   //TODO
}

void trend::Tolder::cleanUp()
{
   //TODO
}

void trend::Tolder::grcCleanUp()
{
   //TODO
}

trend::Tolder::~Tolder()
{
   delete _refLayer;
   if (NULL != _sindex_array)
      delete [] _sindex_array;

}
