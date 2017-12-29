/** String module --
    Implementation of module providing all services for handling
    strings in a GameBoy game.

    Original version by Thomas Tensi, 2006-02
*/

#include "string.h"

/*========================================*/

#include <ctype.h>
#  define CType_toupper   toupper
#include <stdlib.h>
#  define StdLib_malloc   malloc
#  define StdLib_realloc  realloc
#  define StdLib_strtol   strtol
#include <string.h>
#  define STRING_strchr   strchr
#  define STRING_strcmp   strcmp
#  define STRING_strcpy   strcpy
#  define STRING_strlen   strlen
#  define STRING_strncmp  strncmp
#  define STRING_strrchr  strrchr
#  define STRING_strstr   strstr
#  define STRING_memmove  memmove
#  define STRING_memset   memset

#include "error.h"
#include "file.h"
#include "globdefs.h"

/*========================================*/

#define String__maxNumberOfIToADigits 16
#define String__magicNumber 0x12345678

typedef struct String__Record {
  UINT32 magicNumber;
  UINT32 capacity;
  char *characterList;
} String__Record;
  /** string type with a leading information about the allocated size
      and a pointer to a variable sized character array (this type
      information should only be used internally!) */

/*--------------------*/

static TypeDescriptor_Record String__tdRecord =
  { /* .objectSize = */ 0,
    /* .assignmentProc = */ (TypeDescriptor_AssignmentProc) String_copy,
    /* .comparisonProc = */ (TypeDescriptor_ComparisonProc) String_isEqual,
    /* .constructionProc = */ (TypeDescriptor_ConstructionProc) String_make,
    /* .destructionProc = */ (TypeDescriptor_DestructionProc) String_destroy,
    /* .hashCodeProc = */ (TypeDescriptor_HashCodeProc) String_hashCode,
    /* .keyValidationProc = */(TypeDescriptor_KeyValidationProc) String_isEqual
  };

TypeDescriptor_Type String_typeDescriptor = &String__tdRecord;
String_Type String_newline;
String_Type String_emptyString;


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static Boolean String__isValid (in Object st)
  /** checks whether <st> is really a pointer to string */
{
  return isValidObject(st, String__magicNumber);
}


/*--------------------*/

static Boolean String__checkValidityPRE (in Object st, in char *procName)
  /** checks as a precondition of routine <procName> whether <st> is
      a valid string and returns the check result */
{
  return PRE(String__isValid(st), procName, "invalid string");
}

/*--------------------*/
/*--------------------*/

static void String__unsignedIToA (in UINT32 n, in char *st, in UINT8 radix)
  /** puts a representation of an unsigned <n> in <radix> into <st> */
{
  char *hexDigit = "0123456789ABCDEF";
  char buffer[String__maxNumberOfIToADigits];
  char *bufferPtr = buffer;
  char ch;

  PRE(radix <= 16, "String__unsignedIToA", "radix > 16");

  *bufferPtr++ = String_terminator;

  do {
    *bufferPtr++ = hexDigit[n % radix];
    n /= radix;
  } while (n != 0);

  do {
    ch = *--bufferPtr;
    *st++ = ch;
  } while (ch != String_terminator);
}

/*--------------------*/

static void String__itoa (in INT32 value, in char *string, in UINT8 radix)
  /** puts a representation of an integer <value> in <radix> into
      <string> */
{
  if (value < 0 && radix == 10) {
    *string++ = '-';
    String__unsignedIToA(-value, string, radix);
  }
  else {
    String__unsignedIToA(value, string, radix);
  }
}

/*--------------------*/

static void String__ensureCapacity (inout String_Type st,
				    in SizeType count)
   /** ensures that <st> may contain <count> significant characters
       plus the additional terminator */
{
  if (st->capacity < count) {
    /* it is necessary to reallocate the character list; we add some
       slack to be prepared for some later extension */
    count += 10;
    st->capacity = count;
    st->characterList = StdLib_realloc(st->characterList, count + 1);

    if (st->characterList == NULL) {
      Error_raise(Error_Criticality_fatalError,
		  "String__ensureCapacity: out of memory");
    }
  }
}

/*--------------------*/

static SizeType String__convertPointerToIndex (in char *st, in char *ptr)
  /** returns notFound when <ptr> is NULL, otherwise the character
      position within <st> (where the first character has position
      1) */
{
  SizeType result;

  if (ptr == NULL) {
    result = String_notFound;
  } else {
    result = ptr - st + 1;
  }

  return result;
}

/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void String_initialize (void)
{
  String_emptyString = String_make();
  String_newline = String_makeFromCharArray("\n");
}

/*--------------------*/

void String_finalize (void)
{
  String_destroy(&String_emptyString);
  String_destroy(&String_newline);
}


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

String_Type String_allocate (in SizeType capacity)
{
  Boolean isOkay = true;
  String_Type st = StdLib_malloc(sizeof(String__Record));

  if (st == NULL) {
    isOkay = false;
  } else {
    st->magicNumber = String__magicNumber;
    st->capacity = capacity;
    /* allocate space for <capacity> significant characters plus a
       String_terminator */
    st->characterList = StdLib_malloc(capacity + 1);

    if (st->characterList == NULL) {
      isOkay = false;
    } else {
      st->characterList[0] = String_terminator;
    }
  }

  if (!isOkay) {
    Error_raise(Error_Criticality_fatalError,
		"String__allocate: out of memory");
  }

  return st;
}

/*--------------------*/

String_Type String_make (void)
{
  return String_allocate(0);
}

/*--------------------*/

String_Type String_makeFromCharArray (in char *source)
{
  SizeType length = STRING_strlen(source);
  String_Type st = String_allocate(length);
  String_copyCharArray(&st, source);
  return st;
}


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void String_destroy (inout String_Type *st)
{
  char *procName = "String_destroy";
  String_Type string = *st;
  Boolean precondition = String__checkValidityPRE(string, procName);

  if (precondition) {
    DESTROY(string->characterList);
    string->magicNumber = 0;
    DESTROY(string);
  }

  *st = NULL;
}


/*--------------------*/
/* ACCESS             */
/*--------------------*/

char String_getAt (in String_Type st, in SizeType i)
{
  char *procName = "String_getAt";
  Boolean precondition = (String__checkValidityPRE(st, procName)
			  && 
			  PRE((1 <= i) 
			      && (i <= STRING_strlen(st->characterList)),
			      procName, "bad index"));
  char result = 0;

  if (precondition) {
    result = st->characterList[i-1];
  }

  return result;
}

/*--------------------*/

char *String_asCharPointer (in String_Type st)
{
  char *procName = "String_asCharPointer";
  Boolean precondition = String__checkValidityPRE(st, procName);
  char *result = NULL;

  if (precondition) {
    result = st->characterList;
  }

  return result;
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void String_clear (out String_Type *st)
{
  char *procName = "String_clear";
  String_Type string = *st;
  Boolean precondition = String__checkValidityPRE(string, procName);

  if (precondition) {
    String_copyCharArray(st, "");
  }
}

/*--------------------*/

void String_copy (out String_Type *destination, in String_Type source)
{
  char *procName = "String_copy";
  Boolean precondition = (String__checkValidityPRE(*destination, procName)
			  && String__checkValidityPRE(source, procName));

  if (precondition) {
    String_copyCharArray(destination, source->characterList);
  }
}

/*--------------------*/

void String_copyAligned (out String_Type *destination,
			 in UINT8 maxLength, in String_Type source,
			 in char fillChar, in Boolean isLeftAligned)
{
  char *procName = "String_copyAligned";
  Boolean precondition = String__checkValidityPRE(source, procName);

  if (precondition) {
    String_copyCharArrayAligned(destination, maxLength, source->characterList,
				fillChar, isLeftAligned);
  }
}

/*--------------------*/

void String_copyCharArray (out String_Type *destination, in char *source)
{
  char *procName = "String_copyCharArray";
  String_Type destString = *destination;
  Boolean precondition = String__checkValidityPRE(destString, procName);

  if (precondition) {
    SizeType length = STRING_strlen(source);
    String__ensureCapacity(destString, length);
    STRING_memmove(destString->characterList, source, length + 1);
  }
}

/*--------------------*/

void String_copyCharArrayAligned (out String_Type *destination,
				  in UINT8 maxLength, in char *source,
				  in char fillChar, in Boolean isLeftAligned)
{
  char *procName = "String_copyCharArrayAligned";
  String_Type destString = *destination;
  Boolean precondition = String__checkValidityPRE(destString, procName);

  if (precondition) {
    SizeType sourceLength = STRING_strlen(source);

    if (sourceLength > maxLength) {
      /* source is too long ==> truncate */
      if (!isLeftAligned) {
	/* cut off leading characters */
	SizeType offset = sourceLength - maxLength;
	String_copyCharArray(destination, &source[offset]);
      } else {
	/* cut off trailing characters */
	String_copyCharArray(destination, source);
	destString->characterList[maxLength] = String_terminator;
      }
    } else {
      SizeType fillCharCount = maxLength - sourceLength;
      String_Type temp = String_allocate(fillCharCount);

      temp->characterList[fillCharCount] = String_terminator;

      while (fillCharCount-- > 0) {
	temp->characterList[fillCharCount] = fillChar;
      }

      String_copyCharArray(destination, source);

      if (isLeftAligned) {
	String_append(destination, temp);
      } else {
	String_prepend(destination, temp);
      }

      String_destroy(&temp);
    }
  }
}

/*--------------------*/

void String_copyInteger (out String_Type *destination, in INT32 value,
			 in UINT8 base)
{
  char *procName = "String_copyInteger";
  Boolean precondition = String__checkValidityPRE(*destination, procName);

  if (precondition) {
    char numberString[String__maxNumberOfIToADigits];
    String__itoa(value, numberString, base);
    String_copyCharArray(destination, numberString);
  }
}

/*--------------------*/

void String_copyIntegerAligned (out String_Type *destination,
				in UINT8 maxLength, in INT32 value,
				in UINT8 base,
				in char fillChar, in Boolean isLeftAligned)
{
  String_Type numberString = String_make();
  String_copyInteger(&numberString, value, base);
  String_copyAligned(destination, maxLength, numberString, fillChar,
		     isLeftAligned);
  String_destroy(&numberString);
}

/*--------------------*/

void String_append (inout String_Type *destination, 
		    in String_Type otherString)
{
  char *procName = "String_append";
  Boolean precondition = (String__checkValidityPRE(*destination, procName)
			  && String__checkValidityPRE(otherString, procName));

  if (precondition) {
    String_appendCharArray(destination, otherString->characterList);
  }
}

/*--------------------*/

void String_appendChar (inout String_Type *destination, in char ch)
{
  char *procName = "String_appendChar";
  Boolean precondition = String__checkValidityPRE(*destination, procName);
			 
  if (precondition) {
    char st[2];
    st[1] = String_terminator;
    st[0] = ch;
    String_appendCharArray(destination, st);
  }
}

/*--------------------*/

void String_appendCharArray (inout String_Type *destination,
			     in char *otherString)
{
  char *procName = "String_appendCharArray";
  String_Type destString = *destination;
  Boolean precondition = String__checkValidityPRE(destString, procName);

  if (precondition) {
    SizeType destinationLength = String_length(destString);
    SizeType otherLength = STRING_strlen(otherString);
    char *secondPartChar;
    SizeType totalLength = destinationLength + otherLength;

    String__ensureCapacity(destString, totalLength);
    secondPartChar = &destString->characterList[destinationLength];
    STRING_memmove(secondPartChar, otherString, otherLength + 1);
  }
}

/*--------------------*/

void String_appendInteger (out String_Type *destination, in UINT32 value,
			   in UINT8 base)
{
  char *procName = "String_appendInteger";
  Boolean precondition = String__checkValidityPRE(*destination, procName);

  if (precondition) {
    String_Type numberString = String_make();
    String_copyInteger(&numberString, value, base);
    String_append(destination, numberString);
    String_destroy(&numberString);
  }
}

/*--------------------*/

void String_deleteCharacters (inout String_Type *st, in SizeType position,
			      in SizeType count)
{
  char *procName = "String_deleteCharacters";
  String_Type string = *st;
  Boolean precondition = (String__checkValidityPRE(string, procName)
			  && PRE((position > 0 && count > 0), procName,
				 "bad parameters")
			  && PRE((STRING_strlen(string->characterList) 
				  <= position + count - 1), procName,
				 "string too short") );

  if (precondition) {
    char *characterList = string->characterList;
    SizeType stringLength = STRING_strlen(characterList);
    SizeType positionA;
    SizeType positionB;

    positionA = position - 1;
    positionB = positionA + count;
    STRING_memmove(&characterList[positionA], &characterList[positionB],
		   stringLength - positionB + 1);
  }
}

/*--------------------*/

void String_fillWithCharacter (inout String_Type *st, in char ch,
			       in SizeType count)
{
  char *procName = "String_fillWithCharacter";
  String_Type string = *st;
  Boolean precondition = String__checkValidityPRE(string, procName);

  if (precondition) {
    char *characterList;
    String__ensureCapacity(string, count);
    characterList = string->characterList;
    STRING_memset(characterList, ch, count);
    characterList[count] = String_terminator;
  }
}

/*--------------------*/

void String_prepend (inout String_Type *destination, 
		     in String_Type otherString)
{
  char *procName = "String_prepend";
  Boolean precondition = (String__checkValidityPRE(*destination, procName)
			  && String__checkValidityPRE(otherString, procName));

  if (precondition) {
    String_prependCharArray(destination, otherString->characterList);
  }
}

/*--------------------*/

void String_prependChar (inout String_Type *destination, in char ch)
{
  char *procName = "String_prependChar";
  Boolean precondition = String__checkValidityPRE(*destination, procName);
			 
  if (precondition) {
    char st[2];
    st[1] = String_terminator;
    st[0] = ch;
    String_prependCharArray(destination, st);
  }
}

/*--------------------*/

void String_prependCharArray (inout String_Type *destination,
			      in char *otherString)
{
  char *procName = "String_prependCharArray";
  String_Type destString = *destination;
  Boolean precondition = String__checkValidityPRE(destString, procName);

  if (precondition) {
    SizeType destinationLength = String_length(destString);
    SizeType otherLength = STRING_strlen(otherString);
    char *secondPartChar;
    SizeType totalLength = destinationLength + otherLength;

    String__ensureCapacity(destString, totalLength);
    secondPartChar = &destString->characterList[otherLength];
    STRING_memmove(secondPartChar, destString->characterList,
		   destinationLength + 1);
    STRING_memmove(destString->characterList, otherString, otherLength);
  }
}

/*--------------------*/

void String_prependInteger (out String_Type *destination, in UINT32 value,
			    in UINT8 base)
{
  char *procName = "String_prependInteger";
  Boolean precondition = String__checkValidityPRE(*destination, procName);

  if (precondition) {
    String_Type numberString = String_make();
    String_copyInteger(&numberString, value, base);
    String_prepend(destination, numberString);
    String_destroy(&numberString);
  }
}

/*--------------------*/

void String_removeTrailingCrLf (inout String_Type *st)
{
  char *procName = "String_removeTrailingCrLf";
  String_Type string = *st;
  Boolean precondition = String__checkValidityPRE(string, procName);

  if (precondition) {
    char *characterList = string->characterList;
    SizeType stringLength = STRING_strlen(characterList);
    char *ptr = &characterList[stringLength - 1];
    Boolean isNewlineCharacter = true;

    while (ptr >= characterList && isNewlineCharacter) {
      char ch = *ptr;

      if (ch == '\r' || ch == '\n') {
	ptr--;
      } else {
	isNewlineCharacter = false;
      }
    }

    *++ptr = String_terminator;
  }
}


/*--------------------*/
/* CONVERSION         */
/*--------------------*/

void String_convertToCharArray (in String_Type st, in SizeType maxSize,
				out char *chList)
{
  char *procName = "String_convertToCharArray";
  Boolean precondition = (String__checkValidityPRE(st, procName)
			  && PRE(STRING_strlen(st->characterList) <= maxSize,
				 procName, "destination too short") );

  if (precondition) {
    char *characterList = st->characterList;
    STRING_strcpy(chList, characterList);
  }
}

/*--------------------*/

Boolean String_convertToLong (in String_Type st, in UINT8 defaultBase,
			      out long *result)
{
  char *procName = "String_convertToLong";
  Boolean precondition = String__checkValidityPRE(st, procName);
  Boolean isOkay = false;

  if (precondition) {
    char *characterList = st->characterList;
    char *ptr = characterList;
    UINT8 base = defaultBase;
    char *endPtr;

    if (*ptr == '0') {
      char secondChar = ptr[1];
      /* include following radix specification character */

      if (secondChar != String_terminator) {
	char *radixPtr;
	char *radixCharacters = "@oOqQxXhH";
	char *associatedBases = "88888GGGG";

	radixPtr = STRING_strchr(radixCharacters, secondChar);

	if (radixPtr != NULL) {
	  char baseChar = associatedBases[radixPtr - radixCharacters];
	  base = (baseChar == '2' ? 2
		  : (baseChar == '8' ? 8
		     : (baseChar == 'A' ? 10 : 16)));
	  ptr += 2;
	}
      }
    }

    *result = StdLib_strtol(ptr, &endPtr, base);
    isOkay = (ptr != endPtr);
  }

  return isOkay;
}

/*--------------------*/

void String_convertToUpperCase (in String_Type st, out String_Type *result)
{
  char *procName = "String_convertToUpperCase";
  Boolean precondition = (String__checkValidityPRE(st, procName)
			  && String__checkValidityPRE(*result, procName));

  if (precondition) {
    char *charList;
    SizeType i;
    SizeType length = STRING_strlen(st->characterList);
    String_copy(result, st);

    charList = (*result)->characterList;

    for (i = 0;  i < length;  i++) {
      charList[i] = (char) CType_toupper(charList[i]);
    }
  }
}


/*--------------------*/
/* MEASUREMENT        */
/*--------------------*/

SizeType String_findCharacter (in String_Type st, in char ch)
{
  char *procName = "String_findCharacter";
  Boolean precondition = String__checkValidityPRE(st, procName);
  SizeType result = String_notFound;

  if (precondition) {
    char *characterList = st->characterList;
    char *ptr = STRING_strchr(characterList, ch);
    result = String__convertPointerToIndex(characterList, ptr);
  }

  return result;
}

/*--------------------*/

SizeType String_findCharacterFromEnd (in String_Type st,  in char ch)
{
  char *procName = "String_findCharacterFromEnd";
  Boolean precondition = String__checkValidityPRE(st, procName);
  SizeType result = String_notFound;

  if (precondition) {
    char *characterList = st->characterList;
    char *ptr = STRING_strrchr(characterList, ch);
    result = String__convertPointerToIndex(characterList, ptr);
  }

  return result;
}

/*--------------------*/

SizeType String_find (in String_Type st, in String_Type substring)
{
  char *procName = "String_find";
  Boolean precondition = (String__checkValidityPRE(st, procName)
			  && String__checkValidityPRE(substring, procName));
  SizeType result = String_notFound;

  if (precondition) {
    char *characterList    = st->characterList;
    char *subCharacterList = substring->characterList;
    char *ptr = STRING_strstr(characterList, subCharacterList);
    result = String__convertPointerToIndex(characterList, ptr);
  }

  return result;
}

/*--------------------*/

SizeType String_findFromEnd (in String_Type st, in String_Type substring)
{
  char *procName = "String_findFromEnd";
  Boolean precondition = (String__checkValidityPRE(st, procName)
			  && String__checkValidityPRE(substring, procName));
  SizeType result = String_notFound;

  if (precondition) {
    char *characterList    = st->characterList;
    char *subCharacterList = substring->characterList;
    char *ptr = NULL;
    char *previousPtr;
    char *charList = characterList;

    do {  
      previousPtr = ptr;
      ptr = STRING_strstr(charList, subCharacterList);

      if (ptr != NULL) {
	charList = ptr + 1;
      }
    } while (ptr != NULL);

    result = String__convertPointerToIndex(characterList, previousPtr);
  }

  return result;
}

/*--------------------*/

char String_getCharacter (in String_Type st, in SizeType i)
{
  char *procName = "String_getCharacter";
  Boolean precondition = String__checkValidityPRE(st, procName);
  char result = '\0';

  if (precondition) {
    result = st->characterList[i - 1];
  }

  return result;
}

/*--------------------*/

void String_getSubstring (out String_Type *result, in String_Type st,
			  in SizeType startPosition, in SizeType count)
{
  char *procName = "String_getSubstring";
  String_Type destination = *result;
  Boolean precondition = (String__checkValidityPRE(destination, procName)
			  && String__checkValidityPRE(st, procName)
			  && PRE(1 <= startPosition && 0 <= count,
				 procName, "bad argument") );

  if (precondition) {
    char *sourceCharacterList = st->characterList;
    char *destinationCharacterList;
    SizeType sourceLength = STRING_strlen(sourceCharacterList);
    SizeType maxCount;

    maxCount = sourceLength - startPosition + 1;

    if (count > maxCount) {
      count = maxCount;
    }
    
    String__ensureCapacity(destination, count);
    destinationCharacterList = destination->characterList;
    STRING_memmove(destinationCharacterList,
		   &sourceCharacterList[startPosition - 1], count);
    destinationCharacterList[count] = String_terminator;
  }
}

/*--------------------*/

Boolean String_hasPrefix (in String_Type st, in String_Type prefix)
{
  char *procName = "String_hasPrefix";
  Boolean precondition = (String__checkValidityPRE(st, procName)
			  && String__checkValidityPRE(prefix, procName));
  Boolean result = false;

  if (precondition) {
    char *characterList  = st->characterList;
    char *characterListA = prefix->characterList;
    SizeType length  = STRING_strlen(characterList);
    SizeType lengthA = STRING_strlen(characterListA);

    if (lengthA <= length) {
      result = (STRING_strncmp(characterListA, characterList, lengthA) == 0);
    }
  }

  return result;
}

/*--------------------*/

Boolean String_hasSuffix (in String_Type st, in String_Type suffix)
{
  char *procName = "String_hasSuffix";
  Boolean precondition = (String__checkValidityPRE(st, procName)
			  && String__checkValidityPRE(suffix, procName));
  Boolean result = false;

  if (precondition) {
    char *characterList  = st->characterList;
    char *characterListA = suffix->characterList;
    SizeType length  = STRING_strlen(characterList);
    SizeType lengthA = STRING_strlen(characterListA);

    if (lengthA <= length) {
      SizeType position = length - lengthA;
      characterList = &characterList[position];
      result = (STRING_strncmp(characterListA, characterList, lengthA) == 0);
    }
  }

  return result;
}

/*--------------------*/

SizeType String_length (in String_Type st)
{
  char *procName = "String_length";
  Boolean precondition = String__checkValidityPRE(st, procName);
  SizeType result = 0;

  if (precondition) {
    result = STRING_strlen(st->characterList);
  }

  return result;
}

/*--------------------*/

Boolean String_isEqual (in String_Type strA, in String_Type strB)
{
  char *procName = "String_isEqual";
  Boolean precondition = (String__checkValidityPRE(strA, procName)
			  && String__checkValidityPRE(strB, procName));
  Boolean result = false;

  if (precondition) {
    result = (STRING_strcmp(strA->characterList,
			    strB->characterList) == 0);
  }

  return result;
}


/*--------------------*/
/* TRANSFORMATION     */
/*--------------------*/

SizeType String_hashCode (in String_Type st)
{
  char *procName = "String_hashCode";
  Boolean precondition = String__checkValidityPRE(st, procName);
  SizeType hashCode = 0;

  if (precondition) {
    char *characterList = st->characterList;
    SizeType i;
    SizeType length = STRING_strlen(characterList);

    for (i = 0;  i < length; i++) {
      /*      hashCode += (hashCode + characterList[i]);*/
      hashCode += characterList[i];
    }
  }

  return hashCode;
}
