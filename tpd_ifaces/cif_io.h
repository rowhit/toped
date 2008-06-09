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
#if !defined(CIFIO_H_INCLUDED)
#define CIFIO_H_INCLUDED

#include <string>

#define YYLTYPE CIFin::yyltype

#define YYLLOC_DEFAULT(Current, Rhs, N)                         \
   do                                                           \
   if (N)                                                       \
   {                                                            \
      (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;    \
      (Current).first_column = YYRHSLOC (Rhs, 1).first_column;  \
      (Current).last_line    = YYRHSLOC (Rhs, N).last_line;     \
      (Current).last_column  = YYRHSLOC (Rhs, N).last_column;   \
      (Current).filename     = YYRHSLOC (Rhs, N).filename;      \
   }                                                            \
      else                                                      \
   {                                                            \
      (Current).first_line   = (Current).last_line   =          \
                           YYRHSLOC (Rhs, 0).last_line;         \
      (Current).first_column = (Current).last_column =          \
                        YYRHSLOC (Rhs, 0).last_column;          \
         (Current).filename = YYRHSLOC (Rhs, 0).filename;       \
   }                                                            \
   while (0)

int ciflex(void);
int ciferror (char *s);

namespace CIFin {

/*
A formal CIF 2.0 definition in Wirth notation is given below. Taken from 
http://ai.eecs.umich.edu/people/conway/VLSI/ImplGuide/ImplGuide.pdf
"A guide to LSI Implementation", Second Edition, Robert W. Hon Carlo H. Sequin

cifFile              = {{blank} [command] semi} endCommand {blank}.

command              = primCommand | defDeleteCommand | 
                       defStartCommand semi {{blank}[primCommand]semi} defFinishCommand.

primCommand          = polygonCommand       | boxCommand    | roundFlashCommand |
                       wireCommand          | layerCommand  | callCommand       | 
                       userExtensionCommand | commentCommand

polygonCommand       = "P" path.
boxCommand           = "B" <*{blank}*> integer sep integer sep point [sep point]
roundFlashCommand    = "R" integer sep point.
wireCommand          = "W" integer sep path.
layerCommand         = "L" {blank} shortname.
defStartCommand      = "D" {blank} "S" integer [sep integer sep integer].
defFinishCommand     = "D" {blank} "F".
defDeleteCommand     = "D" {blank} "D" integer.
callCommand          = "C" <*{blank}*> integer transformation.
userExtensionCommand = digit userText
commentCommand       = "(" commentText ")".
endCommand           = "E".

transformation       = {{blank} ("T" point | "M" {blank} "X" | "M" {blank} "Y" | "R" point)}.
path                 = point {sep point}.
point                = sInteger sep sInteger.
sInteger             = {sep} ["-"]integerD.
integer              = {sep} integerD.
integerD             = digit{digit}.
shortname            = c[c][c][c]
c                    = digit | upperChar
userText             = {userChar}.
commentText          = {commentChar} | commentText "(" commentText ")" commentText.

semi                 = {blank};{blank}.
sep                  = upperChar | blank;
digit                = "0" | "1" | "2" | ... | "9"
upperChar            = "A" | "B" | "C" | ... | "Z"
blank                = any ASCII character except digit, upperChar, "-" , "(" , ")" , ";".
userChar             = any ASCII character except ";".
commentChar          = any ASCII character except "(" or ")".

<* ... *> -> included in the parser (or the scanner) because existing CIF files are using this syntax, but
             it breaks the formal syntax definition of CIF

*/

   class   CIFFile {
   public:
                     CIFFile(std::string);
                    ~CIFFile();
      bool           status() {return _status;}
   protected:
      bool           _status;
   };

   struct yyltype {
      int          first_line;
      int          first_column;
      int          last_line;
      int          last_column;
      char*        filename;
   };
}

#endif // !defined(CIFIO_H_INCLUDED)