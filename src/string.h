/** String module --
    This module provides all services for handling strings in the
    SDCC linker.

    Those strings are pointers to dynamically sized memory areas and
    are not type compatible with C-strings.  There is a complete set
    of manipulation routines available (assignment, search, slicing,
    ...) and also the conversion from and to integers or C-strings.

    Note that the first character in a string has index 1.

    Original version by Thomas Tensi, 2006-02
*/

#ifndef __MYSTRING_H
#define __MYSTRING_H

/*========================================*/

#include "globdefs.h"
#include "typedescriptor.h"

/*========================================*/

#define String_terminator '\0'
  /** character defining the end of a character array */

#define String_notFound ((SizeType)(-1))
  /** value returned when some string lookup routine fails */


typedef struct String__Record *String_Type;
  /** a character string */


extern TypeDescriptor_Type String_typeDescriptor;
  /** variable used for describing the type properties when string
      objects occur in generic types like lists */


extern String_Type String_newline;
  /** string representing a newline */


extern String_Type String_emptyString;
  /** an empty string */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void String_initialize (void);
  /** sets up internal data structures for this module */

/*--------------------*/

void String_finalize (void);
  /** cleans up internal data structures for this module */


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

String_Type String_allocate (in SizeType capacity);
  /** allocates string with at most <capacity> significant
      characters */

/*--------------------*/

String_Type String_make (void);
  /** constructs empty string */

/*--------------------*/

String_Type String_makeFromCharArray (in char *source);
  /** constructs string from character array <source> */


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void String_destroy (inout String_Type *st);
  /** deallocates string <st> */


/*--------------------*/
/* ACCESS             */
/*--------------------*/

char String_getAt (in String_Type st, in SizeType i);
  /** gets character at <i>-th position in <st> and returns it; when
      <i> is less than 1 or greater than the string length, this
      routine fails */

/*--------------------*/

char *String_asCharPointer (in String_Type st);
  /** returns character array representation of string terminated by
      NUL */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void String_clear (inout String_Type *st);
  /** clears contents of <st> */

/*--------------------*/

void String_copy (out String_Type *destination, in String_Type source);
  /** copies contents of <source> into <destination> */

/*--------------------*/

void String_copyAligned (out String_Type *destination,
			 in UINT8 maxLength, in String_Type source,
			 in char fillChar, in Boolean isLeftAligned);
  /** formats <source> into <destination> using at most <maxLength>
      characters; if <source> is shorter than <maxLength>, the
      remaining space is filled with <fillChar>; when <isLeftAligned>
      source is aligned left in destination, otherwise right */

/*--------------------*/

void String_copyCharArray (out String_Type *destination, in char *source);
  /** copies contents of <source> into <destination> up to and
      including <String_terminator> */

/*--------------------*/

void String_copyCharArrayAligned (out String_Type *destination,
				  in UINT8 maxLength, in char *source,
				  in char fillChar, in Boolean isLeftAligned);
  /** formats <source> into <destination> using at most <maxLength>
      characters; if <source> is shorter than <maxLength>, the
      remaining space is filled with <fillChar>; when <isLeftAligned>
      source is aligned left in destination, otherwise right */

/*--------------------*/

void String_copyInteger (out String_Type *destination, in INT32 value,
			 in UINT8 base);
  /** formats <value> with <base> and copies result into
      <destination> */

/*--------------------*/

void String_copyIntegerAligned (out String_Type *destination,
				in UINT8 maxLength, in INT32 value,
				in UINT8 base,
				in char fillChar, in Boolean isLeftAligned);
  /** formats integer <value> with <base> into <destination> using at
      most <maxLength> characters; if resulting number string is
      shorter than <maxLength>, the remaining space is filled with
      <fillChar>; when <isLeftAligned> source is aligned left in
      destination, otherwise right */

/*--------------------*/

void String_append (inout String_Type *destination,
		    in String_Type otherString);
  /** appends contents of <otherString> to <destination> */

/*--------------------*/

void String_appendChar (inout String_Type *destination, in char ch);
  /** appends character <ch> to <destination> */

/*--------------------*/

void String_appendCharArray (inout String_Type *destination,
			     in char *otherString);
  /** appends contents of <otherString> to <destination> */

/*--------------------*/

void String_appendInteger (out String_Type *destination, in UINT32 value,
			   in UINT8 base);
  /** formats <value> with base <base> and appends result to
      <destination> */

/*--------------------*/

void String_deleteCharacters (inout String_Type *st, in SizeType position,
			      in SizeType count);
  /** deletes <count> characters in <st> starting at <position> */

/*--------------------*/

void String_fillWithCharacter (inout String_Type *st, in char ch,
			       in SizeType count);
  /** fills first <count> characters of <st> with character <ch> */

/*--------------------*/

void String_prepend (inout String_Type *destination,
		     in String_Type otherString);
  /** prepends contents of <otherString> to <destination> */

/*--------------------*/

void String_prependChar (inout String_Type *destination, in char ch);
  /** prepends character <ch> to <destination> */

/*--------------------*/

void String_prependCharArray (inout String_Type *destination,
			      in char *otherString);
  /** prepends contents of <otherString> to <destination> */

/*--------------------*/

void String_prependInteger (out String_Type *destination, in UINT32 value,
			    in UINT8 base);
  /** formats <value> with base <base> and prepends result to
      <destination> */

/*--------------------*/

void String_removeTrailingCrLf (inout String_Type *st);
  /** removes trailing line feed or carriage return characters of <st> */


/*--------------------*/
/* CONVERSION         */
/*--------------------*/

void String_convertToCharArray (in String_Type st, in SizeType maxSize,
				out char *chList);
  /** puts contents of <st> into character array <chList> terminated
      by <String_terminator> when length of <st> is less or equal to
      <maxSize> */

/*--------------------*/

Boolean String_convertToLong (in String_Type st, in UINT8 defaultBase,
			      out long *result);
  /** parses contents of <st> as long number with default base
      <defaultBase> and returns result in <result>; any base changing
      prefices (like "0x") are interpreted; returns false on
      failure */

/*--------------------*/

void String_convertToUpperCase (in String_Type st, out String_Type *result);
  /** returns upper case representation of <st> in <result> */


/*--------------------*/
/* MEASUREMENT        */
/*--------------------*/

SizeType String_findCharacter (in String_Type st, in char ch);
  /** locates <ch> in <st> and returns its position; when <ch> does
      not occur, <notFound> is returned */

/*--------------------*/

SizeType String_findCharacterFromEnd (in String_Type st, in char ch);
  /** locates <ch> in <st> starting at end of string and returns its
      position; when <ch> does not occur, <notFound> is returned */

/*--------------------*/

SizeType String_find (in String_Type st, in String_Type substring);
  /** locates <substring> in <st> and returns its position; when
      <substring> does not occur, <notFound> is returned */

/*--------------------*/

SizeType String_findFromEnd (in String_Type st, in String_Type substring);
  /** locates <substring> in <st> starting at end of string and
      returns its position; when <substring> does not occur,
      <notFound> is returned */

/*--------------------*/

char String_getCharacter (in String_Type st, in SizeType i);
  /** gets <i>-th character in <st> where the first character has
      index 1 */

/*--------------------*/

void String_getSubstring (out String_Type *result, in String_Type st,
			  in SizeType startPosition, in SizeType count);
  /** gets substring of <st> from <startPosition> of at most <count>
      characters into <result>; fails when <startPosition> is
      non-positive or <count> is negative */

/*--------------------*/

Boolean String_hasPrefix (in String_Type st, in String_Type prefix);
  /** tells whether <st> has leading <prefix> */

/*--------------------*/

Boolean String_hasSuffix (in String_Type st, in String_Type suffix);
  /** tells whether <st> has trailing <suffix> */

/*--------------------*/

SizeType String_length (in String_Type st);
  /** returns length of string <st> */

/*--------------------*/

Boolean String_isEqual (in String_Type strA, in String_Type strB);
  /** tells whether two strings <strA> and <strB> are equal */


/*--------------------*/
/* TRANSFORMATION     */
/*--------------------*/

SizeType String_hashCode (in String_Type st);
  /** computes the hash code for string <st>; a simple hash code is
      used: if the sequence of character codes is c_1,c_2,...,c_n, its
      hash value will be \(SUM_{i} (2^{n-i} * c_i)\) */

#endif /* __MYSTRING_H */
