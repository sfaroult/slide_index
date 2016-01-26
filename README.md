# slide_index
A utility for generating an index for Powerpoint slides

IMPORTANT: This utilitity requires miniz (https://code.google.com/archive/p/miniz/) as well as SQLite (which is usually installed on any Mac/Linux machine).

I've written this utility for the benefit of my students while teaching at Kansas State University - Course notes for my classes consist of 4-slide per page handouts of the slides used during lectures. Slides for handouts are modified to make animations legible in a PDF file, and contain added annotations, as I avoid having too much text (other than programs, as I teach CS) on the slides I present, and as I want to make sure that my students have all the important information; I don't doubt their note-taking abilities, but I prefer them to listen and watch rather than try to take everything I say in notes. As I average one slide per minute, that's a lot of slides at the end of the term and remembering during which lecture this important topic was discussed may be challenging when reviewing before exams.
The idea of this tool is to have something far superior to full-text search.
I have only tested it on my Mac; it should run without any problem on any Linux.

There are different ways to use this tool:
1) The preferred way is to 'tag' slides. It requires a bit of preparation but allows to generate a "quality" index. Tags must be inserted in the slide notes between square brackets:
  [tag1;tag2; ...;tagn]
Each tag will become an index entry. Note that tags aren't necessarily a single word, but can be a full expression. They simply must'nt contain the tag separator, which is ';' by default but can be changed.
Example:
    [Bonaparte, Napoleon;Napoleon;French Revolution]
This slide will be indexed under
      Bonaparte, Napoleon
      French Revolution
  and
      Napoleon
2) The other option is to read the words (not expressions) to index from an external text file and to compare them to words found either in the slide or in the notes. Each line in the file of words to index must contain either a single word, or a set of words separated by the same separator as the tag separator above. When the line contains several words, they are understood as variants (singular/plural for instance) and only the first one will appear in the index. For instance, if you have this line:
   Napoleon;Bonaparte
Bonaparte is understood as a variant of Napoleon which will be the index entry. There will be no entry index for Bonaparte, but all slides containing Bonaparte will be indexed under Napoleon.

Slide decks are searched for the words to index in slide text, in slide notes, and also in the name of image files that are included in the files. Note that the search isn't case sensitive, but index entries follow the capitalization found in the file of words to index.

To help with generating a list of words susceptible of appearing in an index, the program can also generate a list of words on demand, by analyzing the slides. It's possible to specify a list of "stop words" (a list of English stop words found on the internet is provided). Words that appear too often are eliminated. This automatically generated list should of course be edited before generating the final index.

When generating the index, there are also several options:
- By default, the index entries are composed of the name of the slide deck, followed by a list of slide numbers. Personally, I provide handouts with 4 slides per page. There is a -p option followed by the number of slides per page that generates an index with the ultimate page rather than slide number (but remember that was is index is a set of .pptx, not .pdf, slides)
- By default again, what is generated is a text file. A -r option allows to generate instead a Rich Text Format (.rtf) file instead. With a rich text format, there is the possibility of making a difference between keywords (which are important in my discipline) and other words. By default (it can be disabled), a word that is all in lower case will be understood to be a special keyword and its index entry will be set in a monospace font and in bold. Alternatively, you can also define a special character (for instance @) as indicative of something that must be set in monospace and bold, and any word prefixed by the defined chracter will appear as such in the index.

The program writes its output to the standard output, which must be redirected. It also writes some informative messages (number of slides in eac .pptx file processed, for example) to the standard error.
 
USAGE
slide_index [flags] <pptx file> [<pptx file> ...]
 Flags:
  -d            : Disable the "auto-keyword" mode. By default, all-lowercase words are understood as keywords
                  and displayed differently in an RTF index.
  -I <filename> : Read words to index from <filename>.
                  Each line can contain several variants of the same word, separated by a character that
                  defaults to a semi-colon. The first entry on the line is the one that appears in the index.
  -k <char>     : Prefix for keywords (displayed differently in an RTF index); must be a single character.
  -p <num>      : By default the slide number appear in the index. If handouts contain multiple slides per page,
                  the -p flag followed by the number of slides per page will list the page number in the index.
  -r            : Generate an RTF (Rich-Text-Format) index instead of a plain-text one.
  -s <char>     : Change the separator from ';' to <char>
  -S <filename> : Read words that are NOT to index (stop words) from <filename>. There must be only one word per
                  line and no stemming is performed (all variants must be explicitly listed in the file)
  -t            : Exclusively index from tags in slide notes.

EXAMPLES
  Generate lecture notes index as an RTF file from tagged slides; Words starting with @ in tags will be displayed as keywords:
  slide_index -t -k '@' $HOME/CIS209/HANDOUTS/*.pptx > LectureIndex.rtf
  
  Generate lecture notes index as a text file using a file of words that should appear in the index:
  slide_index -I words_to_index.txt $HOME/CIS209/HANDOUTS/*.pptx > LectureIndex.txt
  
  Generate a list of words (to edit) that could be used to generate the index. Ignore words in stop_words.txt:
  slide_index -S stop_words.txt $HOME/CIS209/HANDOUTS/*.pptx > words_to_index.txt
  
