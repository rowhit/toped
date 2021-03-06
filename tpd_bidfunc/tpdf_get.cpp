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
//   This file is a part of Toped project (C) 2001-2012 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Thu Apr 19 BST 2007 (from tellibin.h Fri Jan 24 2003)
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Definition of all TOPED database functions
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include "tpdph.h"
#include <sstream>
#include "tpdf_get.h"
#include "tedat.h"
#include "auxdat.h"
#include "datacenter.h"
#include "viewprop.h"

extern DataCenter*               DATC;
extern layprop::PropertyCenter*  PROPC;
extern console::toped_logfile    LogFile;
extern void tellerror(std::string s);

//=============================================================================
tellstdfunc::stdGETLAYTYPE::stdGETLAYTYPE(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::ArgumentLIST,retype, eor)
{
   _arguments->push_back(DEBUG_NEW ArgumentTYPE("", DEBUG_NEW telldata::TtLayout()));
}

int tellstdfunc::stdGETLAYTYPE::execute()
{
   telldata::TtLayout* tx = static_cast<telldata::TtLayout*>(OPstack.top());OPstack.pop();
   OPstack.push(DEBUG_NEW telldata::TtInt(tx->data()->lType()));
   delete tx;
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::stdGETLAYTEXTSTR::stdGETLAYTEXTSTR(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::ArgumentLIST,retype, eor)
{
   _arguments->push_back(DEBUG_NEW ArgumentTYPE("", DEBUG_NEW telldata::TtLayout()));
}

int tellstdfunc::stdGETLAYTEXTSTR::execute()
{
   telldata::TtLayout* tx = static_cast<telldata::TtLayout*>(OPstack.top());OPstack.pop();
   if (laydata::_lmtext != tx->data()->lType())
   {
      tellerror(std::string("Runtime error.Invalid layout type")); // FIXME?! tellerror is a parser function. MUST not be used during runtime
      delete tx;
      return EXEC_ABORT;
   }
   else
   {
      OPstack.push(DEBUG_NEW telldata::TtString(static_cast<laydata::TdtText*>(tx->data())->text()));
      delete tx;
      return EXEC_NEXT;
   }
}

//=============================================================================
tellstdfunc::stdGETLAYREFSTR::stdGETLAYREFSTR(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::ArgumentLIST,retype, eor)
{
   _arguments->push_back(DEBUG_NEW ArgumentTYPE("", DEBUG_NEW telldata::TtLayout()));
}

int tellstdfunc::stdGETLAYREFSTR::execute()
{
   telldata::TtLayout* tx = static_cast<telldata::TtLayout*>(OPstack.top());OPstack.pop();
   if ((laydata::_lmref != tx->data()->lType()) && (laydata::_lmaref != tx->data()->lType()))
   {
      tellerror(std::string("Runtime error.Invalid layout type")); // FIXME?! tellerror is a parser function. MUST not be used during runtime
      delete tx;
      return EXEC_ABORT;
   }
   else
   {
      OPstack.push(DEBUG_NEW telldata::TtString(static_cast<laydata::TdtCellRef*>(tx->data())->cellname()));
      delete tx;
      return EXEC_NEXT;
   }
}
//=============================================================================
tellstdfunc::stdGETOVERLAP::stdGETOVERLAP(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::ArgumentLIST,retype, eor)
{
   _arguments->push_back(DEBUG_NEW ArgumentTYPE("", DEBUG_NEW telldata::TtLayout()));
}

int tellstdfunc::stdGETOVERLAP::execute()
{
   telldata::TtLayout* layObject = static_cast<telldata::TtLayout*>(OPstack.top());OPstack.pop();
   assert(layObject);
   real DBscale = PROPC->DBscale();
   DBbox ovlBox = layObject->data()->overlap();
   telldata::TtPnt p1DB(ovlBox.p1().x()/DBscale, ovlBox.p1().y()/DBscale );
   telldata::TtPnt p2DB(ovlBox.p2().x()/DBscale, ovlBox.p2().y()/DBscale);

   OPstack.push(DEBUG_NEW telldata::TtBox( p1DB, p2DB ));
   delete layObject;
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::stdGETOVERLAPLST::stdGETOVERLAPLST(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::ArgumentLIST,retype, eor)
{
   _arguments->push_back(DEBUG_NEW ArgumentTYPE("", DEBUG_NEW telldata::TtList(telldata::tn_layout)));
}

int tellstdfunc::stdGETOVERLAPLST::execute()
{
   telldata::TtList* layList = static_cast<telldata::TtList*>(OPstack.top());OPstack.pop();
   assert(layList);

//   DBbox ovlBox = layObject->data()->overlap();
   real DBscale = PROPC->DBscale();
   DBbox ovlBox(DEFAULT_OVL_BOX);
   for (unsigned i = 0; i < layList->size(); i++)
   {
      ovlBox.overlap(static_cast<telldata::TtLayout*>(layList->index_var(i))->data()->overlap());
   }
   telldata::TtPnt p1DB(ovlBox.p1().x()/DBscale, ovlBox.p1().y()/DBscale );
   telldata::TtPnt p2DB(ovlBox.p2().x()/DBscale, ovlBox.p2().y()/DBscale);
   OPstack.push(DEBUG_NEW telldata::TtBox( p1DB, p2DB ));
   delete layList;
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::grcGETCELLS::grcGETCELLS(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::ArgumentLIST,retype, eor)
{
}

int tellstdfunc::grcGETCELLS::execute()
{
   telldata::TtList* tllull = DEBUG_NEW telldata::TtList(telldata::tn_string);
   laydata::TdtLibDir* dbLibDir = NULL;
   if (DATC->lockTDT(dbLibDir, dbmxs_dblock))
   {
      laydata::LibCellLists *cll =  dbLibDir->getCells(TARGETDB_LIB);
      for (laydata::LibCellLists::iterator curlib = cll->begin(); curlib != cll->end(); curlib++)
      {
         laydata::CellMap::const_iterator CL;
         for (CL = (*curlib)->begin(); CL != (*curlib)->end(); CL++)
         {
            if (CL->second->checkLayer(GRC_LAY_DEF))
               tllull->add(DEBUG_NEW telldata::TtString(CL->first));
         }
      }
      delete cll;
      LogFile << LogFile.getFN() << "();"; LogFile.flush();
   }
   OPstack.push(tllull);
   DATC->unlockTDT(dbLibDir, true);
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::grcGETLAYERS::grcGETLAYERS(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::ArgumentLIST,retype, eor)
{
}

int tellstdfunc::grcGETLAYERS::execute()
{
   telldata::TtList* tllull = DEBUG_NEW telldata::TtList(telldata::tn_layer);
   laydata::TdtLibDir* dbLibDir = NULL;
   if (DATC->lockTDT(dbLibDir, dbmxs_celllock))
   {
      LayerDefSet grcLays;
      laydata::TdtCell*   tCell   = (*dbLibDir)()->targetECell();
      auxdata::GrcCell* grcCell   = tCell->getGrcCell();
      if (NULL != grcCell)
      {
         grcCell->reportLayers(grcLays);
         for (LayerDefSet::const_iterator CL = grcLays.begin(); CL != grcLays.end(); CL++)
            tllull->add(DEBUG_NEW telldata::TtLayer(*CL));
      }
      LogFile << LogFile.getFN() << "();"; LogFile.flush();
   }
   OPstack.push(tllull);
   DATC->unlockTDT(dbLibDir, true);
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::grcGETDATA::grcGETDATA(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::ArgumentLIST,retype, eor)
{
   _arguments->push_back(DEBUG_NEW ArgumentTYPE("", DEBUG_NEW telldata::TtLayer()));
}

int tellstdfunc::grcGETDATA::execute()
{
//   word     la = getWordValue();
   telldata::TtLayer* tlay = static_cast<telldata::TtLayer*>(OPstack.top());OPstack.pop();
   telldata::TtList* llist = DEBUG_NEW telldata::TtList(telldata::tn_auxilary);
   laydata::TdtLibDir* dbLibDir = NULL;
   if (DATC->lockTDT(dbLibDir, dbmxs_celllock))
   {
      auxdata::AuxDataList dataList;
      laydata::TdtCell*   tCell   = (*dbLibDir)()->targetECell();
      auxdata::GrcCell* grcCell   = tCell->getGrcCell();
      if (NULL != grcCell)
      {
         grcCell->reportLayData(tlay->value(),dataList);
         for (auxdata::AuxDataList::const_iterator CD = dataList.begin(); CD != dataList.end(); CD++)
            llist->add(DEBUG_NEW telldata::TtAuxdata(*CD, tlay->value()));
      }
      LogFile << LogFile.getFN() << "();"; LogFile.flush();
   }
   OPstack.push(llist);
   DATC->unlockTDT(dbLibDir, true);
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::grcCLEANALAYER::grcCLEANALAYER(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::ArgumentLIST,retype, eor)
{
   _arguments->push_back(DEBUG_NEW ArgumentTYPE("", DEBUG_NEW telldata::TtLayer()));
}

void tellstdfunc::grcCLEANALAYER::undo_cleanup()
{
   telldata::TtList* grcShapes = TELL_UNDOOPS_CLEAN(telldata::TtList*);
   clean_ttlaylist(grcShapes);
   delete grcShapes;
}

void tellstdfunc::grcCLEANALAYER::undo()
{
   telldata::TtList* grcTtShapes = TELL_UNDOOPS_UNDO(telldata::TtList*);
   LayerDef grcLayer(TLL_LAY_DEF);
   auxdata::AuxDataList* grcShapes = get_auxdatalist(grcTtShapes, grcLayer);

   laydata::TdtLibDir* dbLibDir = NULL;
   if (DATC->lockTDT(dbLibDir, dbmxs_celllock))
   {
      laydata::TdtDesign* tDesign = (*dbLibDir)();
      laydata::TdtCell*   tCell   = tDesign->targetECell();
      auxdata::GrcCell* grcCell   = tCell->getGrcCell();
      DBbox oldOverlap(tCell->cellOverlap());
      // first restore the grc data
      bool newGrcCellRequired = (NULL == grcCell);
      if (newGrcCellRequired)
      {
         grcCell = DEBUG_NEW auxdata::GrcCell(tCell->name());
      }
      auxdata::QTreeTmpGrc* errlay = grcCell->secureUnsortedLayer(grcLayer);
      for (auxdata::AuxDataList::const_iterator CS = grcShapes->begin(); CS != grcShapes->end(); CS++)
         errlay->put(*CS);
      bool emptyCell = grcCell->fixUnsorted();
      assert(!emptyCell);
      if (newGrcCellRequired)
         tCell->addAuxRef(grcCell);

      tDesign->fixReferenceOverlap(oldOverlap, tCell);
      TpdPost::treeMarkGrcMember(tDesign->activeCellName().c_str(), true);
   }
   DATC->unlockTDT(dbLibDir, true);
   delete grcShapes;
   delete grcTtShapes;
   RefreshGL();
}

int tellstdfunc::grcCLEANALAYER::execute()
{
   telldata::TtLayer* tlay = static_cast<telldata::TtLayer*>(OPstack.top());OPstack.pop();
//   word     la = getWordValue();
   laydata::TdtLibDir* dbLibDir = NULL;
   if (DATC->lockTDT(dbLibDir, dbmxs_celllock))
   {
      laydata::TdtDesign* tDesign = (*dbLibDir)();
      laydata::TdtCell*   tCell   = tDesign->targetECell();
      auxdata::GrcCell* grcCell   = tCell->getGrcCell();
      if (NULL != grcCell)
      {
         DBbox oldOverlap(tCell->cellOverlap());
         auxdata::AuxDataList grcShapes;
         char cellState = grcCell->cleanLay(tlay->value(), grcShapes);
         if (0 <= cellState)
         {
            UNDOcmdQ.push_front(this);
            UNDOPstack.push_front(make_ttlaylist(grcShapes, tlay->value()));
            if (0 == cellState)
            {// grc cell is empty - clear it up
               tCell->clearGrcCell();
               TpdPost::treeMarkGrcMember(tDesign->activeCellName().c_str(), false);
            }
            else if (1 == cellState)
            {
               // grc cell still contains objects and the overlap has changed.
               // update the reference overlaps.
               tDesign->fixReferenceOverlap(oldOverlap, tCell);
            }
            LogFile << LogFile.getFN() << "();"; LogFile.flush();
         }
         else
         {
            std::stringstream ost;
            ost << "No invalid data on layer " << tlay->value();
            tell_log(console::MT_WARNING, ost.str());
         }
      }
      else
         tell_log(console::MT_WARNING,"No invalid data in the current cell.");
   }
   DATC->unlockTDT(dbLibDir, true);
   RefreshGL();
   return EXEC_NEXT;
}


//=============================================================================
tellstdfunc::grcREPAIRDATA::grcREPAIRDATA(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::ArgumentLIST,retype,eor)
{
   _arguments->push_back(DEBUG_NEW ArgumentTYPE("", DEBUG_NEW telldata::TtLayer()));
}

void tellstdfunc::grcREPAIRDATA::undo_cleanup()
{
   telldata::TtList* newShapes = TELL_UNDOOPS_CLEAN(telldata::TtList*);
   telldata::TtList* oldShapes = TELL_UNDOOPS_CLEAN(telldata::TtList*);
   clean_ttlaylist(oldShapes);
   delete newShapes;
   delete oldShapes;
}

void tellstdfunc::grcREPAIRDATA::undo()
{
   telldata::TtList* oldShapes = TELL_UNDOOPS_UNDO(telldata::TtList*);
   telldata::TtList* newShapes = TELL_UNDOOPS_UNDO(telldata::TtList*);
   LayerDef grcLayer(TLL_LAY_DEF);
   auxdata::AuxDataList* grcShapes = get_auxdatalist(oldShapes, grcLayer);
   laydata::AtticList*   tdtlayers = get_shlaylist(newShapes);

   laydata::TdtLibDir* dbLibDir = NULL;
   if (DATC->lockTDT(dbLibDir, dbmxs_celllock))
   {
      laydata::TdtDesign* tDesign = (*dbLibDir)();
      laydata::TdtCell*   tCell   = tDesign->targetECell();
      auxdata::GrcCell* grcCell   = tCell->getGrcCell();
      DBbox oldOverlap(tCell->cellOverlap());
      // first restore the grc data
      bool newGrcCellRequired = (NULL == grcCell);
      if (newGrcCellRequired)
      {
         grcCell = DEBUG_NEW auxdata::GrcCell(tCell->name());
      }
      auxdata::QTreeTmpGrc* errlay = grcCell->secureUnsortedLayer(grcLayer);
      for (auxdata::AuxDataList::const_iterator CS = grcShapes->begin(); CS != grcShapes->end(); CS++)
         errlay->put(*CS);
      bool emptyCell = grcCell->fixUnsorted();
      assert(!emptyCell);
      if (newGrcCellRequired)
         tCell->addAuxRef(grcCell);
      // now remove the recovered data
      assert(1 == tdtlayers->size()); // single layer expected only
      assert(grcLayer == tdtlayers->begin()()); // make sure we have the same layer
      laydata::ShapeList* tdtShapes = *(tdtlayers->begin());
      for (laydata::ShapeList::const_iterator CS = tdtShapes->begin(); CS != tdtShapes->end(); CS++)
         tDesign->destroyThis(*CS, grcLayer, dbLibDir);
      delete tdtShapes;

      tDesign->fixReferenceOverlap(oldOverlap, tCell);
      TpdPost::treeMarkGrcMember(tDesign->activeCellName().c_str(), true);
   }
   DATC->unlockTDT(dbLibDir, true);
   delete tdtlayers;
   delete grcShapes;
   delete newShapes;
   delete oldShapes;
   RefreshGL();
}

int tellstdfunc::grcREPAIRDATA::execute()
{
//   word     la = getWordValue();
   telldata::TtLayer* tlay = static_cast<telldata::TtLayer*>(OPstack.top());OPstack.pop();

   laydata::TdtLibDir* dbLibDir = NULL;
   if (DATC->lockTDT(dbLibDir, dbmxs_celllock))
   {
      laydata::TdtDesign* tDesign = (*dbLibDir)();
      laydata::TdtCell*   tCell   = tDesign->targetECell();
      auxdata::GrcCell* grcCell   = tCell->getGrcCell();
      if (NULL != grcCell)
      {
         laydata::ShapeList newShapes;
         if (grcCell->repairData(tlay->value(), newShapes))
         {
            if (!newShapes.empty())
            {
               UNDOcmdQ.push_front(this);
               DBbox oldOverlap(tCell->cellOverlap());
               tDesign->addList(tlay->value(), newShapes);
               UNDOPstack.push_front(make_ttlaylist(newShapes, tlay->value()));
               auxdata::AuxDataList oldShapes;
               switch (grcCell->cleanRepaired(tlay->value(), oldShapes))
               {
                  case -1: // grc cell is empty - clear it up
                           tCell->clearGrcCell();
                           TpdPost::treeMarkGrcMember(tDesign->activeCellName().c_str(), false);
                           // Theoretically, the reference overlap of the tdt cell is supposed to
                           // remain the same (objects from grc moved to tdt)
                           //tDesign->fixReferenceOverlap(oldOverlap, tCell);
                           break;
                  case  0: // cell still contains objects and the overlap is the same. Nothing to do
                           break;
                  case  1: // grc cell still contains objects and the overlap has changed.
                           // update the reference overlaps. The tdt cell overlap should be the
                           // same, but grc overlap did change.
                           tDesign->fixReferenceOverlap(oldOverlap, tCell);
                           break;
                  default: assert(false); break;
               }
               UNDOPstack.push_front(make_ttlaylist(oldShapes, tlay->value()));
            }
            else
            {
               std::stringstream ost;
               ost << "No recoverable data on layer " << tlay->value() << ". Check poly/wire recovery settings.";
               tell_log(console::MT_WARNING, ost.str());
            }
            LogFile << LogFile.getFN() << "(" << *tlay << ");"; LogFile.flush();
         }
         else
         {
            std::stringstream ost;
            ost << "No invalid data on layer " << tlay->value() << ". Nothing to repair.";
            tell_log(console::MT_WARNING, ost.str());
         }
      }
      else
         tell_log(console::MT_WARNING,"No invalid data in the current cell. Nothing to repair.");
   }
   DATC->unlockTDT(dbLibDir, true);
   RefreshGL();
   return EXEC_NEXT;
}
