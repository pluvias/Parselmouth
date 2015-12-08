/* Data.cpp
 *
 * Copyright (C) 1992-2012,2015 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "Collection.h"

Thing_implement (Daata, Thing, 0);

structMelderDir Data_directoryBeingRead { { 0 } };

void structDaata :: v_copy (Daata /* thee */) {
}

bool structDaata :: v_equal (Daata /* thee */) {
	return true;
}   // names of "identical" objects are allowed to be different

bool structDaata :: v_canWriteAsEncoding (int /* encoding */) {
	return true;
}

void structDaata :: v_writeText (MelderFile /* openFile */) {
}

void structDaata :: v_readText (MelderReadText, int /* formatVersion */) {
}

void structDaata :: v_writeBinary (FILE *) {
}

void structDaata :: v_readBinary (FILE *, int /*formatVersion*/) {
}

Daata _Data_copy (Daata me) {
	try {
		if (! me) return nullptr;
		autoDaata thee = static_cast <Daata> (Thing_newFromClass (my classInfo));
		my v_copy (thee.peek());
		Thing_setName (thee.peek(), my name);
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": not copied.");
	}
}

bool Data_equal (Daata me, Daata thee) {
	if (my classInfo != thy classInfo) return false;   // different class: not equal
	int offset = sizeof (struct structDaata);   // we already compared the methods, and are going to skip the names
	if (! memcmp ((char *) me + offset, (char *) thee + offset, my classInfo -> size - offset))   // BUG: not necessarily portable
		return true;   // no shallow differences
	return my v_equal (thee);
}

bool Data_canWriteAsEncoding (Daata me, int encoding) {
	return my v_canWriteAsEncoding (encoding);
}

bool Data_canWriteText (Daata me) {
	return my v_writable ();
}

void Data_writeText (Daata me, MelderFile openFile) {
	my v_writeText (openFile);
	if (ferror (openFile -> filePointer))
		Melder_throw (U"I/O error.");
}

MelderFile Data_createTextFile (Daata me, MelderFile file, bool verbose) {
	autoMelderFile mfile = MelderFile_create (file);
	#if defined (_WIN32)
		file -> requiresCRLF = true;
	#endif
	file -> verbose = verbose;
	file -> outputEncoding = Melder_getOutputEncoding ();
	if (file -> outputEncoding == kMelder_textOutputEncoding_ASCII_THEN_UTF16)
		file -> outputEncoding = Data_canWriteAsEncoding (me, kMelder_textOutputEncoding_ASCII) ? kMelder_textOutputEncoding_ASCII : kMelder_textOutputEncoding_UTF16;
	else if (file -> outputEncoding == kMelder_textOutputEncoding_ISO_LATIN1_THEN_UTF16)
		file -> outputEncoding = Data_canWriteAsEncoding (me, kMelder_textOutputEncoding_ISO_LATIN1) ? kMelder_textOutputEncoding_ISO_LATIN1 : kMelder_textOutputEncoding_UTF16;
	if (file -> outputEncoding == kMelder_textOutputEncoding_UTF16) {
		binputu2 (0xfeff, file -> filePointer);
	}
	return mfile.transfer();
}

static void _Data_writeToTextFile (Daata me, MelderFile file, bool verbose) {
	try {
		if (! Data_canWriteText (me))
			Melder_throw (U"Objects of class ", my classInfo -> className, U" cannot be written to a text file.");
		autoMelderFile mfile = Data_createTextFile (me, file, verbose);
		#ifndef _WIN32
			flockfile (file -> filePointer);   // BUG
		#endif
		MelderFile_write (file, U"File type = \"ooTextFile\"\nObject class = \"", my classInfo -> className);
		if (my classInfo -> version > 0)
			MelderFile_write (file, U" ", my classInfo -> version);
		MelderFile_write (file, U"\"\n");
		Data_writeText (me, file);
		MelderFile_writeCharacter (file, U'\n');
		#ifndef _WIN32
			if (file -> filePointer) funlockfile (file -> filePointer);
		#endif
		mfile.close ();
	} catch (MelderError) {
		#ifndef _WIN32
			if (file -> filePointer) funlockfile (file -> filePointer);   // the file pointer is null before Data_createTextFile() and after mfile.close()
		#endif
		throw;
	}
}

void Data_writeToTextFile (Daata me, MelderFile file) {
	try {
		_Data_writeToTextFile (me, file, true);
	} catch (MelderError) {
		Melder_throw (me, U": not written to text file ", file, U".");
	}
}

void Data_writeToShortTextFile (Daata me, MelderFile file) {
	try {
		_Data_writeToTextFile (me, file, false);
	} catch (MelderError) {
		Melder_throw (me, U": not written to short text file ", file, U".");
	}
}

bool Data_canWriteBinary (Daata me) {
	return my v_writable ();
}

void Data_writeBinary (Daata me, FILE *f) {
	my v_writeBinary (f);
	if (ferror (f))
		Melder_throw (U"I/O error.");
}

void Data_writeToBinaryFile (Daata me, MelderFile file) {
	try {
		if (! Data_canWriteBinary (me))
			Melder_throw (U"Objects of class ", my classInfo -> className, U" cannot be written to a generic binary file.");
		autoMelderFile mfile = MelderFile_create (file);
		if (fprintf (file -> filePointer, "ooBinaryFile") < 0)
			Melder_throw (U"Cannot write first bytes of file.");
		binputw1 (
			my classInfo -> version > 0 ?
				Melder_cat (my classInfo -> className, U" ", my classInfo -> version) :
				my classInfo -> className,
			file -> filePointer);
		Data_writeBinary (me, file -> filePointer);
		mfile.close ();
	} catch (MelderError) {
		Melder_throw (me, U": not written to binary file ", file, U".");
	}
}

bool Data_canReadText (Daata me) {
	return my v_writable ();
}

void Data_readText (Daata me, MelderReadText text, int formatVersion) {
	try {
		my v_readText (text, formatVersion);
		my v_repair ();
	} catch (MelderError) {
		Melder_throw (Thing_className (me), U" not read.");
	}
}

autoDaata Data_readFromTextFile (MelderFile file) {
	try {
		autoMelderReadText text = MelderReadText_createFromFile (file);
		char32 *line = MelderReadText_readLine (text.peek());
		if (! line)
			Melder_throw (U"No lines.");
		char32 *end = str32str (line, U"ooTextFile");   // oo format?
		autoDaata me;
		int formatVersion;
		if (end) {
			autostring32 klas = texgetw2 (text.peek());
			me = static_cast <Daata> (Thing_newFromClassName (klas.peek(), & formatVersion));
		} else {
			end = str32str (line, U"TextFile");
			if (! end)
				Melder_throw (U"Not an old-type text file; should not occur.");
			*end = U'\0';
			me = static_cast <Daata> (Thing_newFromClassName (line, nullptr));
			formatVersion = -1;   // old version
		}
		MelderFile_getParentDir (file, & Data_directoryBeingRead);
		Data_readText (me.peek(), text.peek(), formatVersion);
		file -> format = structMelderFile :: Format :: text;
		return me;
	} catch (MelderError) {
		Melder_throw (U"Data not read from text file ", file, U".");
	}
}

bool Data_canReadBinary (Daata me) {
	return my v_writable ();
}

void Data_readBinary (Daata me, FILE *f, int formatVersion) {
	try {
		my v_readBinary (f, formatVersion);
		if (feof (f))
			Melder_throw (U"Early end of file.");
		if (ferror (f))
			Melder_throw (U"I/O error.");
		my v_repair ();
	} catch (MelderError) {
		Melder_throw (Thing_className (me), U" not read.");
	}
}

autoDaata Data_readFromBinaryFile (MelderFile file) {
	try {
		autofile f = Melder_fopen (file, "rb");
		char line [200];
		int n = fread (line, 1, 199, f); line [n] = '\0';
		char *end = strstr (line, "ooBinaryFile");
		autoDaata me;
		int formatVersion;
		if (end) {
			fseek (f, strlen ("ooBinaryFile"), 0);
			autostring8 klas = bingets1 (f);
			me = static_cast <Daata> (Thing_newFromClassName (Melder_peek8to32 (klas.peek()), & formatVersion));
		} else {
			end = strstr (line, "BinaryFile");
			if (! end) {
				Melder_throw (U"File ", file, U" is not a Data binary file.");
			}
			*end = '\0';
			me = static_cast <Daata> (Thing_newFromClassName (Melder_peek8to32 (line), nullptr));
			formatVersion = -1;   // old version: override version number, which was set to 0 by newFromClassName
			rewind (f);
			fread (line, 1, end - line + strlen ("BinaryFile"), f);
		}
		MelderFile_getParentDir (file, & Data_directoryBeingRead);
		Data_readBinary (me.peek(), f, formatVersion);
		file -> format = structMelderFile :: Format :: binary;
		f.close (file);
		return me;
	} catch (MelderError) {
		Melder_throw (U"Data not read from binary file ", file, U".");
	}
}

/* Generic reading. */

static int numFileTypeRecognizers = 0;
static Data_FileTypeRecognizer fileTypeRecognizers [100];
void Data_recognizeFileType (Data_FileTypeRecognizer recognizer) {
	Melder_assert (numFileTypeRecognizers < 100);
	fileTypeRecognizers [++ numFileTypeRecognizers] = recognizer;
}

autoDaata Data_readFromFile (MelderFile file) {
	int nread, i;
	char header [513];
	autofile f = Melder_fopen (file, "rb");
	nread = fread (& header [0], 1, 512, f);
	f.close (file);
	header [nread] = 0;

	/***** 1. Is this file a text file as defined in Data.cpp? *****/

	if (nread > 11) {
		char *p = strstr (header, "TextFile");
		if (p && p - header < nread - 8 && p - header < 40)
			return Data_readFromTextFile (file);
	}
	if (nread > 22) {
		char headerCopy [101];
		memcpy (headerCopy, header, 100);
		headerCopy [100] = '\0';
		for (i = 0; i < 100; i ++)
			if (headerCopy [i] == '\0') headerCopy [i] = '\001';
		char *p = strstr (headerCopy, "T\001e\001x\001t\001F\001i\001l\001e");
		if (p && p - headerCopy < nread - 15 && p - headerCopy < 80)
			return Data_readFromTextFile (file);
	}

	/***** 2. Is this file a binary file as defined in Data.cpp? *****/

	if (nread > 13) {
		char *p = strstr (header, "BinaryFile");
		if (p && p - header < nread - 10 && p - header < 40)
			return Data_readFromBinaryFile (file);
	}

	/***** 3. Is this file of a type for which a recognizer has been installed? *****/

	MelderFile_getParentDir (file, & Data_directoryBeingRead);
	for (i = 1; i <= numFileTypeRecognizers; i ++) {
		autoDaata object = fileTypeRecognizers [i] (nread, header, file);
		if (object) {
			if (object -> classInfo == classDaata)   // dummy object? the recognizer could have had a side effect, such as drawing a picture
				return autoDaata ();
			return object;
		}
	}

	/***** 4. Is this a common text file? *****/

	for (i = 0; i < nread; i ++)
		if (header [i] < 32 || header [i] > 126)   // not ASCII?
			break;
	if (i >= nread) return Data_readFromTextFile (file);

	Melder_throw (U"File ", file, U" not recognized.");
}

/* Recursive routines for working with struct members. */

int Data_Description_countMembers (Data_Description structDescription) {
	int count = 0;
	for (Data_Description desc = structDescription; desc -> name; desc ++)
		count ++;
	if (structDescription [0]. type == inheritwa) {
		Data_Description parentDescription = ((Daata) _Thing_dummyObject ((ClassInfo) structDescription [0]. tagType)) -> v_description ();
		if (parentDescription)
			return count + Data_Description_countMembers (parentDescription);
	}
	return count;
}

Data_Description Data_Description_findMatch (Data_Description structDescription, const char32 *name) {
	for (Data_Description desc = structDescription; desc -> name; desc ++)
		if (str32equ (name, desc -> name)) return desc;
	if (structDescription [0]. type == inheritwa) {
		Data_Description parentDescription = ((Daata) _Thing_dummyObject ((ClassInfo) structDescription [0]. tagType)) -> v_description ();
		if (parentDescription)
			return Data_Description_findMatch (parentDescription, name);
	}
	return nullptr;   // not found
}

Data_Description Data_Description_findNumberUse (Data_Description structDescription, const char32 *string) {
	for (Data_Description desc = structDescription; desc -> name; desc ++) {
		if (desc -> max1 && str32equ (desc -> max1, string)) return desc;
		if (desc -> max2 && str32equ (desc -> max2, string)) return desc;
	}
	if (structDescription [0]. type == inheritwa) {
		Data_Description parentDescription = ((Daata) _Thing_dummyObject ((ClassInfo) structDescription [0]. tagType)) -> v_description ();
		if (parentDescription)
			return Data_Description_findNumberUse (parentDescription, string);
	}
	return nullptr;
}

/* Retrieving data from object + description. */

int64 Data_Description_integer (void *address, Data_Description description) {
	switch (description -> type) {
		case bytewa:           return * (signed char *)    ((char *) address + description -> offset);
		case intwa:            return * (int *)            ((char *) address + description -> offset);
		case longwa:           return * (long *)           ((char *) address + description -> offset);
		case ubytewa:          return * (unsigned char *)  ((char *) address + description -> offset);
		case uintwa:           return * (unsigned int *)   ((char *) address + description -> offset);
		case ulongwa:          return * (unsigned long *)  ((char *) address + description -> offset);
		case boolwa:           return * (bool *)           ((char *) address + description -> offset);
		case objectwa:         return (* (Collection *)    ((char *) address + description -> offset)) -> size;
		case autoobjectwa:     return (* (Collection *)    ((char *) address + description -> offset)) -> size;   // FIXME: alignment not guaranteed
		case collectionwa:     return (* (Collection *)    ((char *) address + description -> offset)) -> size;
		case autocollectionwa: return (* (Collection *)    ((char *) address + description -> offset)) -> size;   // FIXME: alignment not guaranteed
		default: return 0;
	}
}

int Data_Description_evaluateInteger (void *structAddress, Data_Description structDescription,
	const char32 *formula, long *result)
{
	if (! formula) {   // this was a VECTOR_FROM array
		*result = 1;
		return 1;
	}
	if (formula [0] >= U'a' && formula [0] <= U'z') {
		char32 buffer [100], *minus1, *psize;
		Data_Description sizeDescription;
		str32cpy (buffer, formula);
		if ((minus1 = str32str (buffer, U" - 1")) != nullptr)
			*minus1 = U'\0';   // strip trailing " - 1", but remember
		if ((psize = str32str (buffer, U" -> size")) != nullptr)
			*psize = U'\0';   // strip trailing " -> size"
		if (! (sizeDescription = Data_Description_findMatch (structDescription, buffer))) {
			*result = 0;
			return 0 /*Melder_error ("Cannot find member \"%ls\".", buffer)*/;
		}
		*result = Data_Description_integer (structAddress, sizeDescription);
		if (minus1) *result -= 1;
	} else {
		*result = Melder_atoi (formula);
	}
	return 1;
}

/* End of file Data.cpp */