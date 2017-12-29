void Target__writeCodeLine (inout File_Type *file, in CodeOutput_State state,
			    in CodeSequence_Type sequence)
{
  UINT8 i;

  switch (state) {
    case CodeOutput_State_atBegin:
      break;

    case CodeOutput_State_inCode:
      File_writeCharArray(file, "CODE [");
      File_writeHex(file, sequence.offsetAddress, 4);
      File_writeCharArray(file, "]:");
     
      for (i = 0;  i < sequence.length;  i++) {
        File_writeCharArray(file, " ");
	File_writeHex(file, sequence.byteList[i], 2);
      }

      File_writeCharArray(file, "\n");
      break;

    case CodeOutput_State_atEnd:
      break;

    default:
      Error_raise(Error_Criticality_fatalError, 
		  "unknown state");
  }
}





static void Parser_dumpCodeSequence (in CodeSequence_RelocationList *relocationList)
{

  CodeSequence_Type *cs = &Parser__codeSequence;
  int i;
  String_Type segmentName = String_make();

  File_writeCharArray(&File_stderr, "----\nCODE-SEQUENCE (");
  File_writeString(&File_stderr, Parser__fileSequence.currentFileName);
  File_writeCharArray(&File_stderr, "): offset = ");
  File_writeHex(&File_stderr, cs->offsetAddress, 8);
  File_writeCharArray(&File_stderr, "\n   ");

  for (i = 0;  i < cs->length;  i++) {
    File_writeCharArray(&File_stderr, " ");
    File_writeHex(&File_stderr, cs->byteList[i], 2);
  }

  File_writeCharArray(&File_stderr, "\nRELOCATIONS FOR SEGMENT ");
  Area_getSegmentName(relocationList->segment, &segmentName);
  File_writeString(&File_stderr, segmentName);

  if (relocationList->count == 0) {
    File_writeCharArray(&File_stderr, ": none");
  } else {
    File_writeCharArray(&File_stderr, ":\n");
  }

  for (i = 0;  i < relocationList->count;  i++) {
    CodeSequence_Relocation *relocation = &relocationList->list[i];
    File_writeCharArray(&File_stderr, " (");
    File_writeHex(&File_stderr,
		  CodeSequence_convertToInteger(relocation->kind), 2);
    File_writeCharArray(&File_stderr, ",");
    File_writeHex(&File_stderr, relocation->index, 2);
    File_writeCharArray(&File_stderr, ",");
    File_writeHex(&File_stderr, relocation->value, 4);
    File_writeCharArray(&File_stderr, ")");
  }

  File_writeCharArray(&File_stderr, "\n");
  String_destroy(&segmentName);
}


