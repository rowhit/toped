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
//        Created: Sat Jan 10 2004
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: TDT I/O and database access control
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#ifndef DATA_HANDLER_INCLUDED
#define DATA_HANDLER_INCLUDED
#include "tedesign.h"
#include "../tpd_ifaces/gds_io.h"
#include "../tpd_ifaces/cif_io.h"
#include "viewprop.h"


namespace CIFin {

   class Cif2Ted {
      public:
                              Cif2Ted(CIFin::CifFile*, laydata::tdtlibdir*, SIMap*, real);
         void                 top_structure(std::string, bool, bool);
      protected:
         void                 child_structure(const CIFin::CIFHierTree*, bool);
         void                 convert_prep(const CIFin::CIFHierTree* item, bool);
         void                 convert(CIFin::CifStructure*, laydata::tdtcell*);
         void                 box ( CIFin::CifBox*     ,laydata::tdtlayer*, std::string );
         void                 poly( CIFin::CifPoly*    ,laydata::tdtlayer*, std::string );
         void                 wire( CIFin::CifWire*    ,laydata::tdtlayer*, std::string );
         void                 ref ( CIFin::CifRef*     ,laydata::tdtcell*);
         void                 lbll( CIFin::CifLabelLoc*,laydata::tdtlayer*, std::string );
         void                 lbls( CIFin::CifLabelSig*,laydata::tdtlayer*, std::string );
         CIFin::CifFile*      _src_lib;
         laydata::tdtlibdir*  _tdt_db;
         SIMap*               _cif_layers;
         real                 _crosscoeff;
         real                 _dbucoeff;
         real                 _techno;
   };

}

class DataCenter {
public:
                              DataCenter(const std::string&, const std::string &);
                             ~DataCenter(); 
   bool                       GDSparse(std::string);
   void                       GDSexport(const LayerMapGds&, std::string&, bool);
   void                       GDSexport(laydata::tdtcell*, const LayerMapGds&, bool, std::string&, bool);
   void                       GDSsplit(GDSin::GdsStructure*, const std::string filename, bool recur);
   void                       importGDScell(const nameList&, const LayerMapGds&, bool recur, bool over);
   void                       GDSclose();
   void                       CIFclose();
   CIFin::CifStatusType       CIFparse(std::string filename);
   void                       CIFexport(USMap*, bool, std::string&);
   void                       CIFexport(laydata::tdtcell*, USMap*, bool, bool, std::string&);
   bool                       cifGetLayers(nameList&);
   bool                       gdsGetLayers(GdsLayers&);
   void                       CIFimport(const nameList&, SIMap*, bool, bool, real);
   void                       PSexport(laydata::tdtcell*, std::string&);
   bool                       TDTread(std::string);
   int                        TDTloadlib(std::string);
   bool                       TDTunloadlib(std::string);
   bool                       TDTwrite(const char* filename = NULL);
   bool                       TDTcheckwrite(const TpdTime&, const TpdTime&, bool&); 
   bool                       TDTcheckread(const std::string, const TpdTime&, const TpdTime&, bool&); 
   void                       newDesign(std::string, time_t);
   laydata::tdtdesign*        lockDB(bool checkACTcell = true);
   bool                       lockGds(GDSin::GdsFile*&);
   bool                       lockCif(CIFin::CifFile*&);
   laydata::tdtlibrary*       getLib(int libID) {return _TEDLIB.getLib(libID);}
   int                        getLastLibRefNo() {return _TEDLIB.getLastLibRefNo();}
   bool                       getCellNamePair(std::string name, laydata::CellDefin& strdefn);
   void                       unlockDB();
   void                       unlockGds(GDSin::GdsFile*&, bool throwexception = false);
   void                       unlockCif(CIFin::CifFile*&, bool throwexception = false);
   void                       mouseStart(int input_type, std::string, const CTM, int4b, int4b, word, word);
   void                       mousePointCancel(TP&);
   void                       mousePoint(TP p);
   void                       mouseStop();
   void                       mouseFlip();
   void                       mouseRotate();
   void                       tmp_draw(const CTM&, TP, TP);
   void                       render(const CTM&);
   const laydata::cellList&   cells();
   laydata::tdtlibdir*        TEDLIB() {return &_TEDLIB;}
   laydata::LibCellLists*     getCells(int libID);
   unsigned int               numselected()           {return (NULL != _TEDLIB()) ? _TEDLIB()->numselected() : 0 ;}
   void                       defaultlayer(word layno){_curlay = layno;}
   void                       setcmdlayer(word layno) {_curcmdlay = layno;}
   word                       curlay() const          {return _curlay;}
   word                       curcmdlay() const       {return _curcmdlay;}
   std::string                tedfilename() const     {return _tedfilename;};
   bool                       neversaved()  const     {return _neversaved;}; 
   bool                       modified() const        {return _TEDLIB.modified();};

   //------------------------------------------------------------------------------------------------
   bool                       addlayer(std::string, unsigned, std::string, std::string, std::string);
   bool                       addlayer(std::string, unsigned);
   bool                       addlayer(unsigned layno);
   unsigned                   addlayer(std::string);
   bool                       isLayerExist(word);
   bool                       isLayerExist(std::string);
   void                       addline(std::string, std::string, word, byte, byte);
   void                       addcolor(std::string, byte, byte, byte, byte);
   void                       addfill(std::string, byte*);
   void                       hideLayer(word, bool);
   void                       lockLayer(word, bool);
   void                       fillLayer(word, bool);
   void                       setcellmarks_hidden(bool);
   void                       settextmarks_hidden(bool);
   void                       setcellbox_hidden(bool);
   void                       settextbox_hidden(bool);
   void                       setGrid(byte, real, std::string);
   bool                       viewGrid(byte, bool);
   void                       addRuler(TP&, TP&);
   void                       clearRulers();
   void                       switch_drawruler(bool st) {_drawruler = st;}
   bool                       drawruler() {return _drawruler;}
   LayerMapGds*               secureGdsLayMap(bool);
   LayerMapCif*               secureCifLayMap(bool);
   bool                       autopan() const         {return _properties.autopan();}
   bool                       zeroCross() const       {return _properties.zeroCross();}
   const real                 step() const            {return _properties.step();}
   const layprop::LayoutGrid* grid(byte gn) const     {return _properties.grid(gn);}
   const int4b                stepDB() const          {return _properties.stepDB();}
   const real                 UU() const              {return _properties.UU();}
   const real                 DBscale() const         {return _properties.DBscale();}
   unsigned                   getLayerNo(std::string name) const
                                                      {return _properties.getLayerNo(name);}
   std::string                getLayerName(word layno) const
                                                      {return _properties.getLayerName(layno);}
   byte                       marker_angle() const    {return _properties.marker_angle();}
   bool                       layerHidden(word layno) {return _properties.drawprop().layerHidden(layno);}
   bool                       layerLocked(word layno) {return _properties.drawprop().layerLocked(layno);}
   const WordList             getAllLayers(void)      {return _properties.getAllLayers();};
   const WordList             getLockedLayers(void)   {return _properties.getLockedLayers();};
   bool                       grid_visual(word no)    {return grid(no)->visual();}
   void                       setautopan(bool status) {_properties.setautopan(status);}
   void                       setZeroCross(bool status) {_properties.setZeroCross(status);}
   void                       setmarker_angle(byte angle)
                                                      {_properties.setmarker_angle(angle);}
   void                       setstep(real st)        {_properties.setstep(st);}
   void                       setClipRegion(DBbox clipR)
                                                      {_properties.setClipRegion(clipR);}
   void                       setScrCTM(CTM ScrCTM)   {_properties.setScrCTM(ScrCTM);}
   void                       setCurrentOp(console::ACTIVE_OP op)
                                                      {_properties.setCurrentOp(op);}
   const console::ACTIVE_OP   currentop() const       {return _properties.currentop();}
   void                       all_layers(nameList& laylist) const {_properties.all_layers(laylist);}
   void                       all_colors(nameList& colist)  const {_properties.all_colors(colist); }
   void                       all_fills(nameList& filist)   const {_properties.all_fills(filist);  }
   void                       all_lines(nameList& linelist) const {_properties.all_lines(linelist);}
   bool                       isFilled(unsigned layno) {return _properties.drawprop().isFilled(layno);}
   const byte*                getFill(word layno) {return _properties.drawprop().getFill(layno);}
   const byte*                getFill(std::string fill_name) {return _properties.drawprop().getFill(fill_name);}
   const layprop::tellRGB&    getColor(word layno) {return _properties.drawprop().getColor(layno);}
   const layprop::tellRGB&    getColor(std::string color_name) {return _properties.drawprop().getColor(color_name);}
   const layprop::LineSettings* getLine(word layno) {return _properties.drawprop().getLine(layno);}
   const layprop::LineSettings* getLine(std::string line_name) {return _properties.drawprop().getLine(line_name);}
   const std::string          getColorName(word layno) {return _properties.drawprop().getColorName(layno);}
   const std::string          getFillName(word layno) {return _properties.drawprop().getFillName(layno);}
   const std::string          getLineName(word layno) {return _properties.drawprop().getLineName(layno);}
   const WordList             upLayers() {return _properties.upLayers();}
   void                       clearUnpublishedLayers() {_properties.clearUnpublishedLayers();}
   const word                 layselmask() {return _properties.layselmask();}
   void                       setlayselmask(word lsm) {_properties.setlayselmask(lsm);}
   void                       setGdsLayMap(USMap* map)   {_properties.setGdsLayMap(map);}
   void                       setCifLayMap(USMap* map)   {_properties.setCifLayMap(map);}
   const USMap*               getGdsLayMap() const       {return _properties.getGdsLayMap();}
   const USMap*               getCifLayMap() const       {return _properties.getCifLayMap();}
   void                       saveProperties(std::string fname)
                                                      {_properties.saveProperties(fname);}
   std::string                globalDir(void) const
                                                      {return _globalDir;}
   void                       loadLayoutFonts(std::string ffn, bool vbo)
                                                      {_properties.loadLayoutFonts(ffn, vbo);}

protected:
   std::string                _tedfilename;
   bool                       _neversaved;
   void                       openGL_draw(const CTM&);
   void                       openGL_render(const CTM&); // alternative to openGL_draw
private:
   word                       _curlay;       // current drawing layer
   word                       _curcmdlay;    // layer used during current drawing operation
   bool                       _drawruler;    // draw a ruler while coposing a shape interactively
   std::string                _localDir;
   std::string                _globalDir;
   laydata::tdtlibdir         _TEDLIB;       // catalog of available TDT libraries
   GDSin::GdsFile*            _GDSDB;        // GDS parsed data
   CIFin::CifFile*            _CIFDB;        // CIF parsed data
   layprop::ViewProperties    _properties;   // properties data base
   wxMutex                    DBLock;
   wxMutex                    GDSLock;
   wxMutex                    CIFLock;
   wxMutex                    PROPLock;

};

void initDBLib(const std::string&, const std::string&);

#endif
