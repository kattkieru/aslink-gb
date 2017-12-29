# cIncludeToLatex -- script to convert C include files to LaTeX
#

BEGIN \
{
  false = (1==0);  true = (1==1);

  documentationLeadIn = "\\begin{documentation}";
  documentationLeadOut = "\\end{documentation}\n";
  headerLeadIn = "\\begin{moduleHeader}";
  headerLeadOut = "\\end{moduleHeader}\n";
  importLeadIn = "\\begin{lstlisting}";
  importLeadOut = "\\end{lstlisting}";
  macroLeadIn = "\\begin{lstlisting}";
  macroLeadOut = "\\end{lstlisting}";
  routineLeadIn = "\\begin{lstlisting}";
  routineLeadOut = "\\end{lstlisting}";
  typeLeadIn = "\\begin{lstlisting}";
  typeLeadOut = "\\end{lstlisting}";
  varLeadIn = "\\begin{lstlisting}";
  varLeadOut = "\\end{lstlisting}";

  ParseState_atBegin = 0;
  ParseState_inWhiteSpace = 1;
  ParseState_inContinuation = 2;
  ParseState_inHeader = 3;
  ParseState_inImportDefinition = 4;
  ParseState_inMacroDefinition = 5;
  ParseState_inVarDefinition = 6;
  ParseState_inTypeDefinition = 7;
  ParseState_inRoutineDefinition = 8;
  ParseState_inDocumentation = 9;

  StateStack_initialize();
  StateStack_push(ParseState_atBegin);
}

#--------------------

{
  thisLine = $0;

  # get rid of some special lines
  if (thisLine ~ /Original version/ || thisLine ~ /^[ \t]*\/\//) {
    next;
  }

  if (thisLine ~ /module --/) {
    # lead in of module header ==> put out section name
    moduleName = thisLine;
    sub(/^ *\/\*\* */, "", moduleName);
    sub(/ +module --/, "", moduleName);
    print "\\moduleStart{" moduleName "}";
  }

  lineHasBeenPrinted = false;
  isLastLine = false;
}

#--------------------
# PREPROCESSOR STUFF
#--------------------

$0 !~ /\#include/ && parseState == ParseState_inImportDefinition \
{
  # current line is not an import definition;
  print importLeadOut;
  StateStack_replaceTop(ParseState_inWhiteSpace);
}

#--------------------

$0 !~ /\#define/ && parseState == ParseState_inMacroDefinition \
{
  # current line is not a macro definition;
  print macroLeadOut;
  StateStack_replaceTop(ParseState_inWhiteSpace);
}

#--------------------

/\#define[\t ]+[^_]/ \
{
  if (parseState != ParseState_inMacroDefinition) {
    print macroLeadIn;
  }

  StateStack_replaceTop(ParseState_inMacroDefinition);

  # check whether the macro line has a continuation
  hasContinuation = (thisLine ~ /\\[ \t]+$/);

  if (thisLine ~ /define[\t ]+[A-Za-z_]+\(/) {
    # this is a macro with parameters
    sub(/\).*$/, ")", thisLine);
  } else {
    thisLine = $1 " " $2;
  }

  thisLine = thisLine " ...";
  convertAndPrintMacroDefinitionLine();

  if (hasContinuation) {
    StateStack_replaceTop(ParseState_inContinuation);
  } else {
    StateStack_replaceTop(ParseState_inMacroDefinition);
  }

  next;
}

#--------------------

/\#ifdef / || /\#endif/ || /\#[\t ]+/ \
{
  next;
}

#--------------------

/\#include/ \
{
  if (parseState != ParseState_inImportDefinition) {
    print importLeadIn;
  }

  convertAndPrintImportDefinitionLine();
  StateStack_replaceTop(ParseState_inImportDefinition);
  next;
}

#--------------------
# COMMENTS
#--------------------

/\/\*\*/ \
{
  # a documentation comment
  if (parseState == ParseState_inDocumentation) {
    print "ERROR: documentation comment within documentation";
  } else if (parseState > ParseState_inMacroDefinition) {
    # some documentation within a routine header, a variable
    # definition or a type definition should be kept as is
  } else {
    sub(/^ *\/\*\* */, "", thisLine);

    if (parseState == ParseState_atBegin) {
      StateStack_replaceTop(ParseState_inWhiteSpace);
      StateStack_push(ParseState_inHeader);
      print headerLeadIn;
    } else {
      StateStack_push(ParseState_inDocumentation);
      print documentationLeadIn;
    }

    DocumentationList_clear();
  }
}

#--------------------

/\*\// && (parseState == ParseState_inDocumentation \
           || parseState == ParseState_inHeader) \
{
  # a documentation comment end
  sub(/ *\*\/ */, "", thisLine);
  isLastLine = true;
}

#--------------------

(parseState == ParseState_inDocumentation \
 || parseState == ParseState_inHeader) \
{
  convertAndPrintDocumentationLine();

  if (isLastLine) {
    DocumentationList_flush();

    if (parseState == ParseState_inHeader) {
      print headerLeadOut;
    } else {
      print documentationLeadOut;
    }

    StateStack_removeTop();
  }

  next;
}

#---------
# TYPEDEFS
#---------

/typedef / \
{
  print typeLeadIn;
  convertAndPrintTypeDefinitionLine();
  StateStack_push(ParseState_inTypeDefinition);
}

#--------------------

(parseState == ParseState_inTypeDefinition) \
	   && ( /}/ || /);$/) \
{
  # the end of a multiline type definition
  convertAndPrintTypeDefinitionLine();
  print typeLeadOut;
  StateStack_removeTop();
  next;
}

#--------------------

/typedef .*;/ \
{
  # some simple one-line type definition
  print typeLeadOut;
  StateStack_replaceTop(ParseState_inWhiteSpace);
  next;
}

#----------
# VARIABLES
#----------

/extern / \
{
  print varLeadIn;
  convertAndPrintVarDefinitionLine();
}

#--------------------

/extern .*;/ \
{
  # some one-line data definition
  StateStack_replaceTop(ParseState_inWhiteSpace);
  print varLeadOut;
  next;
}

#---------
# ROUTINES
#---------

(parseState == ParseState_inWhiteSpace) && / \(/ \
{
  print routineLeadIn;
  convertAndPrintRoutineDefinitionLine();
  StateStack_push(ParseState_inRoutineDefinition);
}

#--------------------

(parseState == ParseState_inRoutineDefinition) && /\);/ \
{
  if (!lineHasBeenPrinted) {
    convertAndPrintRoutineDefinitionLine();
  }

  print routineLeadOut;
  StateStack_removeTop();
  next;
}

#---------------------
# OUTPUT AFTER PARSING
#---------------------

(parseState == ParseState_inContinuation) \
{
  if (thisLine !~ /\\[ \t]*$/) {
    parseState = ParseState_inWhiteSpace;
  }
  next;
}

#--------------------

(parseState == ParseState_inDocumentation \
 || parseState == ParseState_inHeader) \
{
  convertAndPrintDocumentationLine();
  next;
}

#--------------------

(parseState == ParseState_inRoutineDefinition) \
{
  if (!lineHasBeenPrinted) {
    convertAndPrintRoutineDefinitionLine();
  }

  next;
}

#--------------------

(parseState == ParseState_inTypeDefinition) \
{
  convertAndPrintTypeDefinitionLine();
  next;
}

#--------------------

(parseState == ParseState_inVarDefinition) \
{
  convertAndPrintVarDefinitionLine();

  if (thisLine ~ /;/) {
    print varLeadOut;
    StateStack_replaceTop(ParseState_inWhiteSpace);
  }

  next;
}

#--------------------

(parseState == ParseState_inWhiteSpace) \
{
  next;
}

#--------------------
# --   FUNCTIONS   --
#--------------------

function convertAndPrintDocumentationLine() \
  # changes embedded formatting for a documentation line in <thisLine>
{
  if (!lineHasBeenPrinted) {
    sub(/^[ \t]*/, "", thisLine);
    sub(/[A-Za-z]+ +module --/, "", thisLine);
    gsub(/#/, "\\#", thisLine);
    gsub(/_/, "XYYX", thisLine);
    gsub(/</, "\\identifier{", thisLine);
    gsub(/>/, "}", thisLine);
    gsub(/XYYX/, "\\_", thisLine);
    DocumentationList_add(thisLine);
    lineHasBeenPrinted = true;
  }
}

#--------------------

function convertAndPrintImportDefinitionLine() \
  # changes embedded formatting for a macro definition line in
  # <thisLine>
{
  if (!lineHasBeenPrinted) {
    sub(/\#include[ \t]+/, "", thisLine);
    thisLine = toupper(substr(thisLine, 1, 1)) substr(thisLine, 2);
    sub(/Mapcoordinate/, "MapCoordinate", thisLine);
    thisLine = "include " thisLine;
    print thisLine;
    lineHasBeenPrinted = true;
  }
}

#--------------------

function convertAndPrintMacroDefinitionLine() \
  # changes embedded formatting for a macro definition line in
  # <thisLine>
{
  if (!lineHasBeenPrinted) {
    sub(/\#/, "", thisLine);
    print thisLine;
    lineHasBeenPrinted = true;
  }
}

#--------------------

function convertAndPrintRoutineDefinitionLine() \
  # changes embedded formatting for a routine definition line in
  # <thisLine>
{
  if (!lineHasBeenPrinted) {
    sub(/\#/, "", thisLine);
    print thisLine;
    lineHasBeenPrinted = true;
  }
}

#--------------------

function convertAndPrintVarDefinitionLine() \
  # changes embedded formatting for a variable definition line in
  # <thisLine>
{
  if (!lineHasBeenPrinted) {
    print thisLine;
    lineHasBeenPrinted = true;
  }
}

#--------------------

function convertAndPrintTypeDefinitionLine() \
  # changes embedded formatting for a type definition line in
  # <thisLine>
{
  if (!lineHasBeenPrinted) {
    ##gsub(/ +{/, " \\(\\lbrace\\)", thisLine);
    ##gsub(/} +/, "\\(\\rbrace\\) ", thisLine);
    ##gsub(/ /, "~", thisLine);
    print thisLine;
    lineHasBeenPrinted = true;
  }
}

#-------------------------
# MODULE DocumentationList
#-------------------------
  # This module buffers documentation and kicks out extra empty lines

function DocumentationList_add (st) \
  # adds a single line of documentation to buffer
{
  DocumentationList__count++;
  DocumentationList__buffer[DocumentationList__count] = st;
}

#--------------------

function DocumentationList_clear () \
  # resets the buffer for the documentation lines
{
  DocumentationList__count = 0;
}

#--------------------

function DocumentationList_flush () \
  # writes the buffer for the documentation lines and kicks out extra
  # empty lines
{
  State_beforeText = 0;
  State_inText = 1;
  State_inBreak = 2;
  outputState = State_beforeText;

  for (i = 1;  i <= DocumentationList__count; i++) {
    outputLine = DocumentationList__buffer[i];
    lineIsEmpty = (outputLine ~ /^[ \t]*$/);

    if (!lineIsEmpty) {
      if (outputState == State_inBreak) {
	print "";
      }

      print outputLine;
      outputState = State_inText;
    } else if (outputState == State_inText) {
      outputState = State_inBreak;
    }
  }
}

#------------------------------
# END MODULE DocumentationList
#------------------------------


#--------------------
# MODULE StateStack
#--------------------

function StateStack_dump () \
{
  printf "[";

  for (i = StateStack__level;  i > 0;  i--) {
    if (i < StateStack__level) {
      printf ",";
    }

    printf StateStack__info[i];
  }

  printf "]";
}

#--------------------

function StateStack_initialize() \
{
  # initializes all stack stuff
  StateStack__level = 0;
}

#--------------------

function StateStack_push (state) \
{
  # pushes <state> onto stack
  StateStack__info[++StateStack__level] = state;
  parseState = state;
}

#--------------------

function StateStack_replaceTop (state) \
{
  # replaces top entry in stack by <state>
  StateStack__info[StateStack__level] = state;
  parseState = state;
}

#--------------------

function StateStack_removeTop () \
{
  --StateStack__level;
  parseState = StateStack_top();
}

#--------------------

function StateStack_top () \
{
  return StateStack__info[StateStack__level];
}

#-----------------------
# END MODULE StateStack
#-----------------------

