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
//   This file is a part of Toped project (C) 2001-2008 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Sun May 04 2008
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: CIF parser
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include "tpdph.h"
#include <sstream>
#include "cif_io.h"
#include "cif_yacc.h"
#include "outbox.h"
#include "tedat.h"
#include "tedesign.h"


CIFin::CifFile* CIFInFile = NULL;
extern void*   new_cif_lex_buffer( FILE* cifin );
extern void    delete_cif_lex_buffer( void* b ) ;
extern int     cifparse(); // Calls the bison generated parser
extern FILE*   cifin;
extern int     cifdebug;
extern int     cifnerrs;


//=============================================================================
CIFin::CifBox::CifBox(CifData* last, dword length, dword width, TP* center, TP* direction) :
                           CifData(last), _length(length), _width(width), _center(center),
                                   _direction(direction) {}

CIFin::CifBox::~CifBox()
{
   delete _center;
   delete _direction;
}
//=============================================================================
CIFin::CifPoly::CifPoly(CifData* last, pointlist* poly) :
      CifData(last), _poly(poly) {}

CIFin::CifPoly::~CifPoly()
{
   delete _poly;
}
//=============================================================================
CIFin::CifWire::CifWire(CifData* last, pointlist* poly, dword width) :
      CifData(last), _poly(poly), _width(width) {}

CIFin::CifWire::~CifWire()
{
   delete _poly;
}
//=============================================================================
CIFin::CifRef::CifRef(CifData* last, dword cell, CTM* location) :
      CifData(last), _cell(cell), _location(location) {}

CIFin::CifRef::~CifRef()
{
   delete _location;
}
//=============================================================================
CIFin::CifLabelLoc::CifLabelLoc(CifData* last, std::string label, TP* location) :
      CifData(last), _label(label), _location(location) {}

CIFin::CifLabelLoc::~CifLabelLoc()
{
   delete _location;
}
//=============================================================================
CIFin::CifLabelSig::CifLabelSig(CifData* last, std::string label, TP* location) :
      CifLabelLoc(last, label, location)
{}

//=============================================================================
CIFin::CifLayer::CifLayer(std::string name, CifLayer* last):
      _name(name), _last(last), _first(NULL)
{}

CIFin::CifLayer::~CifLayer()
{
   CifData* wdata = _first;
   CifData* wdata4d;
   while (wdata)
   {
      wdata4d = wdata;
      wdata = wdata->last();
      delete wdata4d;
   }
}


void CIFin::CifLayer::addBox(dword length,dword width ,TP* center, TP* direction)
{
   _first = DEBUG_NEW CifBox(_first, length, width, center, direction);
}

void CIFin::CifLayer::addPoly(pointlist* poly)
{
   _first = DEBUG_NEW CifPoly(_first, poly);
}

void CIFin::CifLayer::addWire(pointlist* poly, dword width)
{
   _first = DEBUG_NEW CifWire(_first, poly, width);
}

void CIFin::CifLayer::addLabelLoc(std::string label, TP* loc)
{
   _first = DEBUG_NEW CifLabelLoc(_first, label, loc);
}

void CIFin::CifLayer::addLabelSig(std::string label, TP* loc)
{
   _first = DEBUG_NEW CifLabelSig(_first, label, loc);
}

//=============================================================================
CIFin::CifStructure::CifStructure(dword ID, CifStructure* last, dword a, dword b) :
      _ID(ID), _last(last), _a(a), _b(b), _name(""), _first(NULL),
          _refirst(NULL), _overlap(TP()), _orphan(true), _traversed(false) {}

CIFin::CifStructure::~CifStructure()
{
   // Remove all layers ...
   CifLayer* wlay = _first;
   CifLayer* wlay4d;
   while (NULL != wlay)
   {
      wlay4d = wlay;
      wlay = wlay->last();
      delete wlay4d;
   }
   // ... and all references
   CifRef* wref = _refirst;
   CifRef* wref4d;
   while (NULL != wref)
   {
      wref4d = wref;
      wref = wref->last();
      delete wref4d;
   }
}

CIFin::CifLayer* CIFin::CifStructure::secureLayer(std::string name)
{
   CifLayer* wlay = _first;
   while (NULL != wlay)
   {
      if (name == wlay->name()) return wlay;
      wlay = wlay->last();
   }
   _first = DEBUG_NEW CifLayer(name, _first);
   return _first;
}

void CIFin::CifStructure::collectLayers(nameList& layList, bool hier)
{
   CifLayer* wlay = _first;
   while (NULL != wlay)
   {
      layList.push_back(wlay->name());
      wlay = wlay->last();
   }
   layList.sort();
   layList.unique();

   if (!hier) return;
   for (CIFSList::const_iterator CCS = _children.begin(); CCS != _children.end(); CCS++)
      (*CCS)->collectLayers(layList, hier);
}

void CIFin::CifStructure::addRef(dword cell, CTM* location)
{
   _refirst = DEBUG_NEW CifRef(_refirst, cell, location);
}

void CIFin::CifStructure::hierPrep(CifFile& cfile)
{
   CifRef* _local = _refirst;
   while (NULL != _local)
   {
      CifStructure* celldef = cfile.getStructure(_local->cell());
      if (NULL != celldef)
      {
         celldef->parentFound();
         _children.push_back(celldef);
      }
      _local = _local->last();
   }
   _children.sort();
   _children.unique();

   if ("" == _name)
   {
      std::ostringstream tmp_name;
      tmp_name << "_cifCellNo_" << _ID;
      _name = tmp_name.str();
      std::ostringstream news;
      news << "Name \"" << _name << "\" assigned automatically to CIF cell "<< _ID ;
      tell_log(console::MT_INFO,news.str());
   }
}

CIFin::CIFHierTree* CIFin::CifStructure::hierOut(CIFHierTree* theTree, CifStructure* parent)
{
   // collecting hierarchical information
   theTree = DEBUG_NEW CIFHierTree(this, parent, theTree);
   for (CIFSList::iterator CI = _children.begin(); CI != _children.end(); CI++)
   {
      theTree = (*CI)->hierOut(theTree, this);
   }
   return theTree;
}

//=============================================================================
CIFin::CifFile::CifFile(std::string filename)
{
   _first = _current = _default = NULL;
   _curLay = NULL;
   _hierTree = NULL;
   _fileName = filename;
   std::ostringstream info;
   // Open the input file
	std::string fname(convertString(_fileName));
   if (!(_cifFh = fopen(fname.c_str(),"rt"))) // open the input file
   {
      _status = cfs_FNF; return;
   }
   // feed the flex with the buffer of the input file
   void* b = new_cif_lex_buffer( _cifFh );
   info << "Parsing \"" << _fileName << "\" using CIF grammar";
   tell_log(console::MT_INFO,info.str());
   CIFInFile = this;
   _default = DEBUG_NEW CifStructure(0,NULL);
   _default->cellNameIs(std::string(getFileNameOnly(filename) + "_cif"));

   // run the bison generated parser
   ciflloc.first_column = ciflloc.first_line = 1;
   ciflloc.last_column  = ciflloc.last_line  = 1;
/*   cifdebug = 1;*/
   try {
      // Note! the EXPTNcif_parser exception can be thrown from the
      // lexer, but only after a call from the parser
      cifparse();
      delete_cif_lex_buffer( b );
   }
   catch (EXPTNcif_parser)
   {
      // Not sure we can make something here.flex has thrown an exception
      // but it could be the file system or dynamic memory
      //@TODO check for available dynamic memory
      // see the same comment ted_prompt:307
      cifnerrs++;
   }
   if (cifnerrs > 0) _status = cfs_ERR;
   else              _status = cfs_POK;
   closeFile();
}

CIFin::CifFile::~CifFile()
{
   // delete all CIF structures
   CifStructure* local = _first;
   CifStructure* local4d;
   while (NULL != local)
   {
      local4d = local;
      local = local->last();
      delete local4d;
   }
   // get rid of the hierarchy tree
   const CIFHierTree* hlocal = _hierTree;
   const CIFHierTree* hlocal4d;
   while (hlocal)
   {
      hlocal4d = hlocal;
      hlocal = hlocal->GetLast();
      delete hlocal4d;
   }

   delete _default;
   closeFile();
}

void CIFin::CifFile::addStructure(dword ID, dword a, dword b)
{
   _first = DEBUG_NEW CifStructure(ID,_first, a,b);
   _current = _first;
}

void CIFin::CifFile::doneStructure()
{
   _current = _default;
}

void CIFin::CifFile::secureLayer(char* layname)
{
   if (NULL !=_current)
   {
      _curLay = _current->secureLayer(std::string(layname));
   }
   else assert(false); // Implement a scratch cell - CIF definition allows data definition ourside the cell boundary
}

void CIFin::CifFile::curCellName(char* cellname)
{
   if (NULL !=_current)
   {
      _current->cellNameIs(std::string(cellname));
   }
   else assert(false); // Implement a scratch cell - CIF definition allows data definition ourside the cell boundary
}

void CIFin::CifFile::curCellOverlap(TP* bl, TP* tr)
{
   if (NULL !=_current)
   {
      _current->cellOverlapIs(bl,tr);
   }
   else assert(false); // Implement a scratch cell - CIF definition allows data definition ourside the cell boundary
}

void CIFin::CifFile::addBox(dword length, dword width ,TP* center, TP* direction)
{
   _curLay->addBox(length, width, center, direction);
}

void CIFin::CifFile::addPoly(pointlist* poly)
{
   _curLay->addPoly(poly);
}

void CIFin::CifFile::addWire(pointlist* poly, dword width)
{
   _curLay->addWire(poly, width);
}

void CIFin::CifFile::addRef(dword cell, CTM* location)
{
   _current->addRef(cell, location);
}

void CIFin::CifFile::addLabelLoc(char* label, TP* location, char* layname)
{
   CifLayer* llay = _curLay;
   if (NULL != layname)
   {
      llay = _current->secureLayer(std::string(layname));
   }
   llay->addLabelLoc(std::string(label), location);
}

void CIFin::CifFile::addLabelSig(char* label, TP* location)
{
   _curLay->addLabelSig(std::string(label), location);
}

void CIFin::CifFile::collectLayers(nameList& cifLayers)
{
   CifStructure* local = _first;
   while (NULL != local)
   {
      local->collectLayers(cifLayers, false);
      local = local->last();
   }
   cifLayers.sort();
   cifLayers.unique();
}

CIFin::CifStructure* CIFin::CifFile::getStructure(dword cellno)
{
   CifStructure* local = _first;
   while (NULL != local)
   {
      if (cellno == local->ID())
         return local;
      local = local->last();
   }
   assert(false); // Cell with this number not found ?!
   return NULL;
}

CIFin::CifStructure* CIFin::CifFile::getStructure(std::string cellname)
{
   if (cellname == _default->name()) return _default;
   CifStructure* local = _first;
   while (NULL != local)
   {
      if (cellname == local->name())
         return local;
      local = local->last();
   }
   return NULL; // Cell with this name not found ?!
}

void CIFin::CifFile::hierPrep()
{
   _default->hierPrep(*this);
   CifStructure* local = _first;
   while (NULL != local)
   {
      local->hierPrep(*this);
      local = local->last();
   }
}

void CIFin::CifFile::hierOut()
{
   _hierTree = _default->hierOut(_hierTree,NULL);

   CifStructure* local = _first;
   while (NULL != local)
   {
      if (local->orphan())
         _hierTree = local->hierOut(_hierTree,NULL);
      local = local->last();
   }
}

void CIFin::CifFile::closeFile()
{
   if (NULL != _cifFh)
   {
      fclose(_cifFh); _cifFh = NULL;
   }
   CIFInFile = NULL;
}

//=============================================================================
CIFin::CifExportFile::CifExportFile(std::string fn, laydata::tdtcell* topcell,
   USMap* laymap, bool recur, bool verbose) :  DbExportFile(fn, topcell, recur),
      _laymap(laymap), _verbose(verbose), _lastcellnum(0)
{
   std::string fname(convertString(_fileName));
   _file.open(_fileName.c_str(), std::ios::out);
   //@TODO how to check for an error ?
   // start writing
   TpdTime timec(time(NULL));

   _file << "(              CIF   2.0       );"    << std::endl;
   _file << "(        generator : Toped 0.9.x );"  << std::endl;
   _file << "(             user : tbd );"          << std::endl;
   _file << "(          machine : tbd );"          << std::endl;
   _file << "(       time stamp : " << timec() << ");" << std::endl;

}

bool CIFin::CifExportFile::checkCellWritten(std::string cellname) const
{
   return (_cellmap.end() != _cellmap.find(cellname));
}

void CIFin::CifExportFile::registerCellWritten(std::string cellname)
{
   assert(_cellmap.end() == _cellmap.find(cellname));
   _cellmap[cellname] = ++_lastcellnum;
}

void CIFin::CifExportFile::definitionStart(std::string name)
{
   std::string message = "...converting " + name;
   unsigned dbuu = (unsigned) (1.0/_DBU);
   // clean the error from the conversion (round to step 10)
   dbuu = (int4b) (rint((dbuu + (5)) / 10) * 10);
   unsigned cifu = 100000000;
   unsigned gcd = GCD(dbuu,cifu); // get the greatest common denominator
   unsigned bfact = dbuu / gcd;
   unsigned afact = cifu / gcd;
   tell_log(console::MT_INFO, message);
   registerCellWritten(name);
   if (_verbose)
      _file << std::endl << "Definition Start #" << _lastcellnum << "with a = " << afact << " and b = " << bfact<< ";"<< std::endl;
   else
      _file << std::endl << "DS " << _lastcellnum << " " << afact << " " << bfact <<";" << std::endl;
   _file << "   9 "<< name << ";"<< std::endl;
}

void CIFin::CifExportFile::definitionFinish()
{
   if (_verbose)
      _file << "Definition Finish;" << std::endl;
   else
      _file << "DF;" << std::endl;
}

void CIFin::CifExportFile::libraryStart(std::string libname, TpdTime& libtime, real DBU, real UU)
{
   _file << "(       TDT source : " << libname << ");" << std::endl;
   _file << "(    Last Modified : " << libtime() << ");" << std::endl;

   if (NULL == _topcell)
   {
      _file << "(         Top Cell :  - );" << std::endl;
   }
   else
   {
      _file << "(         Top Cell : " << _topcell->name() << ");" << std::endl;
   }
   _DBU = DBU;
   _UU = UU;
}

void CIFin::CifExportFile::libraryFinish()
{
   // nothing to do for CIF export
   assert(false);
}

bool CIFin::CifExportFile::layerSpecification(unsigned layno)
{
   if (REF_LAY == layno) return true;
   if (_laymap->end() == _laymap->find(layno))
   {
      //std::stringstream message;
      //message << "   Layer " << layno <<" not found in the layer map and will not be converted";
      //tell_log(console::MT_INFO, message.str());
      return false;
   }
   if (_verbose)
      _file << "   Layer "<< (*_laymap)[layno] << " objects follow;" << std::endl;
   else
      _file << "L " << (*_laymap)[layno] << ";" << std::endl;
   return true;
}

void CIFin::CifExportFile::box(const int4b* const pdata)
{
   unsigned int length = abs(pdata[4] - pdata[0]);
   unsigned int width  = abs(pdata[5] - pdata[1]);
   TP center((pdata[4] + pdata[0]) / 2, (pdata[5] + pdata[1]) / 2);

   if (_verbose)
      _file << "      Box length = "<< length << " width = "<< width <<
            " and center = " << center.x() << "," << center.y() << ";" << std::endl;
   else
      _file << "      B"<< length << " " << width << " " << center.x() <<
            " " << center.y() << ";" << std::endl;
}

void CIFin::CifExportFile::polygon(const int4b* const pdata, unsigned psize)
{
   if (_verbose)
      _file <<"      Polygon with vertices";
   else
      _file <<"      P";
   for (unsigned i = 0; i < psize; i++)
      _file << " " << pdata[2*i] << " " << pdata[2*i+1];
   _file << ";"<< std::endl;
}

void CIFin::CifExportFile::wire(const int4b* const pdata, unsigned psize, unsigned width)
{
   if (_verbose)
      _file <<"      Wire width = " << width << "and points";
   else
      _file <<"      W" << width;
   for (unsigned i = 0; i < psize; i++)
      _file << " " << pdata[2*i] << " " << pdata[2*i+1];
   _file << ";"<< std::endl;
}

void CIFin::CifExportFile::text(const std::string& label, const CTM& trans)
{
   int loc;
   std::string labelr(label);
   while ((loc = labelr.find(' ')) >= 0 )
      labelr.replace(loc, 1, "_"); //@FIXME - this should be an option or ...?

   _file << "      94 "<< labelr << " "<< (int)trans.tx() << " " << (int)trans.ty() << ";" << std::endl;


}

void CIFin::CifExportFile::ref(const std::string& cellname, const CTM& tmatrix)
{
   assert(_cellmap.end() != _cellmap.find(cellname));

   TP trans;
   real rot;
   real scale;
   bool flipX;

   tmatrix.Decompose(trans, rot, scale, flipX);
   if (1.0 != scale) assert(false); //@TODO CIF scaling ???
   int4b resultX = static_cast<int4b>(cos(rot*M_PI/180) * 1e6);
   int4b resultY = static_cast<int4b>(sin(rot*M_PI/180) * 1e6);
   if       (0 == resultX) resultY = abs(resultY) / resultY;
   else if (0 == resultY) resultX = abs(resultX) / resultX;
   else if (abs(resultX) == abs(resultY))
   {
      resultX = abs(resultX) / resultX;
      resultY = abs(resultY) / resultY;
   }
   else if (0 == (resultX % resultY))
      resultX /= resultY;
   else if (0 == (resultY % resultX))
      resultY /= resultX;

   if (_verbose)
   {
      _file <<"      Call symbol #" << _cellmap[cellname];
      if (       flipX) _file << " Mirrored in Y";
      if (0.0 != rot  ) _file << " Rotated to " << resultX << " " << resultY;
      _file << " Translated to " << trans.x() << " " << trans.y();
   }
   else
   {
      _file <<"      C" << _cellmap[cellname];
      if (       flipX) _file << " MY";
      if (0.0 != rot  ) _file << " R " << resultX << " " << resultY;
      _file << " T" << trans.x() << " " << trans.y();
   }
   _file << ";"<< std::endl;
}

void CIFin::CifExportFile::aref(const std::string& name,
                                const CTM& translation, const laydata::ArrayProperties& arrprops)
{
   for (int i = 0; i < arrprops.cols(); i++)
   {// start/stop rows
      for(int j = 0; j < arrprops.rows(); j++)
      { // start/stop columns
         // ... get the translation matrix ...
         CTM refCTM(TP(arrprops.stepX() * i , arrprops.stepY() * j ), 1, 0, false);
         refCTM *= translation;
         ref(name, refCTM);
      }
   }
}

CIFin::CifExportFile::~CifExportFile()
{
   _file << "End" << std::endl;
   _file.close();
   // don't delete _laymap - it's should be deleted where it had been created
}

//-----------------------------------------------------------------------------
// class Cif2Ted
//-----------------------------------------------------------------------------
CIFin::Cif2Ted::Cif2Ted(CIFin::CifFile* src_lib, laydata::tdtlibdir* tdt_db,
      SIMap* cif_layers, real techno) : _src_lib (src_lib), _tdt_db(tdt_db),
                                    _cif_layers(cif_layers), _techno(techno)
{
   _dbucoeff = 1e-8/(*_tdt_db)()->DBU();
}


void CIFin::Cif2Ted::top_structure(std::string top_str, bool recursive, bool overwrite)
{
   assert(_src_lib->hiertree());
   CIFin::CifStructure *src_structure = _src_lib->getStructure(top_str);
   if (NULL != src_structure)
   {
      CIFin::CIFHierTree* root = _src_lib->hiertree()->GetMember(src_structure);
      if (recursive) child_structure(root, overwrite);
      convert_prep(root, overwrite);
      root = root->GetNextRoot(TARGETDB_LIB);
   }
   else
   {
      std::ostringstream ost; ost << "CIF import: ";
      ost << "Structure \""<< top_str << "\" not found in the CIF DB in memory.";
      tell_log(console::MT_WARNING,ost.str());
   }
   // Convert the top structure
   //   hCellBrowser->AddRoot(wxString((_src_lib->Get_libname()).c_str(), wxConvUTF8));

}

void CIFin::Cif2Ted::child_structure(const CIFin::CIFHierTree* root, bool overwrite)
{
   const CIFin::CIFHierTree* Child= root->GetChild(TARGETDB_LIB);
   while (Child)
   {
      if ( !Child->GetItem()->traversed() )
      {
         // traverse children first
         child_structure(Child, overwrite);
         convert_prep(Child, overwrite);
      }
      Child = Child->GetBrother(TARGETDB_LIB);
   }
}

void CIFin::Cif2Ted::convert_prep(const CIFin::CIFHierTree* item, bool overwrite)
{
   CIFin::CifStructure* src_structure = const_cast<CIFin::CifStructure*>(item->GetItem());
   std::string gname = src_structure->name();
   // check that destination structure with this name exists
   laydata::tdtcell* dst_structure = static_cast<laydata::tdtcell*>((*_tdt_db)()->checkcell(gname));
   std::ostringstream ost; ost << "CIF import: ";
   if (NULL != dst_structure)
   {
      if (overwrite)
      {
         /*@TODO Erase the existing structure and convert*/
         ost << "Structure "<< gname << " should be overwritten, but cell erase is not implemened yet ...";
         tell_log(console::MT_WARNING,ost.str());
      }
      else
      {
         ost << "Structure "<< gname << " already exists. Omitted";
         tell_log(console::MT_INFO,ost.str());
      }
   }
   else
   {
      ost << "Importing structure " << gname << "...";
      tell_log(console::MT_INFO,ost.str());
      // first create a new cell
      dst_structure = (*_tdt_db)()->addcell(gname, _tdt_db);
      // finally call the cell converter
      convert(src_structure, dst_structure);
   }
   src_structure->set_traversed(true);
}


void CIFin::Cif2Ted::convert(CIFin::CifStructure* src, laydata::tdtcell* dst)
{
   _crosscoeff = _dbucoeff * src->a() / src->b();
   CIFin::CifLayer* swl = src->firstLayer();
   while( swl ) // loop trough the layers
   {
      if (_cif_layers->end() != _cif_layers->find(swl->name()))
      {
         laydata::tdtlayer* dwl =
               static_cast<laydata::tdtlayer*>(dst->securelayer((*_cif_layers)[swl->name()]));
         CIFin::CifData* wd = swl->firstData();
         while ( wd ) // loop trough data
         {
            switch (wd->dataType())
            {
               case cif_BOX     : box ( static_cast<CIFin::CifBox*     >(wd), dwl, swl->name() );break;
               case cif_POLY    : poly( static_cast<CIFin::CifPoly*    >(wd), dwl, swl->name() );break;
               case cif_WIRE    : wire( static_cast<CIFin::CifWire*    >(wd), dwl, swl->name() );break;
               case cif_LBL_LOC : lbll( static_cast<CIFin::CifLabelLoc*>(wd), dwl, swl->name() );break;
               case cif_LBL_SIG : lbls( static_cast<CIFin::CifLabelSig*>(wd), dwl, swl->name() );break;
               default    : assert(false);
            }
            wd = wd->last();
         }
      }
      //else
      //{
      //   std::ostringstream ost;
      //   ost << "CIF Layer name \"" << swl->name() << "\" is not defined in the function input parameter. Will be omitted";
      //   tell_log(console::MT_INFO,ost.str());
      //}
      swl = swl->last();
   }

   CIFin::CifRef* swr = src->refirst();
   while ( swr )
   {
      ref(swr,dst);
      swr = swr->last();
   }
   dst->resort();
}

void CIFin::Cif2Ted::box ( CIFin::CifBox* wd, laydata::tdtlayer* wl, std::string layname)
{
   pointlist pl;   pl.reserve(4);
   real cX, cY;

   cX = rint(((real)wd->center()->x() - (real)wd->length()/ 2.0f) * _crosscoeff );
   cY = rint(((real)wd->center()->y() - (real)wd->width() / 2.0f) * _crosscoeff );
   TP cpnt1( (int4b)cX, (int4b)cY );   pl.push_back(cpnt1);

   cX = rint(((real)wd->center()->x() + (real)wd->length()/ 2.0f) * _crosscoeff );
   cY = rint(((real)wd->center()->y() - (real)wd->width() / 2.0f) * _crosscoeff );
   TP cpnt2( (int4b)cX, (int4b)cY );   pl.push_back(cpnt2);

   cX = rint(((real)wd->center()->x() + (real)wd->length()/ 2.0f) * _crosscoeff );
   cY = rint(((real)wd->center()->y() + (real)wd->width() / 2.0f) * _crosscoeff );
   TP cpnt3( (int4b)cX, (int4b)cY );   pl.push_back(cpnt3);

   cX = rint(((real)wd->center()->x() - (real)wd->length()/ 2.0f) * _crosscoeff );
   cY = rint(((real)wd->center()->y() + (real)wd->width() / 2.0f) * _crosscoeff );
   TP cpnt4( (int4b)cX, (int4b)cY );   pl.push_back(cpnt4);
   if (NULL != wd->direction())
   {
      CTM tmx;
      cX = (real)wd->center()->x() * _crosscoeff;
      cY = (real)wd->center()->y() * _crosscoeff;
      tmx.Translate(-cX,-cY);
      tmx.Rotate(*(wd->direction()));
      tmx.Translate(cX,cY);
      pl[0] *=  tmx;
      pl[1] *=  tmx;
      pl[2] *=  tmx;
      pl[3] *=  tmx;
   }

   laydata::valid_poly check(pl);

   if (!check.valid())
   {
      std::ostringstream ost; ost << "Layer " << layname;
      ost << ": Box check fails - " << check.failtype();
      tell_log(console::MT_ERROR, ost.str());
   }
   else pl = check.get_validated() ;

   if (check.box())  wl->addbox(pl[0], pl[2],false);
   else              wl->addpoly(pl,false);
}

void CIFin::Cif2Ted::poly( CIFin::CifPoly* wd, laydata::tdtlayer* wl, std::string layname)
{
   pointlist pl;
   pl.reserve(wd->poly()->size());
   for(pointlist::const_iterator CP = wd->poly()->begin(); CP != wd->poly()->end(); CP++)
   {
      TP pnt(*CP);
      pnt *= _crosscoeff;
      pl.push_back(pnt);
   }
   laydata::valid_poly check(pl);

   if (!check.valid())
   {
      std::ostringstream ost; ost << "Layer " << layname;
      ost << ": Polygon check fails - " << check.failtype();
      tell_log(console::MT_ERROR, ost.str());
   }
   else pl = check.get_validated() ;

   if (check.box())  wl->addbox(pl[0], pl[2],false);
   else              wl->addpoly(pl,false);
}

void CIFin::Cif2Ted::wire( CIFin::CifWire* wd, laydata::tdtlayer* wl, std::string layname)
{
   pointlist pl;
   pl.reserve(wd->poly()->size());
   for(pointlist::const_iterator CP = wd->poly()->begin(); CP != wd->poly()->end(); CP++)
   {
      TP pnt(*CP);
      pnt *= _crosscoeff;
      pl.push_back(pnt);
   }
   laydata::valid_wire check(pl, wd->width());

   if (!check.valid())
   {
      std::ostringstream ost; ost << "Layer " << layname;
      ost << ": Wire check fails - " << check.failtype();
      tell_log(console::MT_ERROR, ost.str());
   }
   else pl = check.get_validated() ;
   wl->addwire(pl, wd->width(),false);
}

void CIFin::Cif2Ted::ref ( CIFin::CifRef* wd, laydata::tdtcell* dst)
{
   CifStructure* refd = _src_lib->getStructure(wd->cell());
   std::string cell_name = refd->name();
   if (NULL != (*_tdt_db)()->checkcell(cell_name))
   {
      laydata::CellDefin strdefn = (*_tdt_db)()->getcellnamepair(cell_name);
      // Absolute magnification, absolute angle should be reflected somehow!!!
      dst->addcellref((*_tdt_db)(), strdefn, (*wd->location())*_crosscoeff, false);
   }
   else
   {
      std::string news = "Referenced structure \"";
      news += cell_name; news += "\" not found. Reference ignored";
      tell_log(console::MT_ERROR,news);
   }
}

void CIFin::Cif2Ted::lbll( CIFin::CifLabelLoc* wd, laydata::tdtlayer* wl, std::string )
{
   // CIF doesn't have a concept of texts (as GDS)
   // text size and placement are just the default
   if (0.0 == _techno) return;
   TP pnt(*(wd->location()));
   pnt *= _crosscoeff;
   wl->addtext(wd->text(),
               CTM(pnt,
                   (_techno / (/*(*_tdt_db)()->UU() * */ OPENGL_FONT_UNIT)),
                   0.0,
                   false )
              );
}

void CIFin::Cif2Ted::lbls( CIFin::CifLabelSig*,laydata::tdtlayer*, std::string )
{
}

