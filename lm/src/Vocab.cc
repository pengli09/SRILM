/*
 * Vocab.cc --
 *	The vocabulary class implementation.
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/Vocab.cc,v 1.23 1998/12/24 19:11:00 stolcke Exp $";
#endif

#include <iostream.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "File.h"
#include "Vocab.h"

#include "LHash.cc"
#include "Array.cc"

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_LHASH(VocabString,VocabIndex);
INSTANTIATE_ARRAY(VocabString);
#endif

Vocab::Vocab(VocabIndex start, VocabIndex end)
    : byIndex(start), nextIndex(start), maxIndex(end)
{
    /*
     * Vocab_None is both the non-index value and the end-token
     * for key sequences used with Tries.  Both need to be
     * the same value so the user or the LM library doesn't have
     * to deal with Map_noKey() and friends.
     */
    assert(Map_noKeyP(Vocab_None));

    /*
     * Make the last Vocab created be the default one used in
     * ostream output
     */
    outputVocab = this;

    /*
     * default is a closed-vocabulary model
     */
    unkIsWord = false;

    /*
     * do not map word strings to lowercase by defaults
     */
    toLower = false;

    /*
     * set some special vocabulary tokens to their defaults
     */
    unkIndex = addWord(Vocab_Unknown);
    ssIndex = addWord(Vocab_SentStart);
    seIndex = addWord(Vocab_SentEnd);
    pauseIndex = addWord(Vocab_Pause);
}

// Compute memory usage
void
Vocab::memStats(MemStats &stats) const
{
    stats.total += sizeof(*this) - sizeof(byName) - sizeof(byIndex);
    byName.memStats(stats);
    byIndex.memStats(stats);
}

// map word string to lowercase
// returns a static buffer
static VocabString
mapToLower(VocabString name)
{
    static char lower[maxWordLength + 1];

    unsigned  i;
    for (i = 0; name[i] != 0 && i < maxWordLength; i++) {
	lower[i] = tolower((unsigned char)name[i]);
    }
    lower[i] = '\0';

    assert(i < maxWordLength);

    return lower;
}

// Add word to vocabulary
VocabIndex
Vocab::addWord(VocabString name)
{
    if (toLower) {
	name = mapToLower(name);
    }

    Boolean found;
    VocabIndex *indexPtr = byName.insert(name, found);

    if (found) {
	return *indexPtr;
    } else {
	if (nextIndex == maxIndex) {
	    return Vocab_None;
	} else {
	    *indexPtr = nextIndex;
	    byIndex[nextIndex] = byName.getInternalKey(name);
	    return nextIndex++;
	}
    } 
}

// Get a word's index by its name
VocabIndex
Vocab::getIndex(VocabString name, VocabIndex unkIndex) const
{
    if (toLower) {
	name = mapToLower(name);
    }

    VocabIndex *indexPtr = byName.find(name);

    return indexPtr ? *indexPtr : unkIndex;
}

// Get a word's name by its index
VocabString
Vocab::getWord(VocabIndex index) const
{
    if (index < byIndex.base() || index >= nextIndex) {
	return 0;
    } else {
	return (*(Array<VocabString>*)&byIndex)[index];	// discard const
    }
}

// Delete a word by name
void
Vocab::remove(VocabString name)
{
    if (toLower) {
	name = mapToLower(name);
    }

    VocabIndex *indexPtr = byName.remove(name);

    if (indexPtr) {
	byIndex[*indexPtr] = 0;

	if (*indexPtr == ssIndex) {
	    ssIndex = Vocab_None;
	}
	if (*indexPtr == seIndex) {
	    seIndex = Vocab_None;
	}
	if (*indexPtr == unkIndex) {
	    unkIndex = Vocab_None;
	}
	if (*indexPtr == pauseIndex) {
	    pauseIndex = Vocab_None;
	}
    }
}

// Delete a word by index
void
Vocab::remove(VocabIndex index)
{
    if (index < byIndex.base() || index >= nextIndex) {
	return;
    } else {
	VocabString name = byIndex[index];
	if (name) {
	    byName.remove(name);
	    byIndex[index] = 0;

	    if (index == ssIndex) {
		ssIndex = Vocab_None;
	    }
	    if (index == seIndex) {
		seIndex = Vocab_None;
	    }
	    if (index == unkIndex) {
		unkIndex = Vocab_None;
	    }
	    if (index == pauseIndex) {
		pauseIndex = Vocab_None;
	    }
	}
    }
}

// Convert index sequence to string sequence
unsigned int
Vocab::getWords(const VocabIndex *wids, VocabString *words,
						    unsigned int max) const
{
    unsigned int i;

    for (i = 0; i < max && wids[i] != Vocab_None; i++) {
	words[i] = getWord(wids[i]);
    }
    if (i < max) {
	words[i] = 0;
    }
    return i;
}

// Convert word sequence to index sequence, adding words if needed
unsigned int
Vocab::addWords(const VocabString *words, VocabIndex *wids, unsigned int max)
{
    unsigned int i;

    for (i = 0; i < max && words[i] != 0; i++) {
	wids[i] = addWord(words[i]);
    }
    if (i < max) {
	wids[i] = Vocab_None;
    }
    return i;
}

// Convert word sequence to index sequence (without adding words)
unsigned int
Vocab::getIndices(const VocabString *words,
		  VocabIndex *wids, unsigned int max,
		  VocabIndex unkIndex) const
{
    unsigned int i;

    for (i = 0; i < max && words[i] != 0; i++) {
	wids[i] = getIndex(words[i], unkIndex);
    }
    if (i < max) {
	wids[i] = Vocab_None;
    }
    return i;
}

// parse strings into words and update stats
unsigned int
Vocab::parseWords(char *sentence, VocabString *words, unsigned int max)
{
    char *word;
    int i = 0;

    for (word = strtok(sentence, wordSeparators);
	 i < max && word != 0;
	 i++, word = strtok(0, wordSeparators))
    {
	words[i] = word;
    }
    if (i < max) {
	words[i] = 0;
    }

    return i;
}

/*
 * Length of Ngrams
 */
unsigned int
Vocab::length(const VocabIndex *words)
{
    unsigned int len = 0;

    while (words[len] != Vocab_None) len++;
    return len;
}

unsigned int
Vocab::length(const VocabString *words)
{
    unsigned int len = 0;

    while (words[len] != 0) len++;
    return len;
}

/*
 * Word containment
 */
Boolean
Vocab::contains(const VocabIndex *words, VocabIndex word)
{
    unsigned i;

    for (i = 0; words[i] != Vocab_None; i++) {
	if (words[i] == word) {
	    return true;
	}
    }
    return false;
}

/*
 * Reversal of Ngrams
 */
VocabIndex *
Vocab::reverse(VocabIndex *words)
{
    int i, j;	/* j can get negative ! */

    for (i = 0, j = length(words) - 1;
	 i < j;
	 i++, j--)
    {
	VocabIndex x = words[i];
	words[i] = words[j];
	words[j] = x;
    }
    return words;
}

VocabString *
Vocab::reverse(VocabString *words)
{
    int i, j;	/* j can get negative ! */

    for (i = 0, j = length(words) - 1;
	 i < j;
	 i++, j--)
    {
	VocabString x = words[i];
	words[i] = words[j];
	words[j] = x;
    }
    return words;
}

/*
 * Output of Ngrams
 */
Vocab *Vocab::outputVocab;	/* vocabulary implicitly used by operator<< */

void
Vocab::write(File &file, const VocabString *words)
{
    for (unsigned int i = 0; words[i] != 0; i++) {
	fprintf(file, "%s%s", (i > 0 ? " " : ""), words[i]);
    }
}

ostream &
operator<< (ostream &stream, const VocabString *words)
{
    for (unsigned int i = 0; words[i] != 0; i++) {
	stream << (i > 0 ? " " : "") << words[i];
    }
    return stream;
}

ostream &
operator<< (ostream &stream, const VocabIndex *words)
{
    for (unsigned int i = 0; words[i] != Vocab_None; i++) {
	VocabString word = Vocab::outputVocab->getWord(words[i]);

	stream << (i > 0 ? " " : "")
	       << (word ? word : "UNKNOWN");
    }
    return stream;
}

/* 
 * Sorting of words and word sequences
 */
Vocab *Vocab::compareVocab;	/* implicit parameter to compare() */

// compare to word indices by their associated word strings
// This should be a non-static member, so we don't have to pass the
// Vocab in a global variable, but then we couldn't use this function
// with qsort() and friends.
int
Vocab::compare(VocabIndex word1, VocabIndex word2)
{
    return strcmp(compareVocab->getWord(word1),
	          compareVocab->getWord(word2));
}

int
Vocab::compare(const VocabString *words1, const VocabString *words2)
{
    unsigned int i = 0;

    for (i = 0; ; i++) {
	if (words1[i] == 0) {
	    if (words2[i] == 0) {
		return 0;
	    } else {
		return -1;	/* words1 is shorter */
	    }
	} else {
	    if (words2[i] == 0) {
		return 1;	/* words2 is shorted */
	    } else {
		int comp = compare(words1[i], words2[i]);
		if (comp != 0) {
		    return comp;	/* they differ as pos i */
		}
	    }
	}
    }
    /*NOTREACHED*/
}

int
Vocab::compare(const VocabIndex *words1, const VocabIndex *words2)
{
    unsigned int i = 0;

    for (i = 0; ; i++) {
	if (words1[i] == 0) {
	    if (words2[i] == 0) {
		return 0;
	    } else {
		return -1;	/* words1 is shorter */
	    }
	} else {
	    if (words2[i] == 0) {
		return 1;	/* words2 is shorted */
	    } else {
		int comp = compare(words1[i], words2[i]);
		if (comp != 0) {
		    return comp;	/* they differ as pos i */
		}
	    }
	}
    }
    /*NOTREACHED*/
}

/*
 * These are convenience methods which set the implicit Vocab used
 * by the comparison functions and returns a pointer to them.
 * Suitable to generate the 'sort' argument used by the iterators.
 */
VocabIndexComparator
Vocab::compareIndex() const
{
    compareVocab = (Vocab *)this;	// discard const
    return compare;
}

VocabIndicesComparator
Vocab::compareIndices() const
{
    compareVocab = (Vocab *)this;	// discard const
    return compare;
}

// Write vocabulary to file
void
Vocab::write(File &file, Boolean sorted) const
{
    VocabIter iter(*this, sorted);
    VocabString word;

    while (!file.error() && (word = iter.next())) {
	fprintf(file, "%s\n", word);
    }
}

// Read vocabulary from file
unsigned int
Vocab::read(File &file)
{
    char *line;
    unsigned int howmany = 0;

    while (line = file.getline()) {
	/*
	 * getline() returns only non-empty lines, so strtok()
	 * will find at least one word.  Any further ones on that
	 * are ignored.
	 */
	VocabString word = strtok(line, wordSeparators);

	if (addWord(word) == Vocab_None) {
	    file.position() << "warning: failed to add " << word
			    << " to vocabulary\n";
	    continue;
	}
	howmany++;
    }
    return howmany;
}

VocabIndex
Vocab::highIndex() const
{
    if (nextIndex == 0) {
	return Vocab_None;
    } else {
	return nextIndex - 1;
    }
}

/*
 * Iteration
 */
VocabIter::VocabIter(const Vocab &vocab, Boolean sorted)
    : myIter(vocab.byName, !sorted ? 0 : strcmp)
{
}

void
VocabIter::init()
{
    myIter.init();
}

VocabString
VocabIter::next(VocabIndex &index)
{
    VocabString word;
    VocabIndex *idx;
    if (idx = myIter.next(word)) {
	index = *idx;
	return word;
    } else {
	return 0;
    }
}
