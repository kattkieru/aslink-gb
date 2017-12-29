% ModuleView component
%
% makes a component view of all linker modules

numeric ModuleView_mainLevelY, ModuleView_linkerModuleLevelY;
numeric ModuleView_basicTypeLevelY, ModuleView_baseLevelY;
numeric ModuleView_ySpace;

numeric ModuleView__xIndentation, ModuleView__xSpace;
numeric ModuleView__leftMarginX, ModuleView__rightMarginX;
numeric ModuleView__globalBoxHeight;
color ModuleView__fillColor_main;
color ModuleView__fillColor_link;
color ModuleView__fillColor_basic;
color ModuleView__fillColor_globdefs;

%====================
% INTERNAL MACROS
%====================

def ModuleView__fixBoxes (text suffixes) =
  % fix size and position of all boxes given by <suffixes>

  forsuffixes myBox = suffixes:
    fixsize(myBox);
    fixpos(myBox);
  endfor;
enddef;

%--------------------

def ModuleView__initializeBoxColors =
  % initializes colors of the layers

  ModuleView__fillColor_main      := (0.8, 0.8,   1);
  ModuleView__fillColor_link      := (0.8,   1,   1);
  ModuleView__fillColor_basic     := (  1,   1, 0.8);
  ModuleView__fillColor_globdefs  := (0.7,   1, 0.7);
enddef;

%--------------------

def ModuleView__joinBoxes (expr xSpace) =
  % joins boxes horizontally with <xSpace> as spacing; also ensures
  % that internal dx space of box is identical for all subsequent box
  % definitions

  boxjoin(b.w = a.e + (xSpace,0); b.dx = a.dx);
enddef;

%--------------------

def ModuleView__normalizeHeight (expr height) text suffixes =
  % for all boxes given by <suffixes> set height to <height>

  forsuffixes myBox = suffixes:  
    ypart myBox.ne = ypart myBox.se + height;
  endfor;
enddef;

%--------------------

def ModuleView__fillBoxesAndDraw (expr fillColor) text suffixes =
  % fix, fill and draw boxes given by <suffixes> with fill color set
  % to <fillColor>

  forsuffixes myBox = suffixes:
    fill bpath.myBox withcolor fillColor;
    drawboxed(myBox);
  endfor;
enddef;

%--------------------

def ModuleView__setupBasicTypeLevel (expr height, leftX, rightX, centerY,
                                         xSpace, deltaY, xIndentation) =
  % defines and positions boxes for basic types

  % --- first row ---
  ModuleView__joinBoxes(xSpace);
  boxit.multimapModule(btex MultiMap etex);
  boxit.mapModule(btex Map etex);
  boxit.stringlistModule(btex StringList etex);
  boxit.integermapModule(btex IntegerMap etex);
  boxit.setModule(btex Set etex);

  multimapModule.w = (leftX, centerY);
  xpart setModule.e = rightX;
  
  % --- second row ---
  ModuleView__joinBoxes(xSpace);
  boxit.listModule(btex List etex);
  boxit.typedescriptorModule(btex TypeDescriptor etex);
  boxit.stringModule(btex String etex);
  boxit.errorModule(btex Error etex);
  boxit.fileModule(btex File etex);
  
  listModule.w = multimapModule.w + (xIndentation, -deltaY);
  xpart fileModule.e = xpart setModule.e - xIndentation;

  % -- set same height for all module boxes --
  ModuleView__normalizeHeight(height)
      errorModule, fileModule, integermapModule, listModule,
      mapModule, multimapModule, setModule, stringModule,
      stringlistModule, typedescriptorModule;
enddef;

%--------------------

def ModuleView__setupLinkerModuleLevel (expr height, leftX, rightX, centerY,
                                            xSpace, deltaY, xIndentation) =
  % defines and positions boxes for linker modules

  % --- first row ---
  ModuleView__joinBoxes(xSpace);
  boxit.areaModule(btex Area etex);
  boxit.bankingModule(btex Banking etex);
  boxit.codeoutputModule(btex CodeOutput etex);
  boxit.codesequenceModule(btex CodeSequence etex);
  boxit.libraryModule(btex Library etex);
  boxit.listingupdaterModule(btex ListingUpdater etex);
  boxit.mapfileModule(btex MapFile etex);

  areaModule.w = (leftX, centerY);
  xpart mapfileModule.e = rightX;

  % --- second row ---
  ModuleView__joinBoxes(xSpace);
  boxit.moduleModule(btex Module etex);
  boxit.noicemapfileModule(btex NoICEMapFile etex);
  boxit.parserModule(btex Parser etex);
  boxit.scannerModule(btex Scanner etex);
  boxit.stringtableModule(btex StringTable etex);
  boxit.symbolModule(btex Symbol etex);

  moduleModule.w = areaModule.w + (xIndentation, -deltaY);

  % -- set same height for all module boxes --
  ModuleView__normalizeHeight(height)
      areaModule, bankingModule, codeoutputModule,
      codesequenceModule,libraryModule, listingupdaterModule,
      mapfileModule, moduleModule, noicemapfileModule, parserModule,
      scannerModule, stringtableModule, symbolModule;
enddef;

%--------------------

def ModuleView__setupPlatformModuleLevel(expr height, leftX, rightX, centerY,
                                             xSpace, deltaY, xIndentation) =
  % defines and positions boxes for platform modules

  % --- first row ---
  boxjoin();
  boxit.targetModule(btex Target etex);
  targetModule.w = (leftX,centerY);
  xpart targetModule.e = rightX;

  ModuleView__joinBoxes(xSpace);
  boxit.gameboyModule(btex Gameboy etex);
  boxit.otherPlatformModule(btex other platform etex);

  gameboyModule.w = targetModule.w + (0,-deltaY);
  xpart otherPlatformModule.e = xpart targetModule.e;

  % -- set same height for all module boxes --
  ModuleView__normalizeHeight(height)
      targetModule, gameboyModule, otherPlatformModule;
enddef;
    
%====================
% EXPORTED MACROS
%====================

def ModuleView_initialize =
  % sets main configuration variables

  ModuleView__leftMarginX              := 0cm;
  ModuleView__rightMarginX             := 22cm;
  ModuleView__linkerModuleRightMarginX := 16cm;

  ModuleView_mainLevelY          := 11.0cm;
  ModuleView_linkerModuleLevelY  :=  8.0cm;
  ModuleView_basicTypeLevelY     :=  4.0cm;
  ModuleView_baseLevelY          :=  0.0cm;

  ModuleView_ySpace               := 9mm;
  ModuleView__xSpace              := 7mm;
  ModuleView__globalBoxHeight     := 6mm;
  ModuleView__deltaY              := ModuleView__globalBoxHeight
                                     + ModuleView_ySpace;
  ModuleView__xIndentation        := 5mm;

  ModuleView__initializeBoxColors;
enddef;

%--------------------
%--------------------

def ModuleView_defineBoxes =
  % sets up all module boxes in component view

  ModuleView_defineBoxesBasicTypes;
  ModuleView_defineBoxesLAndPModules; 
  ModuleView_defineBoxesGlobdefsAndMain;
enddef;

%--------------------

def ModuleView_defineBoxesBasicTypes =
  % initializes basic type module boxes in component view

  ModuleView__deltaY := ModuleView__globalBoxHeight + ModuleView_ySpace;

  ModuleView__setupBasicTypeLevel(ModuleView__globalBoxHeight,
                                  ModuleView__leftMarginX,
				  ModuleView__rightMarginX,
                                  ModuleView_basicTypeLevelY,
                                  4 * ModuleView__xSpace, ModuleView__deltaY,
                                  ModuleView__xIndentation);

  ModuleView__fixBoxes(errorModule, fileModule, integermapModule, listModule,
                       mapModule, multimapModule, setModule, stringModule,
                       stringlistModule, typedescriptorModule);
enddef;
  
%--------------------

def ModuleView_defineBoxesGlobdefsAndMain =
  % initializes base module boxes in component view

  % --- globdefs level ---
  boxjoin();
  boxit.globdefsModule(btex GlobDefs etex);
  globdefsModule.w = (ModuleView__leftMarginX,ModuleView_baseLevelY);
  xpart globdefsModule.e = ModuleView__rightMarginX;

  % --- main level ---
  boxit.mainModule(btex Main etex);

  mainModule.w = (ModuleView__leftMarginX,ModuleView_mainLevelY);
  xpart mainModule.e = ModuleView__rightMarginX;

  % -- set same height for all module boxes --
  ModuleView__normalizeHeight(ModuleView__globalBoxHeight)
      globdefsModule, mainModule;

  % --- fix all boxes now ---
  ModuleView__fixBoxes(mainModule, globdefsModule);
enddef;
  
%--------------------

def ModuleView_defineBoxesLAndPModules =
  % initializes linker module and platform boxes in component view

  numeric leftMarginX;

  ModuleView__setupLinkerModuleLevel(ModuleView__globalBoxHeight,
                                     ModuleView__leftMarginX,
				     ModuleView__linkerModuleRightMarginX,
                                     ModuleView_linkerModuleLevelY,
                                     ModuleView__xSpace, ModuleView__deltaY,
                                     ModuleView__xIndentation);

  leftMarginX = xpart mapfileModule.e + 3/2 * ModuleView__xSpace;
			       
  ModuleView__setupPlatformModuleLevel(ModuleView__globalBoxHeight,
                                       leftMarginX,
			 	       ModuleView__rightMarginX,
                                       ModuleView_linkerModuleLevelY,
                                       ModuleView__xSpace, ModuleView__deltaY,
                                       ModuleView__xIndentation);

  ModuleView__fixBoxes(areaModule, bankingModule, codeoutputModule,
                       codesequenceModule,libraryModule, listingupdaterModule,
                       mapfileModule, moduleModule, noicemapfileModule,
		       parserModule, scannerModule, stringtableModule,
		       symbolModule, targetModule, gameboyModule,
		       otherPlatformModule);
enddef;
  
%--------------------

def ModuleView_drawBoxes =
  % draws module boxes in current drawing

  %-- fill boxes and draw them --
  ModuleView__fillBoxesAndDraw(ModuleView__fillColor_main) mainModule;
  ModuleView_drawBoxesLAndPModules; 
  ModuleView_drawBoxesBasicTypes;
  ModuleView__fillBoxesAndDraw(ModuleView__fillColor_globdefs) globdefsModule;
enddef;

%--------------------

def ModuleView_drawBoxesBasicTypes =
  % draws module boxes of basic types in current drawing

  ModuleView__fillBoxesAndDraw(ModuleView__fillColor_basic)
      errorModule, fileModule, integermapModule, listModule,
      mapModule, multimapModule, setModule, stringModule,
      stringlistModule, typedescriptorModule;
enddef;

%--------------------

def ModuleView_drawBoxesLAndPModules =
  % draws boxes of linker modules in current drawing

  ModuleView__fillBoxesAndDraw(ModuleView__fillColor_link)
      areaModule, bankingModule, codeoutputModule,
      codesequenceModule,libraryModule, listingupdaterModule,
      mapfileModule, moduleModule, noicemapfileModule, parserModule,
      scannerModule, stringtableModule, symbolModule,
      targetModule, gameboyModule, otherPlatformModule;
enddef;
