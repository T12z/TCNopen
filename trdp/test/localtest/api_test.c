/**********************************************************************************************************************/
/**
 * @file            api_test.c
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
 * @remarks         Copyright NewTec GmbH, 2017-2020.
 *                  All rights reserved.
 *
 * $Id$
 *
 *      BL 2019-08-27: Interval timing in test 9 changed
 *      BL 2018-03-06: Ticket #101 Optional callback function on PD send
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
    VOS_THREAD_T        threadId;

} TRDP_THREAD_SESSION_T;

TRDP_THREAD_SESSION_T   gSession1 = {NULL, 0x0A000364u, 0, 0};
TRDP_THREAD_SESSION_T   gSession2 = {NULL, 0x0A000365u, 0, 0};

/* Data buffers to play with (Content is borrowed from Douglas Adams, "The Hitchhiker's Guide to the Galaxy") */
static uint8_t          dataBuffer1[64 * 1024] =
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

static uint8_t          dataBuffer2[64 * 1024] =
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

static char          xmlBuffer[] =
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
#define PREPARE(a, b)                                                           \
    gFailed = 0;                                                                \
    TRDP_ERR_T err = TRDP_NO_ERR;                                               \
    TRDP_APP_SESSION_T appHandle1 = NULL, appHandle2 = NULL;                    \
    {                                                                           \
        gFullLog = FALSE;                                                       \
        fprintf(gFp, "\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
        appHandle1 = test_init(dbgOut, &gSession1, (b));                        \
        if (appHandle1 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
        appHandle2 = test_init(NULL, &gSession2, (b));                          \
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
        appHandle1 = test_init(dbgOut, &gSession1, "");                         \
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
        fprintf(gFp, "\n-------- Cleaning up %s ----------\n", __FUNCTION__);               \
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
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
    CHAR8       *pF = strrchr(pFile, VOS_DIR_SEP);

    if (gFullLog || ((category == VOS_LOG_USR) || (category != VOS_LOG_DBG && category != VOS_LOG_INFO)))
    {
        fprintf(gFp, "%s %s %s:%d %s",
                strrchr(pTime, '-') + 1,
                catStr[category],
                (pF == NULL)? "" : pF + 1,
                lineNumber,
                pMsgStr);
    }
    /* else if (strstr(pMsgStr, "vos_mem") != NULL) */
    {
        /* fprintf(gFp, "### %s", pMsgStr); */
    }
}

/**********************************************************************************************************************/
/** trdp processing loop (thread)
 *
 *  @param[in]      pArg            user supplied context pointer
 *
 *  @retval         none
 */
static void trdp_loop (void *pArg)
{
    TRDP_THREAD_SESSION_T *pSession = (TRDP_THREAD_SESSION_T *) pArg;
    /*
        Enter the main processing loop.
     */
    while (pSession->threadId)
    {
        TRDP_FDS_T  rfds;
        INT32       noDesc;
        INT32       rv;
        TRDP_TIME_T tv;
        TRDP_TIME_T max_tv  = {0u, 20000};
        TRDP_TIME_T min_tv  = {0u, 5000};

        /*
         Prepare the file descriptor set for the select call.
         Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);

        /*
         Compute the min. timeout value for select.
         This way we can guarantee that PDs are sent in time
         with minimum CPU load and minimum jitter.
         */
        (void)tlc_getInterval(pSession->appHandle, &tv, (TRDP_FDS_T *) &rfds, &noDesc);

        /*
         The wait time for select must consider cycle times and timeouts of
         the PD packets received or sent.
         If we need to poll something faster than the lowest PD cycle,
         we need to set the maximum time out our self.
         */
        if (vos_cmpTime(&tv, &max_tv) > 0)
        {
            tv = max_tv;
        }

        if (vos_cmpTime(&tv, &min_tv) < 0)
        {
            tv = min_tv;
        }
        /*
         Select() will wait for ready descriptors or time out,
         what ever comes first.
         */
        rv = vos_select(noDesc + 1, &rfds, NULL, NULL, &tv);

        /*
         Check for overdue PDs (sending and receiving)
         Send any pending PDs if it's time...
         Detect missing PDs...
         'rv' will be updated to show the handled events, if there are
         more than one...
         The callback function will be called from within the tlc_process
         function (in it's context and thread)!
         */
        (void) tlc_process(pSession->appHandle, &rfds, &rv);

    }

    /*
     *    We always clean up behind us!
     */

    (void)tlc_closeSession(pSession->appHandle);
    pSession->appHandle = NULL;
}

/**********************************************************************************************************************/
/* Print a sensible usage message */
static void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("Run defined test suite on a single machine using two application sessions.\n"
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
    const char              *name)
{
    TRDP_ERR_T err = TRDP_NO_ERR;
    pSession->appHandle = NULL;

    if (dbgout != NULL)
    {
        /* for debugging & testing we use dynamic memory allocation (heap) */
        err = tlc_init(dbgout, NULL, NULL);
    }
    if (err == TRDP_NO_ERR)                 /* We ignore double init here */
    {
        tlc_openSession(&pSession->appHandle, pSession->ifaceIP, 0u, NULL, NULL, NULL, NULL);
        /* On error the handle will be NULL... */
    }

    if (err == TRDP_NO_ERR)
    {
        (void) vos_threadCreate(&pSession->threadId, name, VOS_THREAD_POLICY_OTHER, 0u, 0u, 0u,
                                trdp_loop, pSession);
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
        //pSession1->threadRun = 0;
        vos_threadTerminate(pSession1->threadId);
        vos_threadDelay(100000);
    }
    if (pSession2)
    {
        //pSession2->threadRun = 0;
        vos_threadTerminate(pSession2->threadId);
        vos_threadDelay(100000);
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
/** PD publish and subscribe
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test1 ()
{
    PREPARE("Basic PD publish and subscribe, polling (#128 ComId = 0)", "test"); /* allocates appHandle1, appHandle2,
                                                                                   failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T  pubHandle;
        TRDP_SUB_T  subHandle;

#define TEST1_COMID     0u
#define TEST1_INTERVAL  100000u
#define TEST1_DATA      "Hello World!"
#define TEST1_DATA_LEN  24u

        /*    Copy the packet into the internal send queue, prepare for sending.    */

        err = tlp_publish(gSession1.appHandle, &pubHandle, NULL, NULL,  0u, TEST1_COMID, 0u, 0u,
                          0u, /* gSession1.ifaceIP,                   / * Source * / */
                          gSession2.ifaceIP, /* gDestMC,               / * Destination * / */
                          TEST1_INTERVAL,
                          0u, TRDP_FLAGS_DEFAULT, NULL, NULL, TEST1_DATA_LEN);

        IF_ERROR("tlp_publish");

        err = tlp_subscribe(gSession2.appHandle, &subHandle, NULL, NULL, 0u,
                            TEST1_COMID, 0u, 0u,
                            0u, 0u, /* gSession1.ifaceIP,                  / * Source * / */
                            0u, /* gDestMC,                            / * Destination * / */
                            TRDP_FLAGS_DEFAULT,
                            NULL,                      /*    default interface                    */
                            TEST1_INTERVAL * 3, TRDP_TO_DEFAULT);


        IF_ERROR("tlp_subscribe");

        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter < 50)         /* 5 seconds */
        {
            char    data1[1432u];
            char    data2[1432u];
            UINT32  dataSize2 = sizeof(data2);
            TRDP_PD_INFO_T pdInfo;

            sprintf(data1, "Just a Counter: %08d", counter++);

            err = tlp_put(gSession1.appHandle, pubHandle, (UINT8 *) data1, (UINT32) strlen(data1));
            IF_ERROR("tap_put");

            vos_threadDelay(100000);

            err = tlp_get(gSession2.appHandle, subHandle, &pdInfo, (UINT8 *) data2, &dataSize2);


            if (err == TRDP_NODATA_ERR)
            {
                continue;
            }

            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_INFO, "### tlp_get error: %s\n", vos_getErrorString((VOS_ERR_T)err));

                gFailed = 1;
                /* goto end; */

            }
            else
            {
                if (memcmp(data1, data2, dataSize2) == 0)
                {
                    fprintf(gFp, "received data matches (seq: %u, size: %u)\n", pdInfo.seqCount, dataSize2);
                }
            }
        }
    }


    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test2
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

static void test2PDcallBack (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    void *pSentdata = (void *) pMsg->pUserRef;

    /*    Check why we have been called    */
    switch (pMsg->resultCode)
    {
       case TRDP_NO_ERR:
           if ((pSentdata != NULL) && (pData != NULL) && (memcmp(pData, pSentdata, dataSize) == 0))
           {
               fprintf(gFp, "received data matches (seq: %u, size: %u)\n", pMsg->seqCount, dataSize);
           }
           break;

       case TRDP_TIMEOUT_ERR:
           /* The application can decide here if old data shall be invalidated or kept    */
           fprintf(gFp, "Packet timed out (ComId %d, SrcIP: %s)\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->srcIpAddr));
           break;
       default:
           fprintf(gFp, "Error on packet received (ComId %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
           break;
    }
}

static int test2 ()
{
    PREPARE("Publish & Subscribe, Callback", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T  pubHandle;
        TRDP_SUB_T  subHandle;
        char        data1[1432u];

#define TEST2_COMID     1000u
#define TEST2_INTERVAL  100000u
#define TEST2_DATA      "Hello World!"

        /*    Copy the packet into the internal send queue, prepare for sending.    */

        err = tlp_publish(gSession1.appHandle, &pubHandle, NULL, NULL,  0u, TEST2_COMID, 0u, 0u,
                          0u, /* gSession1.ifaceIP,                   / * Source * / */
                          gSession2.ifaceIP, /* gDestMC,               / * Destination * / */
                          TEST2_INTERVAL,
                          0u, TRDP_FLAGS_DEFAULT, NULL, NULL, 0u);

        IF_ERROR("tlp_publish");

        err = tlp_subscribe(gSession2.appHandle, &subHandle, data1, test2PDcallBack, 0u,
                            TEST2_COMID, 0u, 0u,
                            0u, 0u, /* gSession1.ifaceIP,                  / * Source * / */
                            0u, /* gDestMC,                            / * Destination * / */
                            TRDP_FLAGS_CALLBACK,
                            NULL,                      /*    default interface                    */
                            TEST2_INTERVAL * 3, TRDP_TO_DEFAULT);


        IF_ERROR("tlp_subscribe");

        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter < 5)         /* 0.5 seconds */
        {

            sprintf(data1, "Just a Counter: %08d", counter++);

            err = tlp_put(gSession1.appHandle, pubHandle, (UINT8 *) data1, (UINT32) strlen(data1));
            IF_ERROR("tap_put");

            usleep(100000u);

        }
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test3 tlp_get
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test3 ()
{
    PREPARE("Ticket #140: tlp_get reports immediately TRDP_TIMEOUT_ERR", "test"); /* allocates appHandle1, appHandle2,
                                                                                    failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_SUB_T subHandle;

#define TEST3_COMID     1000u
#define TEST3_INTERVAL  100000u


        err = tlp_subscribe(gSession2.appHandle, &subHandle, NULL, NULL, 0u,
                            TEST3_COMID, 0u, 0u,
                            0u, 0u, /* gSession1.ifaceIP,                  / * Source * / */
                            0u, /* gDestMC,                            / * Destination * / */
                            TRDP_FLAGS_DEFAULT,
                            NULL,                      /*    default interface                    */
                            TRDP_INFINITE_TIMEOUT, TRDP_TO_DEFAULT);


        IF_ERROR("tlp_subscribe");

        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter++ < 50)         /* 5 seconds */
        {
            char    data2[1432u];
            UINT32  dataSize2 = sizeof(data2);
            TRDP_PD_INFO_T pdInfo;

            usleep(TEST3_INTERVAL);

            err = tlp_get(gSession2.appHandle, subHandle, &pdInfo, (UINT8 *) data2, &dataSize2);
            if (err == TRDP_NODATA_ERR)
            {
                fprintf(gFp, ".");
                fflush(gFp);
                continue;
            }

            if (err != TRDP_NO_ERR)
            {
                fprintf(gFp, "\n### tlp_get error: %d\n", err);

                gFailed = 1;
                goto end;

            }

        }
        fprintf(gFp, "\n");
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test4 PD PULL Request
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test4 ()
{
    PREPARE("#153 (two PDs on one pull request", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        {
            TRDP_PUB_T  pubHandle;
            TRDP_SUB_T  subHandle;

#define TEST4_COMID     1000u
#define TEST4_INTERVAL  100000u
#define TEST4_DATA      "Hello World!"
#define TEST4_DATA_LEN  16u

            /*    Session1 Install subscriber and publisher for PULL (interval = 0).    */

            err = tlp_subscribe(gSession1.appHandle, &subHandle, NULL, NULL, 0u,
                                TEST4_COMID, 0u, 0u,
                                0u, 0u,
                                gDestMC,                            /* MC group */
                                TRDP_FLAGS_NONE,
                                NULL,                      /*    default interface                    */
                                0, TRDP_TO_DEFAULT);


            IF_ERROR("tlp_subscribe");

            err = tlp_publish(gSession1.appHandle, &pubHandle, NULL, NULL,  0u, TEST4_COMID, 0u, 0u,
                              0u,
                              gDestMC,                              /* Destination */
                              0u,
                              0u, TRDP_FLAGS_DEFAULT, NULL, (UINT8 *) TEST4_DATA, TEST4_DATA_LEN);

            IF_ERROR("tlp_publish");

            /*    Session2 Install subscriber and do a request for PULL.    */

            err = tlp_subscribe(gSession2.appHandle, &subHandle, NULL, NULL, 0u,
                                TEST4_COMID, 0u, 0u,
                                0u, 0u,                                 /* Source */
                                gDestMC,                            /* Destination */
                                TRDP_FLAGS_DEFAULT,
                                NULL,                      /*    default interface                    */
                                TEST4_INTERVAL * 3, TRDP_TO_DEFAULT);


            IF_ERROR("tlp_subscribe");

            err = tlp_request(gSession2.appHandle, subHandle, 0u,
                              TEST4_COMID, 0u, 0u, gSession2.ifaceIP, gSession1.ifaceIP,
                              0u, TRDP_FLAGS_NONE, NULL, NULL, 0u, TEST4_COMID, gDestMC);

            IF_ERROR("tlp_request");

            /*
             Enter the main processing loop.
             */
            int counter = 0;
            while (counter++ < 50)         /* 5 seconds */
            {
                char    data2[1432u];
                UINT32  dataSize2 = sizeof(data2);
                TRDP_PD_INFO_T pdInfo;

                usleep(100000u);

                err = tlp_get(gSession2.appHandle, subHandle, &pdInfo, (UINT8 *) data2, &dataSize2);
                if (err == TRDP_NODATA_ERR)
                {
                    continue;
                }

                if (err == TRDP_TIMEOUT_ERR)
                {
                    continue;
                }

                if (err != TRDP_NO_ERR)
                {
                    fprintf(gFp, "### tlp_get error: %d\n", err);

                    gFailed = 1;
                    goto end;

                }
                else
                {
                    fprintf(gFp, "received data from pull: %s (seq: %u, size: %u)\n", data2, pdInfo.seqCount, dataSize2);
                    gFailed = 0;
                    goto end;
                }
            }
        }
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test5 MD Request - Reply - Confirm
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

#define                 TEST5_STRING_COMID      1000u
#define                 TEST5_STRING_REQUEST    (char *)&dataBuffer1 /* "Requesting data" */
#define                 TEST5_STRING_REPLY      (char *)&dataBuffer2 /* "Data transmission succeded" */

static void  test5CBFunction (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    TRDP_ERR_T err;
    TRDP_URI_USER_T srcURI = "12345678901234567890123456789012";    /* 32 chars */

    if (pMsg->resultCode == TRDP_REPLYTO_ERR)
    {
        fprintf(gFp, "->> Reply timed out (ComId %u)\n", pMsg->comId);
        gFailed = 1;
    }
    else if ((pMsg->msgType == TRDP_MSG_MR) &&
             (pMsg->comId == TEST5_STRING_COMID))
    {
        if (pMsg->resultCode == TRDP_TIMEOUT_ERR)
        {
            fprintf(gFp, "->> Request timed out (ComId %u)\n", pMsg->comId);
            gFailed = 1;
        }
        else
        {
            /* fprintf(gFp, "->> Request received (%s)\n", pData); */
            if (memcmp(srcURI, pMsg->srcUserURI, 32u) != 0)
            {
                gFailed = 1;
                fprintf(gFp, "## srcUserURI wrong\n");
            }
            fprintf(gFp, "->> Sending reply\n");
            err = tlm_replyQuery(appHandle, &pMsg->sessionId, TEST5_STRING_COMID, 0u, 500000u, NULL,
                                 (UINT8 *)TEST5_STRING_REPLY, 63 * 1024 /*strlen(TEST5_STRING_REPLY)*/, NULL);

            IF_ERROR("tlm_reply");
        }
    }
    else if ((pMsg->msgType == TRDP_MSG_MQ) &&
             (pMsg->comId == TEST5_STRING_COMID))
    {
        fprintf(gFp, "->> Reply received (%s)\n", pData);
        fprintf(gFp, "->> Sending confirmation\n");
        err = tlm_confirm(appHandle, &pMsg->sessionId, 0u, NULL);

        IF_ERROR("tlm_confirm");
    }
    else if (pMsg->msgType == TRDP_MSG_MC)
    {
        fprintf(gFp, "->> Confirmation received (status = %u)\n", pMsg->userStatus);
    }
    else if ((pMsg->msgType == TRDP_MSG_MN) &&
             (pMsg->comId == TEST5_STRING_COMID))
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
        fprintf(gFp, "->> Unsolicited Message received (type = %0xhx)\n", pMsg->msgType);
        gFailed = 1;
    }
end:
    return;
}

static int test5 ()
{
    PREPARE("TCP MD Request - Reply - Confirm, #149, #160", "test"); /* allocates appHandle1, appHandle2, failed = 0,
                                                                       err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_UUID_T sessionId1;
        TRDP_LIS_T listenHandle;
        TRDP_URI_USER_T destURI1    = "12345678901234567890123456789012";    /* 32 chars */
        TRDP_URI_USER_T destURI2    = "12345678901234567890123456789012";    /* 32 chars */
        TRDP_URI_USER_T srcURI      = "12345678901234567890123456789012";    /* 32 chars */

        err = tlm_addListener(appHandle2, &listenHandle, NULL, test5CBFunction,
                              TRUE,
                              TEST5_STRING_COMID, 0u, 0u, 0u,
                              VOS_INADDR_ANY, VOS_INADDR_ANY,
                              TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP, NULL, destURI1);
        IF_ERROR("tlm_addListener1");
        fprintf(gFp, "->> MD TCP Listener1 set up\n");

        err = tlm_request(appHandle1, NULL, test5CBFunction, &sessionId1,
                          TEST5_STRING_COMID, 0u, 0u,
                          0u, gSession2.ifaceIP,
                          TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP, 1u, 1000000u, NULL,
                          (UINT8 *)TEST5_STRING_REQUEST, 63 * 1024 /*strlen(TEST5_STRING_REQUEST)*/,
                          srcURI, destURI2);

        IF_ERROR("tlm_request1");
        fprintf(gFp, "->> MD TCP Request1 sent\n");

        vos_threadDelay(2000000u);


        err = tlm_request(appHandle1, NULL, test5CBFunction, &sessionId1,
                          TEST5_STRING_COMID, 0u, 0u,
                          0u, gSession2.ifaceIP,
                          TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP, 1u, 1000000u, NULL,
                          (UINT8 *)TEST5_STRING_REQUEST, 63 * 1024 /*strlen(TEST5_STRING_REQUEST)*/,
                          srcURI, destURI2);

        IF_ERROR("tlm_request2");
        fprintf(gFp, "->> MD TCP Request2 sent\n");

        vos_threadDelay(2000000u);

        err = tlm_delListener(appHandle2, listenHandle);
        IF_ERROR("tlm_delListener2");
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test6 (extension of test5, should fail)
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test6 ()
{
    PREPARE("UDP MD Request - Reply - Confirm, #149", "test"); /* allocates appHandle1, appHandle2, failed = 0, err =
                                                                 TRDP_NO_ERR */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_UUID_T sessionId1;
        TRDP_LIS_T listenHandle;
        TRDP_URI_USER_T destURI1    = "12345678901234567890123456789012";    /* 32 chars */
        TRDP_URI_USER_T destURI2    = "1234567890123456789012345678901";    /* 32 chars */
        TRDP_URI_USER_T srcURI      = "12345678901234567890123456789012";    /* 32 chars */

        err = tlm_addListener(appHandle2, &listenHandle, NULL, test5CBFunction,
                              TRUE,
                              TEST5_STRING_COMID, 0u, 0u, 0u,
                              VOS_INADDR_ANY, VOS_INADDR_ANY, TRDP_FLAGS_CALLBACK, NULL, destURI1);
        IF_ERROR("tlm_addListener");
        fprintf(gFp, "->> MD Listener set up\n");

        err = tlm_request(appHandle1, NULL, test5CBFunction, &sessionId1,
                          TEST5_STRING_COMID, 0u, 0u,
                          0u, gSession2.ifaceIP,
                          TRDP_FLAGS_CALLBACK, 1u, 1000000u, NULL,
                          (UINT8 *)TEST5_STRING_REQUEST, (UINT32) strlen(TEST5_STRING_REQUEST),
                          srcURI, destURI2);

        IF_ERROR("tlm_request");
        fprintf(gFp, "->> MD Request sent\n");

        vos_threadDelay(5000000u);

        /* Test expects to fail because of wrong destURI2, must timeout... */
        gFailed ^= gFailed;

        err = tlm_delListener(appHandle2, listenHandle);
        IF_ERROR("tlm_delListener");
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test7
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test7 ()
{
    PREPARE("UDP MD Notify no sessionID #127", "test"); /* allocates appHandle1, appHandle2, failed = 0, err =
                                                          TRDP_NO_ERR */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_LIS_T listenHandle;

        err = tlm_addListener(appHandle2, &listenHandle, NULL, test5CBFunction,
                              TRUE,
                              TEST5_STRING_COMID, 0u, 0u, 0u, VOS_INADDR_ANY, VOS_INADDR_ANY, TRDP_FLAGS_CALLBACK,
                              NULL, NULL);
        IF_ERROR("tlm_addListener");
        fprintf(gFp, "->> MD Listener set up\n");

        err = tlm_notify(appHandle1, NULL, test5CBFunction, TEST5_STRING_COMID, 0u, 0u, 0u,
                         gSession2.ifaceIP, TRDP_FLAGS_CALLBACK, NULL,
                         (UINT8 *)TEST5_STRING_REQUEST, (UINT32) strlen(TEST5_STRING_REQUEST), NULL, NULL );


        IF_ERROR("tlm_notify");
        fprintf(gFp, "->> MD Request sent\n");

        vos_threadDelay(5000000u);

        /* Test expects to fail because of wrong destURI2, must timeout... */
        gFailed ^= gFailed;

        err = tlm_delListener(appHandle2, listenHandle);
        IF_ERROR("tlm_delListener");
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test8
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test8 ()
{
    PREPARE("#153 (two PDs on one pull request? Receiver only", "test"); /* allocates appHandle1, appHandle2, failed =
                                                                           0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T pubHandle;
        TRDP_SUB_T subHandle;

#define TEST8_COMID     1000u
#define TEST8_INTERVAL  100000u
#define TEST8_DATA      "Hello World!"
#define TEST8_DATA_LEN  16u

        /*    Session1 Install subscriber and publisher for PULL (interval = 0).    */

        err = tlp_subscribe(gSession1.appHandle, &subHandle, NULL, NULL, 0u,
                            TEST8_COMID, 0u, 0u,
                            0u, 0u,
                            gDestMC,                            /* MC group */
                            TRDP_FLAGS_NONE,
                            NULL,                      /*    default interface                    */
                            0, TRDP_TO_DEFAULT);


        IF_ERROR("tlp_subscribe");

        err = tlp_publish(gSession1.appHandle, &pubHandle, NULL, NULL, 0u, TEST8_COMID, 0u, 0u,
                          0u,
                          gDestMC,                              /* Destination */
                          0u,
                          0u, TRDP_FLAGS_DEFAULT, NULL, (UINT8 *) TEST8_DATA, TEST8_DATA_LEN);

        IF_ERROR("tlp_publish");


        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter++ < 600)         /* 60 seconds */
        {
            char data2[1432u];
            UINT32 dataSize2 = sizeof(data2);
            TRDP_PD_INFO_T pdInfo;

            usleep(100000u);

            err = tlp_get(gSession1.appHandle, subHandle, &pdInfo, (UINT8 *) data2, &dataSize2);
            if (err == TRDP_NODATA_ERR)
            {
                fprintf(gFp, ".");
                continue;
            }

            if (err == TRDP_TIMEOUT_ERR)
            {
                fprintf(gFp, ".");
                fflush(gFp);
                continue;
            }

            if (err != TRDP_NO_ERR)
            {
                fprintf(gFp, "\n### tlp_get error: %d\n", err);

                gFailed = 1;
                goto end;

            }
            else
            {
                fprintf(gFp, "\nreceived data from pull: %s (seq: %u, size: %u)\n", data2, pdInfo.seqCount, dataSize2);
                gFailed = 0;
                goto end;
            }
        }
    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test9
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test9 ()
{
    PREPARE("Send and receive many telegrams, to check time optimisations", "test"); /* allocates appHandle1,
                                                                                       appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
#define TEST9_NO_OF_TELEGRAMS   200u
#define TEST9_COMID             10000u
#define TEST9_INTERVAL          20000u
#define TEST9_TIMEOUT           (TEST9_INTERVAL * 3)
#define TEST9_DATA              "Hello World!"
#define TEST9_DATA_LEN          16u

        TRDP_PUB_T pubHandle[TEST9_NO_OF_TELEGRAMS];
        TRDP_SUB_T subHandle[TEST9_NO_OF_TELEGRAMS];
        unsigned int i;

        for (i = 0; i < TEST9_NO_OF_TELEGRAMS; i++)
        {
            /*    Session1 Install all publishers    */

            err = tlp_publish(gSession1.appHandle, &pubHandle[i], NULL, NULL, 0u,
                              TEST9_COMID + i, 0u, 0u,
                              0u,
                              gSession2.ifaceIP,                              /* Destination */
                              TEST9_INTERVAL,
                              0u, TRDP_FLAGS_DEFAULT, NULL, (UINT8 *) TEST9_DATA, TEST9_DATA_LEN);

            IF_ERROR("tlp_publish");
            err = tlp_subscribe(gSession2.appHandle, &subHandle[i], NULL, NULL, 0u,
                                TEST9_COMID + i, 0u, 0u,
                                gSession1.ifaceIP,
                                0u, 0u,                        /* MC group */
                                TRDP_FLAGS_NONE,
                                NULL,                      /*    default interface                    */
                                TEST9_TIMEOUT, TRDP_TO_DEFAULT);


            IF_ERROR("tlp_subscribe");

        }

        fprintf(gFp, "\nInitialized %u publishers & subscribers!\n", i);

        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter++ < 10)         /* 1 * TEST9_NO_OF_TELEGRAMS seconds -> 200s */
        {
            char data2[1432u];
            UINT32 dataSize2 = sizeof(data2);
            TRDP_PD_INFO_T pdInfo;


            for (i = 0; i < TEST9_NO_OF_TELEGRAMS; i++)
            {
                sprintf(data2, "--ComId %08u", i);
                (void) tlp_put(gSession1.appHandle, pubHandle[i], (UINT8 *) data2, TEST9_DATA_LEN);

                /*    Wait for answer   */
                vos_threadDelay(TEST9_INTERVAL * 2);

                err = tlp_get(gSession2.appHandle, subHandle[i], &pdInfo, (UINT8 *) data2, &dataSize2);
                if (err == TRDP_NODATA_ERR)
                {
                    /* fprintf(gFp, "."); */
                    continue;
                }

                if (err == TRDP_TIMEOUT_ERR)
                {
                    /*
                       fprintf(gFp, ".");
                       fflush(gFp);
                     */
                    continue;
                }

                if (err != TRDP_NO_ERR)
                {
                    fprintf(gFp, "\n### tlp_get error: %d\n", err);

                    gFailed = 1;
                    goto end;

                }
                else
                {
                    /* fprintf(gFp, "\nreceived data from pull: %s (seq: %u, size: %u)\n", data2, pdInfo.seqCount,
                       dataSize2); */
                    gFailed = 0;
                    /* goto end; */
                }
            }
        }
    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test10
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
#include "trdp_private.h"
static int test10 ()
{
    PREPARE1(""); /* allocates appHandle1, failed = 0, err = TRDP_NO_ERR */

    /* ------------------------- test code starts here --------------------------- */

    {
        err = 0;
        fprintf(gFp, "TRDP Version %s\n", tlc_getVersionString());
        fprintf(gFp, "TRDP Version %s\n", TRDP_VERSION_STR);
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test11
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test11 ()
{
    PREPARE("babbling idiot :-)", "-"); /* allocates appHandle1, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    /* from Ticket #172:
     In case of PD Pull Multipoint-to-Point, when a PD Pull Request message is sent and the device is registered in the MC group it sends the request to, then the stack has two behaviours:
     It receives the request it sent to itself (ok) but then replies with a 'Pr' message
     It will endlessly start sending messages
     Given the following setup:
     - The device has the IP address 172.16.4.21
     - Subscribed to ComId 2000, destination MC 239.0.0.10
     - Subscribed to ComId 1000, destination 172.16.4.21
     - Request ComId 1000, destination 239.0.0.10, reply ComId 1000, replyIp 172.16.4.21 */

    {
        TRDP_SUB_T pubHandle1;
        TRDP_SUB_T subHandle0;
        TRDP_SUB_T subHandle1;
        TRDP_SUB_T subHandle2;

#define TEST11_COMID_2000       2000u
#define TEST11_COMID_2000_DEST  0xEF00000Au
#define TEST11_COMID_1000       1000u
#define TEST11_COMID_1000_SRC   gSession1.ifaceIP
#define TEST11_COMID_1000_DEST  0xEF00000Au


#define TEST11_INTERVAL     100000u
#define TEST11_DATA         "Hello World!"
#define TEST11_DATA_LEN     24u

        err = tlp_publish(gSession2.appHandle, &pubHandle1, NULL, NULL, 0u,
                          TEST11_COMID_1000, 0u, 0u,
                          0u,
                          TEST11_COMID_1000_DEST,
                          0u, 0u, TRDP_FLAGS_DEFAULT, NULL,
                          (UINT8 *)TEST11_DATA, 12u);



        IF_ERROR("tlp_publish");

        err = tlp_subscribe(gSession2.appHandle, &subHandle0, NULL, NULL, 0u,
                            TEST11_COMID_1000, 0u, 0u,
                            0u, 0u, /* gSession2.ifaceIP,                  / * Source * / */
                            TEST11_COMID_1000_DEST,                  /* Destination */
                            TRDP_FLAGS_DEFAULT,
                            NULL,                      /*    default interface                    */
                            0u, TRDP_TO_DEFAULT);


        IF_ERROR("tlp_subscribe1");

        err = tlp_subscribe(gSession1.appHandle, &subHandle1, NULL, NULL, 0u,
                            TEST11_COMID_2000, 0u, 0u,
                            0u, 0u, /* gSession1.ifaceIP,                  / * Source * / */
                            TEST11_COMID_2000_DEST,                  /* Destination */
                            TRDP_FLAGS_DEFAULT,
                            NULL,                      /*    default interface                    */
                            0u, TRDP_TO_DEFAULT);


        IF_ERROR("tlp_subscribe1");

        err = tlp_subscribe(gSession1.appHandle, &subHandle2, NULL, NULL, 0u,
                            TEST11_COMID_1000, 0u, 0u,
                            0u, 0u, /* TEST11_COMID_1000_SRC,             / * Source filter* / */
                            0u,                                     /* Destination unicast */
                            TRDP_FLAGS_DEFAULT,
                            NULL,                      /*    default interface                    */
                            0u, TRDP_TO_DEFAULT);


        IF_ERROR("tlp_subscribe2");

        err = tlp_request(gSession1.appHandle, subHandle2, 0u, TEST11_COMID_1000, 0u, 0u,
                          0u,
                          TEST11_COMID_1000_DEST, 0u, TRDP_FLAGS_NONE, NULL, NULL, 0u,
                          TEST11_COMID_1000, TEST11_COMID_1000_SRC);

        IF_ERROR("tlp_request");

        int counter = 100;
        while (counter--)
        {
            TRDP_PD_INFO_T pdInfo;
            UINT8 buffer[TRDP_MAX_PD_DATA_SIZE];
            UINT32 dataSize = TRDP_MAX_PD_DATA_SIZE;
            vos_threadDelay(20000);
            err = tlp_get(gSession1.appHandle, subHandle2, &pdInfo, buffer, &dataSize);
            if (err == TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_USR,
                             "Rec. Seq: %u Typ: %c%c\n",
                             pdInfo.seqCount,
                             pdInfo.msgType >> 8,
                             pdInfo.msgType & 0xFF);
                vos_printLog(VOS_LOG_USR, "Data: %*s\n", dataSize, buffer);
                break;
            }
        }
        IF_ERROR("tlp_get");
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}




/**********************************************************************************************************************/
/** test12 Ticket #1
 *
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test12 ()
{
    /* allocates appHandle1, appHandle2, failed = 0, err = TRDP_NO_ERR */
    PREPARE("testing unsubscribe and unjoin", "");

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T pubHandle;
        TRDP_SUB_T subHandle1;
        TRDP_SUB_T subHandle2;
        TRDP_SUB_T subHandle3;
        TRDP_SUB_T subHandle4;

#define TEST12_COMID1       10001u
#define TEST12_COMID2       10002u
#define TEST12_COMID3       10003u
#define TEST12_COMID4       10004u
#define TEST12_MCDEST1      0xEF000301u
#define TEST12_MCDEST2      0xEF000302u
#define TEST12_MCDEST3      0xEF000303u
#define TEST12_MCDEST4      0xEF000304u

#define TEST12_INTERVAL     100000u
#define TEST12_DATA         "Hello World!"
#define TEST12_DATA_LEN     24u

        /*    Copy the packet into the internal send queue, prepare for sending.    */

        err = tlp_publish(gSession1.appHandle, &pubHandle, NULL, NULL, 0u, TEST12_COMID1, 0u, 0u,
                          0u, /* gSession1.ifaceIP,                   / * Source * / */
                          TEST12_MCDEST1,                           /* Destination */
                          TEST12_INTERVAL,
                          0u, TRDP_FLAGS_DEFAULT, NULL, NULL, TEST12_DATA_LEN);

        IF_ERROR("tlp_publish");

        err = tlp_subscribe(gSession2.appHandle, &subHandle1, NULL, NULL, 0u,
                            TEST12_COMID1, 0u, 0u,
                            0u, 0u, /* gSession1.ifaceIP,                  / * Source * / */
                            TEST12_MCDEST1, /* gDestMC,                            / * Destination * / */
                            TRDP_FLAGS_DEFAULT,
                            NULL,                      /*    default interface                    */
                            TEST12_INTERVAL * 3, TRDP_TO_DEFAULT);

        IF_ERROR("tlp_subscribe1");
        err = tlp_subscribe(gSession2.appHandle, &subHandle2, NULL, NULL, 0u,
                            TEST12_COMID2, 0u, 0u,
                            0u, 0u, /* gSession1.ifaceIP,                  / * Source * / */
                            TEST12_MCDEST2, /* gDestMC,                            / * Destination * / */
                            TRDP_FLAGS_DEFAULT,
                            NULL,                      /*    default interface                    */
                            TEST12_INTERVAL * 3, TRDP_TO_DEFAULT);

        IF_ERROR("tlp_subscribe2");
        err = tlp_subscribe(gSession2.appHandle, &subHandle3, NULL, NULL, 0u,
                            TEST12_COMID3, 0u, 0u,
                            0u, 0u, /* gSession1.ifaceIP,                  / * Source * / */
                            TEST12_MCDEST3, /* gDestMC,                            / * Destination * / */
                            TRDP_FLAGS_DEFAULT,
                            NULL,                      /*    default interface                    */
                            TEST12_INTERVAL * 3, TRDP_TO_DEFAULT);

        IF_ERROR("tlp_subscribe3");
        err = tlp_subscribe(gSession2.appHandle, &subHandle4, NULL, NULL, 0u,
                            TEST12_COMID4, 0u, 0u,
                            0u, 0u, /* gSession1.ifaceIP,                  / * Source * / */
                            TEST12_MCDEST2, /* gDestMC,                            / * Destination * / */
                            TRDP_FLAGS_DEFAULT,
                            NULL,                      /*    default interface                    */
                            TEST12_INTERVAL * 3, TRDP_TO_DEFAULT);

        IF_ERROR("tlp_subscribe4");

        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter < 10)         /* 1 second */
        {
            char data1[1432u];
            char data2[1432u];
            UINT32 dataSize2 = sizeof(data2);
            TRDP_PD_INFO_T pdInfo;

            sprintf(data1, "Just a Counter: %08d", counter++);

            err = tlp_put(gSession1.appHandle, pubHandle, (UINT8 *) data1, (UINT32) strlen(data1));
            IF_ERROR("tap_put");

            vos_threadDelay(100000);

            err = tlp_get(gSession2.appHandle, subHandle1, &pdInfo, (UINT8 *) data2, &dataSize2);


            if (err == TRDP_NODATA_ERR)
            {
                continue;
            }

            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_USR, "### tlp_get error: %s\n", vos_getErrorString((VOS_ERR_T)err));

                gFailed = 1;
                goto end;

            }
            else
            {
                if (memcmp(data1, data2, dataSize2) == 0)
                {
                    fprintf(gFp, "receiving data ..\n");
                }
            }
        }

        /* Now unsubscribe and check for info log: */
        vos_printLog(VOS_LOG_USR, "Unsubscribing 2 should not unjoin MC %s!\n", vos_ipDotted(TEST12_MCDEST2));
        FULL_LOG(TRUE);
        err = tlp_unsubscribe(gSession2.appHandle, subHandle2);
        IF_ERROR("tlp_unsubscribe2");

        err = tlp_unsubscribe(gSession2.appHandle, subHandle3);
        IF_ERROR("tlp_unsubscribe3");

        vos_printLog(VOS_LOG_USR, "Unsubscribing 4 should unjoin MC %s!\n", vos_ipDotted(TEST12_MCDEST2));
        err = tlp_unsubscribe(gSession2.appHandle, subHandle4);
        IF_ERROR("tlp_unsubscribe4");

        err = tlp_unsubscribe(gSession2.appHandle, subHandle1);
        IF_ERROR("tlp_unsubscribe1");
        FULL_LOG(FALSE);         /* switch off debug & info log */
        vos_printLog(VOS_LOG_USR,
                     "Check log manually whether unjoining %s occured after unsubscribing 4\n",
                     vos_ipDotted(TEST12_MCDEST2));

    }

    /* ------------------------- test code ends here ------------------------------------ */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test13
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
#define TEST13_COMID        0u
#define TEST13_INTERVAL     100000u
#define TEST13_DATA         "Hello World!"
#define TEST13_DATA_LEN     24u

/* Callback incrementer function */
static void cbIncrement (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    INT32                   dataSize)
{
    static unsigned long counter = 0;
    if (dataSize > 18)
    {
        sprintf((char *)pData, "Counting up: %08lu", counter++);
    }
}

static int test13 ()
{
    PREPARE("PD publish and subscribe, auto increment using new 1.4 callback function", "test"); /* allocates
                                                                                                   appHandle1,
                                                                                                   appHandle2, failed =
                                                                                                   0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T pubHandle;
        TRDP_SUB_T subHandle;

        /*    Copy the packet into the internal send queue, prepare for sending.    */

        err = tlp_publish(gSession1.appHandle, &pubHandle, NULL, (TRDP_PD_CALLBACK_T) cbIncrement, 0u, TEST13_COMID, 0u, 0u,
                          0u, /* gSession1.ifaceIP,                   / * Source * / */
                          gSession2.ifaceIP, /* gDestMC,               / * Destination * / */
                          TEST13_INTERVAL,
                          0u, TRDP_FLAGS_DEFAULT, NULL, NULL, TEST13_DATA_LEN);

        IF_ERROR("tlp_publish");

        err = tlp_subscribe(gSession2.appHandle, &subHandle, NULL, NULL, 0u,
                            TEST13_COMID, 0u, 0u,
                            0u, 0u, /* gSession1.ifaceIP,                  / * Source * / */
                            0u, /* gDestMC,                            / * Destination * / */
                            TRDP_FLAGS_DEFAULT,
                            NULL,                      /*    default interface                    */
                            TEST13_INTERVAL * 3, TRDP_TO_DEFAULT);


        IF_ERROR("tlp_subscribe");

        /* We need to call put once to start the transmission! */

        (void) tlp_put(gSession1.appHandle, pubHandle, (UINT8 *)TEST13_DATA, TEST13_DATA_LEN);

        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter++ < 50)         /* 5 seconds */
        {
            char data2[1432u];
            UINT32 dataSize2 = sizeof(data2);
            TRDP_PD_INFO_T pdInfo;

            vos_threadDelay(500000);

            err = tlp_get(gSession2.appHandle, subHandle, &pdInfo, (UINT8 *) data2, &dataSize2);


            if (err == TRDP_NODATA_ERR)
            {
                continue;
            }

            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_INFO, "### tlp_get error: %s\n", vos_getErrorString((VOS_ERR_T)err));

                gFailed = 1;
                /* goto end; */

            }
            else
            {
                fprintf(gFp, "Receiving (seq: %u): %s\n", pdInfo.seqCount, data2);
            }
        }
    }


    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}


/**********************************************************************************************************************/
/** test14
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static UINT32 gTest14CBCounter = 0;

static void test14PDcallBack (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    void *pSentdata = (void *) pMsg->pUserRef;

    gTest14CBCounter++;

    /*    Check why we have been called    */
    switch (pMsg->resultCode)
    {
       case TRDP_NO_ERR:
           if ((pSentdata != NULL) && (pData != NULL) && (memcmp(pData, pSentdata, dataSize) == 0))
           {
               fprintf(gFp, "received data matches (seq: %u, size: %u)\n", pMsg->seqCount, dataSize);
           }
           else
           {
               fprintf(gFp, "some data received (seq: %u, size: %u)\n", pMsg->seqCount, dataSize);
           }
           break;

       case TRDP_TIMEOUT_ERR:
           /* The application can decide here if old data shall be invalidated or kept    */
           fprintf(gFp, "Packet timed out (ComId %d, SrcIP: %s)\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->srcIpAddr));
           break;
       default:
           fprintf(gFp, "Error on packet received (ComId %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
           break;
    }
}

static int test14 ()
{
    PREPARE("Publish & Subscribe, Callback", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T pubHandle;
        TRDP_SUB_T subHandle;
        char data1[1432u];

        gTest14CBCounter = 0;

#define TEST14_COMID        1000u
#define TEST14_INTERVAL     100000u
#define TEST14_DATA         "Hello World!"

#define TEST14_LOOP         TEST14_INTERVAL
#define TEST14_WAIT         600000u

        /*    Copy the packet into the internal send queue, prepare for sending.    */

        err = tlp_publish(gSession1.appHandle, &pubHandle, NULL, NULL, 0u, TEST14_COMID, 0u, 0u,
                          0u, /* gSession1.ifaceIP,                   / * Source * / */
                          gSession2.ifaceIP, /* gDestMC,               / * Destination * / */
                          TEST14_INTERVAL,
                          0u, TRDP_FLAGS_DEFAULT, NULL, NULL, 0u);

        IF_ERROR("tlp_publish");

        err = tlp_subscribe(gSession2.appHandle, &subHandle, data1, test14PDcallBack, 0u,
                            TEST14_COMID, 0u, 0u,
                            0u, 0u, /* gSession1.ifaceIP,                  / * Source * / */
                            0u, /* gDestMC,                            / * Destination * / */
                            TRDP_FLAGS_CALLBACK | TRDP_FLAGS_FORCE_CB,
                            NULL,                      /*    default interface                    */
                            TEST14_INTERVAL * 3, TRDP_TO_DEFAULT);


        IF_ERROR("tlp_subscribe");

        /*
         Enter the main processing loop.
         */
        unsigned int counter = 0;
        while (counter < 5)         /* 0.5 seconds */
        {

            sprintf(data1, "Just a Counter: %08u", counter++);

            err = tlp_put(gSession1.appHandle, pubHandle, (UINT8 *) data1, (UINT32) strlen(data1));
            IF_ERROR("tap_put");

            vos_threadDelay(TEST14_LOOP);

        }

        /* test if no of callbacks equals no of sent packets (bug report Bryce Jensen) */
        /* wait a bit */
        vos_threadDelay(TEST14_WAIT);
        fprintf(gFp,
                "%u max. expected, %u callbacks received\n",
                (counter * TEST14_LOOP + TEST14_WAIT) / TEST14_INTERVAL,
                gTest14CBCounter);
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test15 MD Request - Reply / Reuse of TCP connection
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

#define                 TEST15_STRING_COMID     1000u
#define                 TEST15_STRING_REQUEST   (char *)&dataBuffer1  /* "Requesting data" */
#define                 TEST15_STRING_REQUEST_LEN   32  /* "Requesting data len" */
#define                 TEST15_STRING_REPLY     (char *)&dataBuffer2  /* "Data transmission succeded" */
#define                 TEST15_STRING_REPLY_LEN     33    /* "Replying data len" */
#define                 TEST15_64BYTES          (char *)&dataBuffer3  /* "1...15 * 4" */

static void  test15CBFunction (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    TRDP_ERR_T err;
    TRDP_URI_USER_T srcURI = "12345678901234567890123456789012";    /* 32 chars */
    CHAR8           *localData = (pData == NULL)? "empty data" : (CHAR8*) pData;

    if (pMsg->resultCode == TRDP_REPLYTO_ERR)
    {
        fprintf(gFp, "->> Reply timed out (ComId %u)\n", pMsg->comId);
        gFailed = 1;
    }
    else if ((pMsg->msgType == TRDP_MSG_MR) &&
             (pMsg->comId == TEST15_STRING_COMID))
    {
        if (pMsg->resultCode == TRDP_TIMEOUT_ERR)
        {
            fprintf(gFp, "->> Request timed out (ComId %u)\n", pMsg->comId);
            gFailed = 1;
        }
        else
        {
            fprintf(gFp, "<<- Request received (%.16s...)\n", localData);
//            if (memcmp(srcURI, pMsg->srcUserURI, 32u) != 0)
//            {
//                gFailed = 1;
//                fprintf(gFp, "### srcUserURI wrong\n");
//            }
            fprintf(gFp, "->> Sending reply with query (%.16s)\n", (UINT8 *)TEST15_STRING_REPLY);
            err = tlm_replyQuery(appHandle, &pMsg->sessionId, TEST15_STRING_COMID, 0u, 0u, NULL,
                                 (UINT8 *)TEST15_STRING_REPLY, TEST15_STRING_REPLY_LEN, NULL);

            IF_ERROR("tlm_reply");
        }
    }
    else if ((pMsg->msgType == TRDP_MSG_MQ) &&
             (pMsg->comId == TEST15_STRING_COMID))
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
             (pMsg->comId == TEST15_STRING_COMID))
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

static int test15 ()
{
    PREPARE("TCP MD Request - Reply - Confirm, #206", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        int i;
        TRDP_UUID_T sessionId1;
        TRDP_LIS_T listenHandle;
        TRDP_URI_USER_T destURI1    = "12345678901234567890123456789012";    /* 32 chars */
        TRDP_URI_USER_T destURI2    = "12345678901234567890123456789012";    /* 32 chars */
        TRDP_URI_USER_T srcURI      = "12345678901234567890123456789012";    /* 32 chars */

        gFullLog = TRUE;

        err = tlm_addListener(appHandle2, &listenHandle, NULL, test15CBFunction,
                              TRUE,
                              TEST5_STRING_COMID, 0u, 0u, 0u,
                              VOS_INADDR_ANY, VOS_INADDR_ANY,
                              TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP, NULL, destURI1);
        IF_ERROR("tlm_addListener1");
        fprintf(gFp, "<<- MD TCP Listener1 set up\n");

        /* try to connect (request) some data */
        for (i = 0; i < 10; i++)
        {
            err = tlm_request(appHandle1, NULL, test15CBFunction, &sessionId1,
                              TEST5_STRING_COMID, 0u, 0u,
                              0u, gSession2.ifaceIP,
                              TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP, 1u, 1000000u, NULL,
                              (UINT8 *)TEST15_STRING_REQUEST, TEST15_STRING_REQUEST_LEN,
                              srcURI, destURI2);

            IF_ERROR("tlm_request1");
            fprintf(gFp, "->> MD TCP Request1 sent\n");

            vos_threadDelay(500000u);
        }

        fprintf(gFp, "Waiting 6s ... \n");
        /* Wait until listener connection times out */
        vos_threadDelay(6000000u);

        /* now try to connect again */
        for (i = 0; i < 10; i++)
        {
            err = tlm_request(appHandle1, NULL, test15CBFunction, &sessionId1,
                              TEST5_STRING_COMID, 0u, 0u,
                              0u, gSession2.ifaceIP,
                              TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP, 1u, 1000000u, NULL,
                              (UINT8 *)TEST15_STRING_REQUEST, TEST15_STRING_REQUEST_LEN,
                              srcURI, destURI2);

            IF_ERROR("tlm_request2");
            fprintf(gFp, "->> MD TCP Request2 sent\n");

            vos_threadDelay(500000u);
        }

        err = tlm_delListener(appHandle2, listenHandle);
        IF_ERROR("tlm_delListener2");

        gFullLog = FALSE;

    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

static int test16 ()
{
    PREPARE("UDP MD Request - Reply - Confirm, #206", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        int i;
        TRDP_UUID_T sessionId1;
        TRDP_LIS_T listenHandle;
        TRDP_URI_USER_T destURI1    = "12345678901234567890123456789012";    /* 32 chars */
        TRDP_URI_USER_T destURI2    = "12345678901234567890123456789012";    /* 32 chars */
        TRDP_URI_USER_T srcURI      = "12345678901234567890123456789012";    /* 32 chars */

        gFullLog = TRUE;

        err = tlm_addListener(appHandle2, &listenHandle, NULL, test15CBFunction,
                              TRUE,
                              TEST5_STRING_COMID, 0u, 0u, 0u,
                              VOS_INADDR_ANY, VOS_INADDR_ANY,
                              TRDP_FLAGS_CALLBACK, NULL, NULL);
        IF_ERROR("tlm_addListener1");
        fprintf(gFp, "->> MD UDP Listener1 set up\n");

        /* try to connect (request) some data */
        for (i = 0; i < 10; i++)
        {
            err = tlm_request(appHandle1, NULL, test15CBFunction, &sessionId1,
                              TEST5_STRING_COMID, 0u, 0u,
                              0u, gSession2.ifaceIP,
                              TRDP_FLAGS_CALLBACK, 1u, 1000000u, NULL,
                              (UINT8 *)TEST15_STRING_REQUEST, TEST15_STRING_REQUEST_LEN,
                              NULL, NULL);

            IF_ERROR("tlm_request1");
            fprintf(gFp, "->> MD UDP Request1 sent\n");

            vos_threadDelay(500000u);
        }

        fprintf(gFp, "Waiting 6s for connection close... \n");
        /* Wait until listener connection times out */
        vos_threadDelay(6000000u);

        /* now try to connect again */
        for (i = 0; i < 10; i++)
        {
            err = tlm_request(appHandle1, NULL, test15CBFunction, &sessionId1,
                              TEST5_STRING_COMID, 0u, 0u,
                              0u, gSession2.ifaceIP,
                              TRDP_FLAGS_CALLBACK, 1u, 1000000u, NULL,
                              (UINT8 *)TEST15_STRING_REQUEST, TEST15_STRING_REQUEST_LEN,
                              NULL, NULL);

            IF_ERROR("tlm_request2");
            fprintf(gFp, "->> MD UDP Request2 sent\n");

            vos_threadDelay(500000u);
        }

        err = tlm_delListener(appHandle2, listenHandle);
        IF_ERROR("tlm_delListener2");

        gFullLog = FALSE;

    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test17
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test17 ()
{
    //PREPARE1(""); /* allocates appHandle1, failed = 0, err = TRDP_NO_ERR */

    /* ------------------------- test code starts here --------------------------- */

    {
        //err = 0;

        {
            UINT8   str[] = "123456789";
            UINT32 result, seed, len = (UINT32) strlen((char*) str);
            /* CRC of the string "123456789" is 0x1697d06a ??? */
            seed = 0;
            result = vos_sc32(seed, str, len);
            fprintf(gFp, "sc32 of '%s' (seed = %0x) is 0x%08x\n", str, seed, result);
        }
        {
            UINT8   str[] = "123456789";
            UINT32 result, seed, len = (UINT32) strlen((char*) str);
            /* CRC of the string "123456789" is 0x1697d06a ??? */
            seed = 0xFFFFFFFF;
            result = vos_sc32(seed, str, len);
            fprintf(gFp, "sc32 of '%s' (seed = %0x) is 0x%08x\n", str, seed, result);
        }

    }

    /* ------------------------- test code ends here --------------------------- */


    //CLEANUP;
    return 0;
}

/**********************************************************************************************************************/
/** test18
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test18 ()
{
    PREPARE1("XML test18"); /* allocates appHandle1, failed = 0, err = TRDP_NO_ERR */

    /* ------------------------- test code starts here --------------------------- */

    {
        /* Try to parse the xmlBuffer (test6.xml in memory): */

        TRDP_XML_DOC_HANDLE_T   docHnd;
        TRDP_MEM_CONFIG_T       memConfig;
        TRDP_DBG_CONFIG_T       dbgConfig;
        UINT32                  numComPar;
        TRDP_COM_PAR_T          *pComPar;
        UINT32                  numIfConfig;
        TRDP_IF_CONFIG_T        *pIfConfig;
        unsigned int i;

        err = tau_prepareXmlMem (xmlBuffer, strlen(xmlBuffer), &docHnd);
        IF_ERROR("tau_prepareXmlMem");

        err = tau_readXmlDeviceConfig (&docHnd, &memConfig, &dbgConfig, &numComPar, &pComPar, &numIfConfig, &pIfConfig);
        IF_ERROR("tau_readXmlDeviceConfig");

        for (i = 0; i < numIfConfig; i++)
        {
            fprintf (gFp, "interface label: %s\n", pIfConfig[i].ifName);            /**< interface name   */
            fprintf (gFp, "network ID     : %u\n", pIfConfig[i].networkId);         /**< used network on the device (1...4)   */
            fprintf (gFp, "host IP        : 0x%08x\n", pIfConfig[i].hostIp);        /**< host IP address   */
            fprintf (gFp, "leader IP      : 0x%08x\n", pIfConfig[i].leaderIp);      /**< Leader IP address dependant on redundancy concept   */
        }
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
    test1,  /* PD publish and subscribe */
    test2,  /* Publish & Subscribe, Callback */
    test3,  /* Ticket #140: tlp_get reports immediately TRDP_TIMEOUT_ERR */
    test4,  /* Ticket #153 (two PDs on one pull request */
    test5,  /* TCP MD Request - Reply - Confirm, #149, #160 */
    test6,  /* UDP MD Request - Reply - Confirm, #149 */
    test7,  /* UDP MD Notify no sessionID #127 */
    test8,  /* #153 (two PDs on one pull request? Receiver only */
    test9,  /* Send and receive many telegrams, to check time optimisations */
    test10,  /* tlc_getVersionString */
    test11,  /* babbling idiot :-) */
    test12,  /* testing unsubscribe and unjoin */
    test13,  /* PD publish and subscribe, auto increment using new 1.4 callback function */
    test14,  /* Publish & Subscribe, Callback */
    test15,     /* MD Request - Reply / Reuse of TCP connection */
    test16,     /* MD Request - Reply / UDP */
    test17,     /* CRC */
    test18,     /* XML stream */
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
           {    /*  read primary ip    */
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
           {    /*  read alternate ip    */
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
           {    /*  read target ip (MC)   */
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
           {    /*  read test number    */
               if (sscanf(optarg, "%u",
                          &testNo) < 1)
               {
                   usage(argv[0]);
                   exit(1);
               }
               break;
           }
           case 'v':    /*  version */
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

    printf("TRDP Stack Version %s\n", tlc_getVersionString());
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
