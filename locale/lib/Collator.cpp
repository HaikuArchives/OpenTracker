/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <Collator.h>
#include <UnicodeChar.h>
#include <String.h>
#include <Message.h>

#include <stdio.h>	// <- for debugging only
#include <ctype.h>


struct input_context {
	input_context(bool ignorePunctuation)
		:
		double_char(0),
		ignore_punctuation(ignorePunctuation)
	{
	}

	uint32	double_char;
	bool	ignore_punctuation;
};


// conversion array for character ranges 192 - 223 & 224 - 255
static uint8 gNoDiacrits[] = {
	'a','a','a','a','a','a','a',
	'c',
	'e','e','e','e',
	'i','i','i','i',
	240,	// eth
	'n',
	'o','o','o','o','o',
	247,	//
	'o',
	'u','u','u','u',
	'y',
	254,	// thorn
	'y'
};

static inline uint32
getPrimaryChar(uint32 c)
{
	if (c < 0x80)
		return tolower(c);

	// this automatically returns lowercase letters
	if (c >= 192 && c < 223)
		return gNoDiacrits[c - 192];
	if (c == 223)	// ß
		return 's';
	if (c >= 224 && c < 256)
		return gNoDiacrits[c - 224];

	return BUnicodeChar::ToLower(c);
}


static inline uint32
getNextChar(const char **string, input_context &context)
{
	uint32 c = context.double_char;
	if (c != 0) {
		context.double_char = 0;
		return c;
	}

	do {
		c = BUnicodeChar::FromUTF8(string);
	} while (context.ignore_punctuation
		&& (BUnicodeChar::IsPunctuation(c) || BUnicodeChar::IsSpace(c)));

	if (c == 223) {
		context.double_char = 's';
		return 's';
	}

	return c;
}


/** Fills the specified buffer with the primary sort key. The buffer
 *	has to be long enough to hold the key.
 *	It returns the position in the buffer immediately after the key;
 *	it does not add a terminating null byte!
 */

static char *
putPrimarySortKey(const char *string, char *buffer, int32 length, bool ignorePunctuation)
{
	input_context context(ignorePunctuation);

	uint32 c;
	for (int32 i = 0; (c = getNextChar(&string, context)) != 0 && i < length; i++) {
		if (c < 0x80)
			*buffer++ = tolower(c);
		else
			BUnicodeChar::ToUTF8(getPrimaryChar(c), &buffer);
	}

	return buffer;
}


//	#pragma mark -


BCollator::BCollator()
	:
	fCollatorImage(B_ERROR),
	fStrength(B_COLLATE_PRIMARY),
	fIgnorePunctuation(true)
{
	// ToDo: the collator construction will have to change; the default
	//	collator should be constructed by the Locale/LocaleRoster, so we
	//	only need a constructor where you specify all details

	fCollator = new BCollatorAddOn();
}


BCollator::BCollator(BMessage *archive)
	: BArchivable(archive),
	fCollator(NULL),
	fCollatorImage(B_ERROR)
{
	int32 data;
	if (archive->FindInt32("loc:strength", &data) == B_OK)
		fStrength = (uint8)data;
	else
		fStrength = B_COLLATE_PRIMARY;

	if (archive->FindBool("loc:punctuation", &fIgnorePunctuation) != B_OK)
		fIgnorePunctuation = true;

	BMessage collatorArchive;
	if (archive->FindMessage("loc:collator", &collatorArchive) == B_OK) {
		BArchivable *unarchived = instantiate_object(&collatorArchive, &fCollatorImage);

		// do we really have a BCollatorAddOn here?
		fCollator = dynamic_cast<BCollatorAddOn *>(unarchived);
		if (fCollator == NULL)
			delete unarchived;
	}

	if (fCollator == NULL) {
		fCollator = new BCollatorAddOn();
		fCollatorImage = B_ERROR;
	}
}


BCollator::~BCollator()
{
	delete fCollator;

	if (fCollatorImage >= B_OK)
		unload_add_on(fCollatorImage);
}


void
BCollator::SetDefaultStrength(int8 strength)
{
	fStrength = strength;
}


int8
BCollator::DefaultStrength() const
{
	return fStrength;
}


void
BCollator::SetIgnorePunctuation(bool ignore)
{
	fIgnorePunctuation = ignore;
}


bool
BCollator::IgnorePunctuation() const
{
	return fIgnorePunctuation;
}


void
BCollator::GetSortKey(const char *string, BString *key, int8 strength)
{
	if (strength == B_COLLATE_DEFAULT)
		strength = fStrength;

	fCollator->GetSortKey(string, key, strength, fIgnorePunctuation);
}


int
BCollator::Compare(const char *a, const char *b, int32 length, int8 strength)
{
	if (length == -1)	// match the whole string
		length = 0x7fffffff;

	return fCollator->Compare(a, b, length,
				strength == B_COLLATE_DEFAULT ? fStrength : strength, fIgnorePunctuation);
}


status_t 
BCollator::Archive(BMessage *archive, bool deep)
{
	status_t status = BArchivable::Archive(archive, deep);
	if (status < B_OK)
		return status;

	if (status == B_OK)
		status = archive->AddInt32("loc:strength", fStrength);
	if (status == B_OK)
		status = archive->AddBool("loc:punctuation", fIgnorePunctuation);

	// ToDo: the following code makes the assumption that only loaded
	//	add-ons would have to be archived - a BRuleBasedCollator would
	//	have to be archived as well, though
	BMessage collatorArchive;
	if (status == B_OK && deep && fCollatorImage >= B_OK
		&& (status = fCollator->Archive(&collatorArchive, true)) == B_OK)
		status = archive->AddMessage("loc:collator", &collatorArchive);

	return status;
}


BArchivable *
BCollator::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BCollator"))
		return new BCollator(archive);

	return NULL;
}


//	#pragma mark -


BCollatorAddOn::BCollatorAddOn()
{
}


BCollatorAddOn::BCollatorAddOn(BMessage *archive)
	: BArchivable(archive)
{
}


BCollatorAddOn::~BCollatorAddOn()
{
}


void 
BCollatorAddOn::GetSortKey(const char *string, BString *key, int8 strength,
	bool ignorePunctuation)
{
	if (strength == B_COLLATE_QUATERNARY) {
		// the difference between tertiary and quaternary collation strength
		// are usually a different handling of punctuation characters
		ignorePunctuation = false;
	}

	int32 length = strlen(string);

	switch (strength) {
		case B_COLLATE_PRIMARY:
		{
			char *begin = key->LockBuffer(length * 2);
				// the primary key needs to make space for doubled characters (like 'ß')

			char *end = putPrimarySortKey(string, begin, length, ignorePunctuation);
			*end = '\0';

			key->UnlockBuffer(end - begin);
			break;
		}

		case B_COLLATE_SECONDARY:
		{
			char *begin = key->LockBuffer(length * 3 + 1);
				// the primary key + the secondary key + separator char

			char *buffer = putPrimarySortKey(string, begin, length, ignorePunctuation);
			*buffer++ = '\01';
				// separator

			input_context context(ignorePunctuation);
			uint32 c;
			for (int32 i = 0; (c = getNextChar(&string, context)) && i < length; i++) {
				if (c < 0x80)
					*buffer++ = tolower(c);
				else
					BUnicodeChar::ToUTF8(BUnicodeChar::ToLower(c), &buffer);
			}
			*buffer = '\0';

			key->UnlockBuffer(buffer - begin);
			break;
		}

		case B_COLLATE_TERTIARY:
		case B_COLLATE_QUATERNARY:
		{
			char *begin = key->LockBuffer(length * 3 + 1);
				// the primary key + the tertiary key + separator char

			char *buffer = putPrimarySortKey(string, begin, length, ignorePunctuation);
			*buffer++ = '\01';
				// separator

			input_context context(ignorePunctuation);
			uint32 c;
			for (int32 i = 0; (c = getNextChar(&string, context)) && i < length; i++) {
				BUnicodeChar::ToUTF8(c, &buffer);
			}
			*buffer = '\0';

			key->UnlockBuffer(buffer + length - begin);
			break;
		}

		case B_COLLATE_IDENTICAL:
		default:
			key->SetTo(string, length);
			break;
	}
}


int 
BCollatorAddOn::Compare(const char *a, const char *b, int32 length, int8 strength,
	bool ignorePunctuation)
{
	if (strength == B_COLLATE_QUATERNARY) {
		// the difference between tertiary and quaternary collation strength
		// are usually a different handling of punctuation characters
		ignorePunctuation = false;
	}

	input_context contextA(ignorePunctuation);
	input_context contextB(ignorePunctuation);

	switch (strength) {
		case B_COLLATE_PRIMARY:
		{
			for (int32 i = 0; i < length; i++) {
				uint32 charA = getNextChar(&a, contextA);
				uint32 charB = getNextChar(&b, contextB);
				if (charA == 0)
					return charB == 0 ? 0 : -(int32)charB;
				else if (charB == 0)
					return (int32)charA;

				charA = getPrimaryChar(charA);
				charB = getPrimaryChar(charB);

				if (charA != charB)
					return (int32)charA - (int32)charB;
			}
			return 0;
		}

		case B_COLLATE_SECONDARY:
		{
			// diacriticals can only change the order between equal strings
			int32 compare = Compare(a, b, length, B_COLLATE_PRIMARY, ignorePunctuation);
			if (compare != 0)
				return compare;

			for (int32 i = 0; i < length; i++) {
				uint32 charA = BUnicodeChar::ToLower(getNextChar(&a, contextA));
				uint32 charB = BUnicodeChar::ToLower(getNextChar(&b, contextB));

				// the two strings does have the same size when we get here
				if (charA == 0)
					return 0;

				if (charA != charB)
					return (int32)charA - (int32)charB;
			}
			return 0;
		}

		case B_COLLATE_TERTIARY:
		case B_COLLATE_QUATERNARY:
		{
			// diacriticals can only change the order between equal strings
			int32 compare = Compare(a, b, length, B_COLLATE_PRIMARY, ignorePunctuation);
			if (compare != 0)
				return compare;

			for (int32 i = 0; i < length; i++) {
				uint32 charA = getNextChar(&a, contextA);
				uint32 charB = getNextChar(&b, contextB);

				// the two strings does have the same size when we get here
				if (charA == 0)
					return 0;

				if (charA != charB)
					return (int32)charA - (int32)charB;
			}
			return 0;
		}

		case B_COLLATE_IDENTICAL:
		default:
			return strncmp(a, b, length);
	}
}


status_t 
BCollatorAddOn::Archive(BMessage *archive, bool deep)
{
	return BArchivable::Archive(archive, deep);
}


BArchivable *
BCollatorAddOn::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BCollatorAddOn"))
		return new BCollator(archive);

	return NULL;
}

