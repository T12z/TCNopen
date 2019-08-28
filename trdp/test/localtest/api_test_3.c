/**********************************************************************************************************************/
/**
 * @file            api_test_3.c
 *
 * @brief           TRDP test functions on dual interface
 *
 * @details         Extensible test suite working on multihoming / dual interface. Basic functionality and
 *                  regression tests can easily be appended to an array.
 *                  This code is work in progress and can be used verify changes additionally to the standard
 *                  PD and MD tests.
 *
 * @note            Project: TRDP
 *
 * @author          Bernd Loehr
 *
 * @remarks         Copyright NewTec GmbH, 2019.
 *                  All rights reserved.
 *
 * $Id$
 *
 */

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (POSIX)
#include <unistd.h>
#include <sys/select.h>
#elif (defined (WIN32) || defined (WIN64))
#include "getopt.h"
#endif

#include "trdp_if_light.h"
#include "vos_sock.h"
#include "vos_utils.h"
#include "trdp_serviceRegistry.h"
#include "tau_so_if.h"
#include "tau_dnr.h"
#include "tau_tti.h"

#include "tau_xml.h"
#include "vos_shared_mem.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION  "1.0"

typedef int (test_func_t)(void);

UINT32      gDestMC = 0xEF000202u;
int         gFailed;
int         gFullLog = FALSE;

static FILE *gFp = NULL;

typedef struct
{
    TRDP_APP_SESSION_T  appHandle;
    TRDP_IP_ADDR_T      ifaceIP;
    int                 threadRun;
    VOS_THREAD_T        threadIdTxPD;
    VOS_THREAD_T        threadIdRxPD;
    VOS_THREAD_T        threadIdMD;
} TRDP_THREAD_SESSION_T;

TRDP_THREAD_SESSION_T   gSession1 = {NULL, 0x0A000364u, 1, 0, 0, 0};
TRDP_THREAD_SESSION_T   gSession2 = {NULL, 0x0A000365u, 1, 0, 0, 0};

/* Data buffers to play with (Content is borrowed from Douglas Adams, "The Hitchhiker's Guide to the Galaxy") */

static uint8_t          dataBuffer1[(64 * 1024) -1] =
{
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
"Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
"In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
"First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
"But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
"It begins with a house.\n"
"Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
"This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
"And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
"Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
"And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
"Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
"This is not her story.\n"
"But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
"It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
"Nevertheless, a wholly remarkable book.\n"
"In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
"Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
"In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
"First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
"But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
"It begins with a house.\n"
"Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
"This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
"And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
"Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
"And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
"Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
"This is not her story.\n"
"But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
"It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
"Nevertheless, a wholly remarkable book.\n"
"In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
"Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
"In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
"First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
"But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
"It begins with a house.\n"
"Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
"This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
"And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
"Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
"And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
"Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
"This is not her story.\n"
"But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
"It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
"Nevertheless, a wholly remarkable book.\n"
"In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
"Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
"In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
"First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
"But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
"It begins with a house.\n"
"Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
"This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
"And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
"Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
"And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
"Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
"This is not her story.\n"
"But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
"It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
"Nevertheless, a wholly remarkable book.\n"
"In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
"Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
"In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
"First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
"But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
"It begins with a house.\n"
"Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
"This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
"And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
"Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
"And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
"Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
"This is not her story.\n"
"But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
"It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
"Nevertheless, a wholly remarkable book.\n"
"In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
"Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
"In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
"First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
"But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
"It begins with a house.\n"
"Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
"This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
"And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
"Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
"And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
"Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
"This is not her story.\n"
"But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
"It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
"Nevertheless, a wholly remarkable book.\n"
"In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
"Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
"In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
"First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
"But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
"It begins with a house.\n" 
};


static uint8_t          dataBuffer2[(64 * 1024) - 1] =
{
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
};


static uint8_t          dataBuffer3[64] =
{
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static char xmlBuffer[] =
{
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<device xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"trdp-config.xsd\" host-name=\"examplehost\" leader-name=\"leaderhost\" type=\"dummy\">"
    "<device-configuration memory-size=\"65535\">"
    "<mem-block-list>"
    "<mem-block size=\"32\" preallocate=\"512\" />"
    "<mem-block size=\"72\" preallocate=\"256\"/>"
    "<mem-block size=\"128\" preallocate=\"256\"/>"
    "</mem-block-list>"
    "</device-configuration>"
    ""
    "<bus-interface-list>"
    "<bus-interface network-id=\"1\" name=\"enp0s3:1\" host-ip=\"10.0.1.30\">"
    "<trdp-process blocking=\"no\" cycle-time=\"100000\" priority=\"80\" traffic-shaping=\"on\" />"
    "<pd-com-parameter marshall=\"on\" port=\"17224\" qos=\"5\" ttl=\"64\" timeout-value=\"1000000\" validity-behavior=\"zero\" />"
    "<md-com-parameter udp-port=\"17225\" tcp-port=\"17225\""
    "confirm-timeout=\"1000000\" connect-timeout=\"60000000\" reply-timeout=\"5000000\""
    "marshall=\"off\" protocol=\"UDP\" qos=\"3\" retries=\"2\" ttl=\"64\" />"
    "<telegram name=\"tlg1001\" com-id=\"3000\" data-set-id=\"1001\" com-parameter-id=\"1\">"
    "<pd-parameter cycle=\"500000\" marshall=\"off\" timeout =\"3000000\" validity-behavior=\"keep\"/>"
    "<source id=\"1\" uri1=\"239.1.1.2\" >"
    "<sdt-parameter smi1=\"1234\" udv=\"56\" rx-period=\"500\" tx-period=\"2000\" />"
    "</source>"
    "</telegram>"
    "<telegram name=\"tlg1005\" com-id=\"3001\" data-set-id=\"1001\" com-parameter-id=\"1\">"
    "<pd-parameter cycle=\"500000\" marshall=\"off\" timeout =\"3000000\" validity-behavior=\"zero\"/>"
    "<source id=\"1\" uri1=\"239.1.1.2\" />"
    "</telegram>"
    "</bus-interface>"
    "</bus-interface-list>"
    ""
    "<mapped-device-list>"
    "</mapped-device-list>"
    ""
    "<com-parameter-list>"
    "<!--Default PD communication parameters-->"
    "<com-parameter id=\"1\" qos=\"5\" ttl=\"64\" />"
    "<!--Default MD communication parameters-->"
    "<com-parameter id=\"2\" qos=\"3\" ttl=\"64\" />"
    "<!--Own PD communication parameters-->"
    "<com-parameter id=\"4\" qos=\"4\" ttl=\"2\" />"
    "</com-parameter-list>"
    ""
    "<data-set-list>"
    "<data-set name=\"testDS1001\" id=\"1001\">"
    "<element name=\"r32\" type=\"REAL32\"/>"
    "<element name=\"r64\" type=\"REAL64\"/>"
    "</data-set>"
    "</data-set-list>"
    ""
    "<debug file-name=\"trdp.log\" file-size=\"1000000\" info=\"DTFC\" level=\"W\" />"
    "</device>"
};

/**********************************************************************************************************************/
/*  Macro to initialize the library and open two sessions                                                             */
/**********************************************************************************************************************/
#define PREPARE2(a, b, c)                                                           \
    gFailed = 0;                                                                \
    TRDP_ERR_T err = TRDP_NO_ERR;                                               \
    TRDP_APP_SESSION_T appHandle1 = NULL, appHandle2 = NULL;                    \
    {                                                                           \
        gFullLog = FALSE;                                                       \
        fprintf(gFp, "\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
        appHandle1 = test_init(dbgOut, &gSession1, (b), (c));                   \
        if (appHandle1 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
        appHandle2 = test_init(NULL, &gSession2, (b), (c));                     \
        if (appHandle2 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
    }

/**********************************************************************************************************************/
/*  Macro to initialize the library and open two sessions                                                             */
/**********************************************************************************************************************/
#define PREPARE(a, b)                                                           \
    gFailed = 0;                                                                \
    TRDP_ERR_T err = TRDP_NO_ERR;                                               \
    TRDP_APP_SESSION_T appHandle1 = NULL, appHandle2 = NULL;                    \
    {                                                                           \
        gFullLog = FALSE;                                                       \
        fprintf(gFp, "\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
        appHandle1 = test_init(dbgOut, &gSession1, (b), 10000u);                        \
        if (appHandle1 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
        appHandle2 = test_init(NULL, &gSession2, (b), 10000u);                          \
        if (appHandle2 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
    }

/**********************************************************************************************************************/
/*  Macro to initialize the library and open one session                                                              */
/**********************************************************************************************************************/
#define PREPARE1(a)                                                             \
    gFailed = 0;                                                                \
    TRDP_ERR_T err = TRDP_NO_ERR;                                               \
    TRDP_APP_SESSION_T appHandle1 = NULL;                                       \
    {                                                                           \
        gFullLog = FALSE;                                                       \
        fprintf(gFp, "\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
        appHandle1 = test_init(dbgOut, &gSession1, "", 10000u);                 \
        if (appHandle1 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
    }


/**********************************************************************************************************************/
/*  Macro to initialize common function testing                                                                       */
/**********************************************************************************************************************/
#define PREPARE_COM(a)                                                    \
    gFailed = 0;                                                          \
    TRDP_ERR_T err = TRDP_NO_ERR;                                         \
    {                                                                     \
                                                                          \
        printf("\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
                                                                          \
    }



/**********************************************************************************************************************/
/*  Macro to terminate the library and close two sessions                                                             */
/**********************************************************************************************************************/
#define CLEANUP                                                                             \
end:                                                                                        \
    {                                                                                       \
        test_deinit(&gSession1, &gSession2);                                                \
                                                                                            \
        if (gFailed) {                                                                      \
            fprintf(gFp, "\n###########  FAILED!  ###############\nlasterr = %d\n", err); } \
        else{                                                                               \
            fprintf(gFp, "\n-----------  Success  ---------------\n"); }                    \
                                                                                            \
        fprintf(gFp, "--------- End of %s --------------\n\n", __FUNCTION__);               \
                                                                                            \
        return gFailed;                                                                     \
    }

/**********************************************************************************************************************/
/*  Macro to check for error and terminate the test                                                                   */
/**********************************************************************************************************************/
#define IF_ERROR(message)                                                                           \
    if (err != TRDP_NO_ERR)                                                                         \
    {                                                                                               \
        fprintf(gFp, "### %s (error: %d, %s)\n", message, err, vos_getErrorString((VOS_ERR_T)err)); \
        gFailed = 1;                                                                                \
        goto end;                                                                                   \
    }

/**********************************************************************************************************************/
/*  Macro to terminate the test                                                                                       */
/**********************************************************************************************************************/
#define FAILED(message)                \
    fprintf(gFp, "### %s\n", message); \
    gFailed = 1;                       \
    goto end;                          \

/**********************************************************************************************************************/
/*  Macro to terminate the test                                                                                       */
/**********************************************************************************************************************/
#define PRINT(message) \
    fprintf(gFp, "### %s\n", message);

/**********************************************************************************************************************/
/*  Macro to terminate the test                                                                                       */
/**********************************************************************************************************************/
#define FULL_LOG(true_false)     \
    { gFullLog = (true_false); } \

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon            user supplied context pointer
 *  @param[in]        category        Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        lineNumber        line
 *  @param[in]        pMsgStr         pointer to NULL-terminated string
 *  @retval         none
 */
static void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      lineNumber,
    const CHAR8 *pMsgStr)
{
    const char  *catStr[]   = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
    CHAR8       *pF         = strrchr(pFile, VOS_DIR_SEP);

    if (gFullLog || ((category == VOS_LOG_USR) || (category != VOS_LOG_DBG && category != VOS_LOG_INFO)))
    {
        fprintf(gFp, "%s %s %s:%d\t%s",
                strrchr(pTime, '-') + 1,
                catStr[category],
                (pF == NULL) ? "" : pF + 1,
                lineNumber,
                pMsgStr);
    }
    /* else if (strstr(pMsgStr, "vos_mem") != NULL) */
    {
        /* fprintf(gFp, "### %s", pMsgStr); */
    }
}

/*********************************************************************************************************************/
/** Call tlp_processReceive asynchronously
 */
static void *receiverThreadPD (void *pArg)
{
    TRDP_ERR_T  result;
    TRDP_TIME_T interval = {0, 0};
    TRDP_FDS_T  fileDesc;
    INT32       noDesc = 0;

    TRDP_THREAD_SESSION_T *pSession = (TRDP_THREAD_SESSION_T *) pArg;
    /*
     Enter the main processing loop.
     */

    /* vos_printLogStr(VOS_LOG_USR, "receiver thread started...\n"); */

    while (pSession->threadRun &&
           (vos_threadDelay(0u) == VOS_NO_ERR))   /* this is a cancelation point! */
    {
        FD_ZERO(&fileDesc);
        result = tlp_getInterval(pSession->appHandle, &interval, &fileDesc, &noDesc);
        if (result != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "tlp_getInterval failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
        noDesc  = vos_select(noDesc + 1, &fileDesc, NULL, NULL, &interval);
        result  = tlp_processReceive(pSession->appHandle, &fileDesc, &noDesc);
        if ((result != TRDP_NO_ERR) && (result != TRDP_BLOCK_ERR))
        {
            vos_printLog(VOS_LOG_ERROR, "tlp_processReceive failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
    }
    /* vos_printLogStr(VOS_LOG_USR, "...receiver thread stopped\n"); */

    return NULL;
}

/*********************************************************************************************************************/
/** Call tlp_processSend synchronously
 */
static void *senderThreadPD (void *pArg)
{
    TRDP_THREAD_SESSION_T *pSession = (TRDP_THREAD_SESSION_T *) pArg;
    TRDP_ERR_T result = tlp_processSend(pSession->appHandle);
    if ((result != TRDP_NO_ERR) && (result != TRDP_BLOCK_ERR))
    {
        vos_printLog(VOS_LOG_ERROR, "tlp_processSend failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
    }
    return NULL;
}

/*********************************************************************************************************************/
/** Call tlm_process
 */
static void *transceiverThreadMD (void *pArg)
{
    TRDP_ERR_T  result;
    TRDP_TIME_T interval = {0, 0};
    TRDP_FDS_T  fileDesc;
    INT32       noDesc = 0;

    TRDP_THREAD_SESSION_T *pSession = (TRDP_THREAD_SESSION_T *) pArg;
    /*
     Enter the main processing loop.
     */
    while (1/*pSession->threadRun &&
           pSession->threadIdMD &&
           (vos_threadDelay(0u) == VOS_NO_ERR)*/)   /* this is a cancelation point! */
    {
        FD_ZERO(&fileDesc);
        result = tlm_getInterval(pSession->appHandle, &interval, &fileDesc, &noDesc);
        if (result != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "tlm_getInterval failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
        noDesc  = vos_select(noDesc + 1, &fileDesc, NULL, NULL, &interval);
        result  = tlm_process(pSession->appHandle, &fileDesc, &noDesc);
        if ((result != TRDP_NO_ERR) && (result != TRDP_BLOCK_ERR))
        {
            vos_printLog(VOS_LOG_ERROR, "tlm_process failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
    }
    return NULL;
}


/**********************************************************************************************************************/
/* Print a sensible usage message */
static void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("Run defined test suite on a single machine using two application sessions.\n"
           "This version uses separate communication threads for PD and MD.\n"
           "Pre-condition: There must be two IP addresses/interfaces configured and connected by a switch.\n"
           "Arguments are:\n"
           "-o <own IP address> (default 10.0.1.100)\n"
           "-i <second IP address> (default 10.0.1.101)\n"
           "-t <destination MC> (default 239.0.1.1)\n"
           "-m number of test to run (1...n, default 0 = run all tests)\n"
           "-v print version and quit\n"
           "-h this list\n"
           );
}

/**********************************************************************************************************************/
/** common initialisation
 *
 *  @param[in]      dbgout          pointer to logging function
 *  @param[in]      pSession        pointer to session
 *  @param[in]      name            optional name of thread
 *  @retval         application session handle
 */
static TRDP_APP_SESSION_T test_init (
    TRDP_PRINT_DBG_T        dbgout,
    TRDP_THREAD_SESSION_T   *pSession,
    const char              *name,
    UINT32                  cycleTime)
{
    TRDP_ERR_T err = TRDP_NO_ERR;
    pSession->appHandle     = NULL;
    pSession->threadIdRxPD  = 0;
    pSession->threadIdTxPD  = 0;
    pSession->threadIdMD    = 0;
    TRDP_PROCESS_CONFIG_T procConf = {"Test", "me", cycleTime, 0, TRDP_OPTION_NONE};

    /* Initialise only once! */
    if (dbgout != NULL)
    {
        /* for debugging & testing we use dynamic memory allocation (heap) */
        err = tlc_init(dbgout, NULL, NULL);
    }
    if (err == TRDP_NO_ERR)                 /* We ignore double init here */
    {
        err = tlc_openSession(&pSession->appHandle, pSession->ifaceIP, 0u, NULL, NULL, NULL, &procConf);
        /* On error the handle will be NULL... */
    }

    if (err == TRDP_NO_ERR)
    {
        printf("Creating PD Receiver task ...\n");
        /* Receiver thread runs until cancel */
        err = (TRDP_ERR_T) vos_threadCreate(&pSession->threadIdRxPD,
                                            "Receiver Task",
                                            VOS_THREAD_POLICY_OTHER,
                                            (VOS_THREAD_PRIORITY_T) VOS_THREAD_PRIORITY_DEFAULT,
                                            0u,
                                            0u,
                                            (VOS_THREAD_FUNC_T) receiverThreadPD,
                                            pSession);
    }
    if (err == TRDP_NO_ERR)
    {
        printf("Creating PD Sender task with cycle time:\t%uµs\n", procConf.cycleTime);
        /* Send thread is a cyclic thread, runs until cancel */
        err = (TRDP_ERR_T) vos_threadCreate(&pSession->threadIdTxPD,
                                            "Sender Task",
                                            VOS_THREAD_POLICY_OTHER,
                                            (VOS_THREAD_PRIORITY_T) VOS_THREAD_PRIORITY_HIGHEST,
                                            procConf.cycleTime,
                                            0u,
                                            (VOS_THREAD_FUNC_T) senderThreadPD,
                                            pSession);
    }
    if (err == TRDP_NO_ERR)
    {
        /* Receiver thread runs until cancel */
        printf("Creating MD Transceiver task ...\n");
        err = (TRDP_ERR_T) vos_threadCreate(&pSession->threadIdMD,
                                            "Transceiver Task",
                                            VOS_THREAD_POLICY_OTHER,
                                            (VOS_THREAD_PRIORITY_T) VOS_THREAD_PRIORITY_DEFAULT,
                                            0u,
                                            0u,
                                            (VOS_THREAD_FUNC_T) transceiverThreadMD,
                                            pSession);

    }
    if (err != TRDP_NO_ERR)
    {
        printf("Error initing session:\t%d\n", err);
    }
    return pSession->appHandle;
}

/**********************************************************************************************************************/
/** common initialisation
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static void test_deinit (
    TRDP_THREAD_SESSION_T   *pSession1,
    TRDP_THREAD_SESSION_T   *pSession2)
{
    if (pSession1)
    {
        vos_threadTerminate(pSession1->threadIdTxPD);
        vos_threadDelay(100000);
        pSession1->threadIdTxPD = 0;
        vos_threadTerminate(pSession1->threadIdRxPD);
        vos_threadDelay(100000);
        pSession1->threadIdRxPD = 0;
        vos_threadTerminate(pSession1->threadIdMD);
        vos_threadDelay(100000);
        pSession1->threadIdMD = 0;
        tlc_closeSession(pSession1->appHandle);
    }
    if (pSession2)
    {
        vos_threadTerminate(pSession2->threadIdTxPD);
        vos_threadDelay(100000);
        pSession1->threadIdTxPD = 0;
        vos_threadTerminate(pSession2->threadIdRxPD);
        vos_threadDelay(100000);
        pSession1->threadIdRxPD = 0;
        vos_threadTerminate(pSession2->threadIdMD);
        vos_threadDelay(100000);
        pSession1->threadIdMD = 0;
        tlc_closeSession(pSession2->appHandle);
    }
    tlc_terminate();
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/******************************************** Testing starts here *****************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/



/**********************************************************************************************************************/
/** test1 SRM 
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

#define                 TEST1_STRING_COMID         1000u
#define                 TEST1_STRING_REQUEST       (char *)&dataBuffer1 /* "Requesting data" */
#define                 TEST1_STRING_REQUEST_LEN   32  /* "Requesting data len" */
#define                 TEST1_STRING_REPLY         (char *)&dataBuffer2 /* "Data transmission succeded" */
#define                 TEST1_STRING_REPLY_LEN     33    /* "Replying data len" */
#define                 TEST1_64BYTES              (char *)&dataBuffer3 /* "1...15 * 4" */

static void  test1CBFunction (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    TRDP_ERR_T err;
    TRDP_URI_USER_T srcURI  = "12345678901234567890123456789012";   /* 32 chars */
    CHAR8 *localData        = (pData == NULL) ? "empty data" : (CHAR8 *) pData;

    if (pMsg->resultCode == TRDP_REPLYTO_ERR)
    {
        fprintf(gFp, "->> Reply timed out (ComId %u)\n", pMsg->comId);
        gFailed = 1;
    }
    else if ((pMsg->msgType == TRDP_MSG_MR) &&
             (pMsg->comId == TEST1_STRING_COMID))
    {
        if (pMsg->resultCode == TRDP_TIMEOUT_ERR)
        {
            fprintf(gFp, "->> Request timed out (ComId %u)\n", pMsg->comId);
            gFailed = 1;
        }
        else
        {
            fprintf(gFp, "<<- Request received (%.16s...)\n", localData);
/*
              if (memcmp(srcURI, pMsg->srcUserURI, 32u) != 0)
              {
                  gFailed = 1;
                  fprintf(gFp, "### srcUserURI wrong\n");
              }
 */
            fprintf(gFp, "->> Sending reply with query (%.16s)\n", (UINT8 *)TEST1_STRING_REPLY);
            err = tlm_replyQuery(appHandle, &pMsg->sessionId, TEST1_STRING_COMID, 0u, 0u, NULL,
                                 (UINT8 *)TEST1_STRING_REPLY, TEST1_STRING_REPLY_LEN);

            IF_ERROR("tlm_reply");
        }
    }
    else if ((pMsg->msgType == TRDP_MSG_MQ) &&
             (pMsg->comId == TEST1_STRING_COMID))
    {
        fprintf(gFp, "<<- Reply received (%.16s...)\n", localData);
        fprintf(gFp, "->> Sending confirmation\n");
        err = tlm_confirm(appHandle, &pMsg->sessionId, 0u, NULL);

        IF_ERROR("tlm_confirm");
    }
    else if (pMsg->msgType == TRDP_MSG_MC)
    {
        fprintf(gFp, "<<- Confirmation received (status = %u)\n", pMsg->userStatus);
    }
    else if ((pMsg->msgType == TRDP_MSG_MN) &&
             (pMsg->comId == TEST1_STRING_COMID))
    {
        if (strlen((char *)pMsg->sessionId) > 0)
        {
            gFailed = 1;
            fprintf(gFp, "#### ->> Notification received, sessionID = %16s\n", pMsg->sessionId);
        }
        else
        {
            gFailed = 0;
            fprintf(gFp, "->> Notification received, sessionID == 0\n");
        }
    }
    else
    {
        fprintf(gFp, "<<- Unsolicited Message received (type = %0xhx)\n", pMsg->msgType);
        gFailed = 1;
    }
end:
    return;
}


static int test1 ()
{
    PREPARE1("SRM offer"); /* allocates appHandle1, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        int i;
        
        gFullLog = TRUE;
        UINT16                 noOfServices = 1;
        SRM_SERVICE_ARRAY_T     servicesToAdd;

        memset(&servicesToAdd, 0, sizeof(servicesToAdd));
        servicesToAdd.version.ver   = 1;
        servicesToAdd.version.rel   = 0;
        servicesToAdd.noOfEntries   = vos_htons(1u);
        servicesToAdd.serviceEntry[0].flags         = 0u;
        servicesToAdd.serviceEntry[0].instanceId    = 0u;
        servicesToAdd.serviceEntry[0].serviceTypeId = vos_htonl(10001u);
        vos_strncpy(servicesToAdd.serviceEntry[0].serviceName, "testFakeNewService", 32);
        vos_strncpy(servicesToAdd.serviceEntry[0].serviceURI, vos_ipDotted(gSession1.ifaceIP), 80);
        servicesToAdd.serviceEntry[0].destMCIP = vos_htonl(0u);
        servicesToAdd.serviceEntry[0].reserved = 0u;
        vos_strncpy(servicesToAdd.serviceEntry[0].hostname, "ccu-o", 80);
        servicesToAdd.serviceEntry[0].machineIP = vos_htonl(gSession1.ifaceIP);
//        servicesToAdd.serviceEntry[0].timeToLive = {0,0};
//        servicesToAdd.serviceEntry[0].lastUpdated = {0,0};
//        servicesToAdd.serviceEntry[0].timeSlot = {0,0};
//        {
//            TRDP_SHORT_VERSION_T    version;        /**< 1.0 service version                */
//            BITSET8                 flags;          /**< 0x01 | 0x02 == Safe Service        */
//            UINT8                   instanceId;     /**< 8 Bit relevant                     */
//            UINT32                  serviceTypeId;  /**< lower 24 Bit relevant              */
//            CHAR8                   serviceName[32];/**< name of the service                */
//            CHAR8                   serviceURI[80]; /**< destination URI for services       */
//            TRDP_IP_ADDR_T          destMCIP;       /**< destination multicast for services */
//            UINT32                  reserved;
//            CHAR8                   hostname[80];   /**< device name - FQN (optional)       */
//            TRDP_IP_ADDR_T          machineIP;      /**< current IP address of service host */
//            TIMEDATE64              timeToLive;     /**< when to check for life sign        */
//            TIMEDATE64              lastUpdated;    /**< last time seen  (optional)         */
//            TIMEDATE64              timeSlot;       /**< timeslot for TSN (optional)        */
//        } GNU_PACKED SRM_SERVICE_REGISTRY_ENTRY;

        /* Preliminary definition of Request/Reply (DSID 113) */
//        typedef struct
//        {
//            TRDP_SHORT_VERSION_T        version;        /**< 1.0 telegram version           */
//            UINT16                      noOfEntries;    /**< number of entries in array     */
//            SRM_SERVICE_REGISTRY_ENTRY  serviceEntry[1];/**< var. number of entries         */
//            TRDP_SDTv2_T                safetyTrail;    /**< opt. SDT trailer               */
//        } GNU_PACKED SRM_SERVICE_ARRAY_T;

        /* We need DNR services! */
        err = tau_initDnr (appHandle1, 0u, 0u,
                               "hostsfile.txt",
                           TRDP_DNR_COMMON_THREAD, TRUE);
        IF_ERROR("tau_initDnr");
        /* add the service */
        err = tau_addServices (appHandle1, noOfServices, &servicesToAdd, TRUE);
        IF_ERROR("tau_addServices");

        /* Wait a second */
        vos_threadDelay(1000000u);

        /* list the services */
        SRM_SERVICE_ARRAY_T *pServicesToAdd = NULL;
        err = tau_getServiceList (appHandle1, &pServicesToAdd);
        IF_ERROR("tau_getServiceList");
        noOfServices = pServicesToAdd->noOfEntries;
        for (int i = 0; i < noOfServices; i++)
        {
            vos_printLog(VOS_LOG_USR, "[%d] Service name: %s, SrvId: %u, InstId: %u, host: %s, IP: %s\n",
                         i,
                         pServicesToAdd->serviceEntry[i].serviceName,
                         pServicesToAdd->serviceEntry[i].serviceTypeId,
                         pServicesToAdd->serviceEntry[i].instanceId,
                         pServicesToAdd->serviceEntry[i].hostname,
                         vos_ipDotted(pServicesToAdd->serviceEntry[i].machineIP));
        }
        tau_freeServiceList(pServicesToAdd);
    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}


/**********************************************************************************************************************/
/* This array holds pointers to the m-th test (m = 1 will execute test1...)                                           */
/**********************************************************************************************************************/
test_func_t *testArray[] =
{
    NULL,
    test1,  /* SRM test 1 */
    NULL
};

/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main (int argc, char *argv[])
{
    int ch;
    unsigned int ip[4];
    UINT32 testNo = 0;

    if (gFp == NULL)
    {
        /* gFp = fopen("/tmp/trdp.log", "w+"); */
        gFp = stdout;
    }

    while ((ch = getopt(argc, argv, "d:i:t:o:m:h?v")) != -1)
    {
        switch (ch)
        {
            case 'o':
            {   /*  read primary ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gSession1.ifaceIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'i':
            {   /*  read alternate ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gSession2.ifaceIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 't':
            {   /*  read target ip (MC)   */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gDestMC = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'm':
            {   /*  read test number    */
                if (sscanf(optarg, "%u",
                           &testNo) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
            }
            case 'v':   /*  version */
                printf("%s: Version %s\t(%s - %s)\n",
                       argv[0], APP_VERSION, __DATE__, __TIME__);
                printf("No. of tests: %lu\n", sizeof(testArray) / sizeof(void *) - 2);
                exit(0);
                break;
            case 'h':
            case '?':
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (testNo > (sizeof(testArray) / sizeof(void *) - 1))
    {
        printf("%s: test no. %u does not exist\n", argv[0], testNo);
        exit(1);
    }

    if (testNo == 0)    /* Run all tests in sequence */
    {
        while (1)
        {
            int i, ret = 0;
            for (i = 1; testArray[i] != NULL; ++i)
            {
                ret += testArray[i]();
            }
            if (ret == 0)
            {
                fprintf(gFp, "All tests passed!\n");
            }
            else
            {
                fprintf(gFp, "### %d test(s) failed! ###\n", ret);
            }
            return ret;
        }
    }

    return testArray[testNo]();
}
