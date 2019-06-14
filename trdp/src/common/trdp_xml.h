/******************************************************************************/
/**
 * @file            trdp_xml.h
 *
 * @brief           Simple XML parser
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright NewTec GmbH or its subsidiaries and others, 2016. All rights reserved.
 */
 /*
 * $Id$
 *
 *      BL 2019-01-23: Ticket #231: XML config from stream buffer
 *      BL 2016-02-11: Ticket #102: Replacing libxml2
 *
 */


#ifndef TRDP_XML_H
#define TRDP_XML_H

/*******************************************************************************
 * INCLUDES
 */

#include <stdio.h>

#include "trdp_private.h"
#include "vos_utils.h"

/*******************************************************************************
 * DEFINES
 */

/*******************************************************************************
 * TYPEDEFS
 */

/* XML parser declarations */

#define MAX_URI_LEN     101u         /* Max length of a URI string */
#define MAX_TOK_LEN     124u         /* Max length of token/attribute string */
#define MAX_TAG_LEN     132u         /* Max length of tag string */

/* Tokens */
typedef enum
{
    TOK_OPEN = 0,       /* "<" character    */
    TOK_CLOSE,          /* ">" character    */
    TOK_OPEN_END,       /* "</" characters. Open for an end tag.    */
    TOK_CLOSE_EMPTY,    /* "/> characters. Close for an empty element.    */
    TOK_EQUAL,          /* "=" character    */
    TOK_ID,             /* "Identifier". Identifiers are character sequences limited
                         by whitespace characters or special characters ("<>/=").
                         Identifiers in quotes (") may contain whitespace and special characters.
                         The id is put in tokenValue.    */
    TOK_EOF,            /* End of file    */
    TOK_START_TAG,      /* TOK_OPEN + TOK_ID    */
    TOK_END_TAG,        /* TOK_OPEN_END + TOK_ID + TOK_CLOSE    */
    TOK_ATTRIBUTE       /* "<" character    */
} XML_TOKEN_T;

typedef struct XML_HANDLE
{
    FILE    *infile;
    char    tokenValue[MAX_TOK_LEN];
    int     tagDepth;
    int     tagDepthSeek;
    char    tokenTag[MAX_TAG_LEN + 1];
    int     error;
} XML_HANDLE_T, *TRDP_XML_HANDLE_T;

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

TRDP_ERR_T  trdp_XMLOpen (XML_HANDLE_T  *pXML,
                          const char    *file);

TRDP_ERR_T  trdp_XMLMemOpen (XML_HANDLE_T   *pXML,
                             const char     *pBuffer,
                             size_t         bufSize);

void        trdp_XMLClose (XML_HANDLE_T *pXML);
int         trdp_XMLCountStartTag (XML_HANDLE_T    *pXML,
                                   const char      *tag);
int         trdp_XMLSeekStartTagAny (XML_HANDLE_T   *pXML,
                                     char           *tag,
                                     int            maxlen);
int         trdp_XMLSeekStartTag (XML_HANDLE_T  *pXML,
                                  const char    *tag);
XML_TOKEN_T trdp_XMLGetAttribute (XML_HANDLE_T  *pXML,
                                  CHAR8         *attribute,
                                  UINT32        *pValueInt,
                                  CHAR8         *value);

void    trdp_XMLRewind (XML_HANDLE_T *pXML);

void    trdp_XMLEnter (XML_HANDLE_T *pXML);
void    trdp_XMLLeave (XML_HANDLE_T *pXML);

#endif /* TRDP_XML_H */
