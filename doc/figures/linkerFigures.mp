filenametemplate "%j-%c.mps";

input boxes
input ModuleView

%========================================

def arrowPath (suffix a, b)(expr linePath)(expr lineColor)(expr isDotted) =
  % draw a single arrow path given by <linePath> between box objects
  % <a> and <b> at their boundaries in color <lineColor> where
  % <isDotted> tells whether the line is dotted or dashed

  path currentPath;

  currentPath := linePath cutbefore bpath.a cutafter bpath.b;

  if isDotted:
    drawarrow currentPath dashed withdots withcolor lineColor
        withpen pencircle scaled 1.5pt;
  else:
    drawarrow currentPath dashed evenly withcolor lineColor
        withpen pencircle scaled 0.75pt;
  fi
enddef;

%========================================

def addUsageBottomToTop (suffix a, b)(expr lineColor)(expr isDotted) =
  % draw a single arrow path between box objects <a> and <b> at their
  % boundaries with the path leaving <a> on bottom and entering <b>
  % from top using color <lineColor> for the line where <isDotted>
  % tells whether the line is dotted or dashed
  
  path currentPath;
  currentPath := a.c{down} .. tension 2 .. b.c{down};
  arrowPath(a, b, currentPath, lineColor, isDotted);
enddef;
  
%========================================

def addUsageToTop (suffix a, b)(expr lineColor)(expr isDotted) =
  % draw a single arrow path between box objects <a> and <b> at their
  % boundaries with the path entering <b> from top using color
  % <lineColor> for the line where <isDotted> tells whether the line
  % is dotted or dashed
  
  path currentPath;
  numeric difference;

  difference := (xpart b.c - xpart a.c);

  if abs(difference) < 5mm:
    currentPath := a.c..b.c;
  else:
    pair direction;

    if difference < 0:
      direction := left;
    else:
      direction := right;
    fi

    currentPath := a.c{direction} .. tension 2 .. b.c{down};
  fi

  arrowPath(a, b, currentPath, lineColor, isDotted);
enddef;

%========================================

def addImplementationUsageToTop (suffix a, b) =
  % draw a single arrow path between box objects <a> and <b> at their
  % boundaries with the path entering <b> from top; path line has
  % <interfaceDependencyColor> because implementation of <a> uses <b>

  addUsageToTop(a, b, black, true);
enddef;

%========================================

def addInterfaceUsageToTop (suffix a, b) =
  % draw a single arrow path between box objects <a> and <b> at their
  % boundaries with the path entering <b> from top; path line has
  % <interfaceDependencyColor> because interface of <a> uses <b>

  addUsageToTop(a, b, red, false);
enddef;

%========================================

def addInterfaceUsageFromBottom (suffix a, b) =
  % draw a single arrow path between box objects <a> and <b> at their
  % boundaries with the path leaving <a> at bottom

  path currentPath;
  numeric difference;

  difference := (xpart b.c - xpart a.c);

  if abs(difference) < 5mm:
    currentPath := a.c..b.c;
  else:
    pair direction;

    if difference < 0:
      direction := left;
    else:
      direction := right;
    fi

    currentPath := a.c{down} .. tension 2 .. b.c{direction};
  fi

  arrowPath(a, b, currentPath, red, false);
enddef;

%========================================

def initializeLinkerModulePicture =
  % sets up picture for linker module dependencies
  
  ModuleView_initialize;
  ModuleView_basicTypeLevelY :=  0.0cm;
  ModuleView_linkerModuleLevelY :=  4.0cm;
  ModuleView_defineBoxesBasicTypes;
  ModuleView_defineBoxesLAndPModules; 
enddef;

%========================================

def drawLinkerModulePicture =
  % draws picture for linker module dependencies

  ModuleView_drawBoxesBasicTypes;
  ModuleView_drawBoxesLAndPModules; 
enddef;

%========================================
% usage relations from Main
%========================================

beginfig(1);
  ModuleView_initialize;
  ModuleView_defineBoxes;

  addImplementationUsageToTop(mainModule, areaModule);
  addImplementationUsageToTop(mainModule, bankingModule);
  addImplementationUsageToTop(mainModule, codeoutputModule);
  addImplementationUsageToTop(mainModule, errorModule);
  addImplementationUsageToTop(mainModule, fileModule);
  addImplementationUsageToTop(mainModule, globdefsModule);
  addImplementationUsageToTop(mainModule, libraryModule);
  addImplementationUsageToTop(mainModule, listModule);
  addImplementationUsageToTop(mainModule, listingupdaterModule);
  addImplementationUsageToTop(mainModule, mapfileModule);
  addImplementationUsageToTop(mainModule, noicemapfileModule);
  addImplementationUsageToTop(mainModule, parserModule);
  addImplementationUsageToTop(mainModule, stringModule);
  addImplementationUsageToTop(mainModule, stringlistModule);
  addImplementationUsageToTop(mainModule, stringtableModule);
  addImplementationUsageToTop(mainModule, symbolModule);
  addImplementationUsageToTop(mainModule, targetModule);

  ModuleView_drawBoxes;
endfig;

%========================================
% usage relations to GlobDefs
%========================================

beginfig(2);
  ModuleView_initialize;
  ModuleView_defineBoxes;

  addInterfaceUsageFromBottom(errorModule, globdefsModule);
  addInterfaceUsageFromBottom(fileModule, globdefsModule);
  addInterfaceUsageFromBottom(integermapModule, globdefsModule);
  addInterfaceUsageFromBottom(listModule, globdefsModule);
  addInterfaceUsageFromBottom(mapModule, globdefsModule);
  addInterfaceUsageFromBottom(multimapModule, globdefsModule);
  addInterfaceUsageFromBottom(setModule, globdefsModule);
  addInterfaceUsageFromBottom(stringModule, globdefsModule);
  addInterfaceUsageFromBottom(stringlistModule, globdefsModule);
  addInterfaceUsageFromBottom(typedescriptorModule, globdefsModule);

  addInterfaceUsageFromBottom(areaModule, globdefsModule);
  addInterfaceUsageFromBottom(bankingModule, globdefsModule);
  addInterfaceUsageFromBottom(codeoutputModule, globdefsModule);
  addInterfaceUsageFromBottom(codesequenceModule, globdefsModule);
  addInterfaceUsageFromBottom(libraryModule, globdefsModule);
  addInterfaceUsageFromBottom(listingupdaterModule, globdefsModule);
  addInterfaceUsageFromBottom(mapfileModule, globdefsModule);
  addInterfaceUsageFromBottom(moduleModule, globdefsModule);
  addInterfaceUsageFromBottom(noicemapfileModule, globdefsModule);
  addInterfaceUsageFromBottom(parserModule, globdefsModule);
  addInterfaceUsageFromBottom(scannerModule, globdefsModule);
  addInterfaceUsageFromBottom(stringtableModule, globdefsModule);
  addInterfaceUsageFromBottom(symbolModule, globdefsModule);
  addInterfaceUsageFromBottom(targetModule, globdefsModule);
  addInterfaceUsageFromBottom(gameboyModule, globdefsModule);
  addInterfaceUsageFromBottom(otherPlatformModule, globdefsModule);

  addImplementationUsageToTop(mainModule, globdefsModule);

  ModuleView_drawBoxes;
endfig;


%========================================
% usage relations in basic type modules
%========================================

beginfig(3);
  ModuleView_initialize;
  ModuleView_basicTypeLevelY :=  0.0cm;
  ModuleView_ySpace          :=  20mm;
  ModuleView_defineBoxesBasicTypes;

  % error
  addInterfaceUsageToTop(errorModule, fileModule);

  % file
  addInterfaceUsageToTop(fileModule, stringModule);

  % integermap
  addInterfaceUsageToTop(integermapModule, mapModule);

  % list
  addInterfaceUsageToTop(listModule, typedescriptorModule);
  
  % map
  addInterfaceUsageToTop(mapModule, listModule);
  addInterfaceUsageToTop(mapModule, typedescriptorModule);

  % multimap
  addInterfaceUsageToTop(multimapModule, listModule);
  addInterfaceUsageToTop(multimapModule, mapModule);
  addInterfaceUsageToTop(multimapModule, typedescriptorModule);

  % set
    % none

  % string
  addImplementationUsageToTop(stringModule, errorModule);
  addInterfaceUsageToTop(stringModule, typedescriptorModule);

  % stringlist
  addUsageBottomToTop(stringlistModule, fileModule, red, false);
  addUsageBottomToTop(stringlistModule, listModule, red, false);
  addInterfaceUsageToTop(stringlistModule, stringModule);

  % typedescriptor
    % none

  ModuleView_drawBoxesBasicTypes;
endfig;

%========================================
% usage relations in linker modules
%========================================

% -- area --
beginfig(4);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(areaModule, listModule);
  addInterfaceUsageToTop(areaModule, moduleModule);
  addInterfaceUsageToTop(areaModule, setModule);
  addInterfaceUsageToTop(areaModule, stringModule);
  addInterfaceUsageToTop(areaModule, symbolModule);
  addInterfaceUsageToTop(areaModule, targetModule);
  addInterfaceUsageToTop(areaModule, typedescriptorModule);

  addImplementationUsageToTop(areaModule, errorModule);

  drawLinkerModulePicture;
endfig;

% -- bankingModule --
beginfig(5);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(bankingModule, moduleModule);
  addInterfaceUsageToTop(bankingModule, stringModule);
  addInterfaceUsageToTop(bankingModule, stringlistModule);
  addInterfaceUsageToTop(bankingModule, targetModule);

  addImplementationUsageToTop(bankingModule, areaModule);
  addImplementationUsageToTop(bankingModule, errorModule);
  addImplementationUsageToTop(bankingModule, fileModule);
  addImplementationUsageToTop(bankingModule, integermapModule);
  addImplementationUsageToTop(bankingModule, listModule);
  addImplementationUsageToTop(bankingModule, mapModule);
  addImplementationUsageToTop(bankingModule, parserModule);
  addImplementationUsageToTop(bankingModule, symbolModule);
  addImplementationUsageToTop(bankingModule, typedescriptorModule);

  drawLinkerModulePicture;
endfig;

% -- codeoutputModule --
beginfig(6);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(codeoutputModule, codesequenceModule);
  addInterfaceUsageToTop(codeoutputModule, fileModule);
  addInterfaceUsageToTop(codeoutputModule, stringModule);
  addInterfaceUsageToTop(codeoutputModule, stringlistModule);

  addImplementationUsageToTop(codeoutputModule, errorModule);
  addImplementationUsageToTop(codeoutputModule, listModule);

  drawLinkerModulePicture;
endfig;

% -- codesequenceModule --
beginfig(7);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(codesequenceModule, areaModule);
  addInterfaceUsageToTop(codesequenceModule, targetModule);

  addImplementationUsageToTop(codesequenceModule, errorModule);
  addImplementationUsageToTop(codesequenceModule, mapfileModule);
  addImplementationUsageToTop(codesequenceModule, moduleModule);
  addImplementationUsageToTop(codesequenceModule, stringModule);
  addImplementationUsageToTop(codesequenceModule, symbolModule);

  drawLinkerModulePicture;
endfig;

% -- libraryModule --
beginfig(8);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(libraryModule, stringModule);
  addInterfaceUsageToTop(libraryModule, stringlistModule);

  addImplementationUsageToTop(libraryModule, errorModule);
  addImplementationUsageToTop(libraryModule, fileModule);
  addImplementationUsageToTop(libraryModule, listModule);
  addImplementationUsageToTop(libraryModule, multimapModule);
  addImplementationUsageToTop(libraryModule, parserModule);
  addImplementationUsageToTop(libraryModule, symbolModule);
  addImplementationUsageToTop(libraryModule, typedescriptorModule);
  
  drawLinkerModulePicture;
endfig;

% -- listingupdaterModule --
beginfig(9);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(listingupdaterModule, stringlistModule);

  addImplementationUsageToTop(listingupdaterModule, areaModule);
  addImplementationUsageToTop(listingupdaterModule, bankingModule);
  addImplementationUsageToTop(listingupdaterModule, errorModule);
  addImplementationUsageToTop(listingupdaterModule, fileModule);
  addImplementationUsageToTop(listingupdaterModule, integermapModule);
  addImplementationUsageToTop(listingupdaterModule, listModule);
  addImplementationUsageToTop(listingupdaterModule, mapModule);
  addImplementationUsageToTop(listingupdaterModule, moduleModule);
  addImplementationUsageToTop(listingupdaterModule, scannerModule);
  addImplementationUsageToTop(listingupdaterModule, stringModule);
  addImplementationUsageToTop(listingupdaterModule, targetModule);
  
  drawLinkerModulePicture;
endfig;

% -- mapfileModule --
beginfig(10);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(mapfileModule, areaModule);
  addInterfaceUsageToTop(mapfileModule, fileModule);
  addInterfaceUsageToTop(mapfileModule, stringModule);
  addInterfaceUsageToTop(mapfileModule, stringlistModule);
  addInterfaceUsageToTop(mapfileModule, symbolModule);

  addImplementationUsageToTop(mapfileModule, errorModule);
  addImplementationUsageToTop(mapfileModule, libraryModule);
  addImplementationUsageToTop(mapfileModule, listModule);
  addImplementationUsageToTop(mapfileModule, moduleModule);
  addImplementationUsageToTop(mapfileModule, multimapModule);
  addImplementationUsageToTop(mapfileModule, setModule);
  addImplementationUsageToTop(mapfileModule, stringtableModule);
  addImplementationUsageToTop(mapfileModule, targetModule);

  drawLinkerModulePicture;
endfig;

% -- moduleModule --
beginfig(11);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(moduleModule, areaModule);
  addInterfaceUsageToTop(moduleModule, listModule);
  addInterfaceUsageToTop(moduleModule, stringModule);
  addInterfaceUsageToTop(moduleModule, symbolModule);
  addInterfaceUsageToTop(moduleModule, typedescriptorModule);

  addImplementationUsageToTop(moduleModule, errorModule);
  
  drawLinkerModulePicture;
endfig;

% -- noicemapfileModule --
beginfig(12);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(noicemapfileModule, fileModule);
  addInterfaceUsageToTop(noicemapfileModule, stringModule);

  addImplementationUsageToTop(noicemapfileModule, areaModule);
  addImplementationUsageToTop(noicemapfileModule, errorModule);
  addImplementationUsageToTop(noicemapfileModule, listModule);
  addImplementationUsageToTop(noicemapfileModule, mapfileModule);
  addImplementationUsageToTop(noicemapfileModule, symbolModule);
  addImplementationUsageToTop(noicemapfileModule, targetModule);
  
  drawLinkerModulePicture;
endfig;

% -- parserModule --
beginfig(13);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(parserModule, stringModule);
  addInterfaceUsageToTop(parserModule, stringlistModule);

  addImplementationUsageToTop(parserModule, areaModule);
  addImplementationUsageToTop(parserModule, bankingModule);
  addImplementationUsageToTop(parserModule, codeoutputModule);
  addImplementationUsageToTop(parserModule, codesequenceModule);
  addImplementationUsageToTop(parserModule, errorModule);
  addImplementationUsageToTop(parserModule, fileModule);
  addImplementationUsageToTop(parserModule, listModule);
  addImplementationUsageToTop(parserModule, mapfileModule);
  addImplementationUsageToTop(parserModule, moduleModule);
  addImplementationUsageToTop(parserModule, scannerModule);
  addImplementationUsageToTop(parserModule, setModule);
  addImplementationUsageToTop(parserModule, symbolModule);
  addImplementationUsageToTop(parserModule, targetModule);
  
  drawLinkerModulePicture;
endfig;

% -- scannerModule --
beginfig(14);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(scannerModule, listModule);
  addInterfaceUsageToTop(scannerModule, stringModule);

  addImplementationUsageToTop(scannerModule, errorModule);
  addImplementationUsageToTop(scannerModule, fileModule);
  addImplementationUsageToTop(scannerModule, typedescriptorModule);
  
  drawLinkerModulePicture;
endfig;

% -- stringtableModule --
beginfig(15);
  initializeLinkerModulePicture;
  addInterfaceUsageToTop(stringtableModule, stringlistModule);
  addImplementationUsageToTop(stringtableModule, listModule);
  addImplementationUsageToTop(stringtableModule, stringModule);
  drawLinkerModulePicture;
endfig;

% -- symbolModule --
beginfig(16);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(symbolModule, areaModule);
  addInterfaceUsageToTop(symbolModule, fileModule);
  addInterfaceUsageToTop(symbolModule, listModule);
  addInterfaceUsageToTop(symbolModule, stringModule);
  addInterfaceUsageToTop(symbolModule, targetModule);
  addInterfaceUsageToTop(symbolModule, typedescriptorModule);

  addImplementationUsageToTop(symbolModule, errorModule);
  addImplementationUsageToTop(symbolModule, mapModule);
  addImplementationUsageToTop(symbolModule, moduleModule);
  addImplementationUsageToTop(symbolModule, setModule);
  
  drawLinkerModulePicture;
endfig;

% -- targetModule --
beginfig(17);
  initializeLinkerModulePicture;
  addInterfaceUsageToTop(targetModule, bankingModule);
  addInterfaceUsageToTop(targetModule, stringModule);
  addInterfaceUsageToTop(targetModule, stringlistModule);
  addImplementationUsageToTop(targetModule, gameboyModule);
  drawLinkerModulePicture;
endfig;

% -- gameboyModule --
beginfig(18);
  initializeLinkerModulePicture;

  addInterfaceUsageToTop(gameboyModule, targetModule);

  addImplementationUsageToTop(gameboyModule, areaModule);
  addImplementationUsageToTop(gameboyModule, bankingModule);
  addImplementationUsageToTop(gameboyModule, codeoutputModule);
  addImplementationUsageToTop(gameboyModule, codesequenceModule);
  addImplementationUsageToTop(gameboyModule, errorModule);
  addImplementationUsageToTop(gameboyModule, fileModule);
  addImplementationUsageToTop(gameboyModule, listModule);
  addImplementationUsageToTop(gameboyModule, mapfileModule);
  addImplementationUsageToTop(gameboyModule, stringModule);
  addImplementationUsageToTop(gameboyModule, stringlistModule);
  addImplementationUsageToTop(gameboyModule, stringtableModule);
  addImplementationUsageToTop(gameboyModule, symbolModule);
  addImplementationUsageToTop(gameboyModule, typedescriptorModule);
  
  drawLinkerModulePicture;
endfig;

bye
