/**********************************************************************************************************************/
/**
 * @file            trdp_xml.c
 *
 * @brief           Simple XML parser
 *
 * @details         Hint: Missing optional elements must be handled using the count-function, otherwise following
 *                           elements will be following ignored!
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH; based on code by Peter Brander, Bombardier
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/*
* $Id$
*
*     AHW 2023-01-20: Ticket #415: trdp_XMLGet Attribute: ULONG_MAX should be an allowed value
*      BL 2020-01-07: Ticket #284: Parsing Unsigned Values from Config XML
*      BL 2019-01-29: Ticket #232: Write access to XML file
*      BL 2019-01-23: Ticket #231: XML config from stream buffer
*      SB 2018-11-07: Ticket #221: readXmlDatasets failed
*      BL 2016-07-06: Ticket #122: 64Bit compatibility (+ compiler warnings)
*      BL 2016-02-24: missing include (thanks to Robert)
*      BL 2016-02-11: Ticket #102: Replacing libxml2
*/

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>

#include "trdp_xml.h"

/***********************************************************************************************************************
 * DEFINES
 */

/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
*  LOCAL FUNCTIONS
*/

/***********************************************************************************************************************
NAME:       trdp_XMLNextToken
ABSTRACT:   Returns next XML token.
            Skips occurences of whitespace and <!...> and <?...>
RETURNS:    TOK_OPEN ("<"), TOK_CLOSE (">"), TOK_OPEN_END = ("</"),
            TOK_CLOSE_EMPTY = ("/>"), TOK_EQUAL = ("="), TOK_ID, TOK_EOF
*/
/**********************************************************************************************************************/
/** Return next XML token.
 *    Skips occurences of whitespace and <!...> and <?...>
 *
 *  @param[in]      pXML        Pointer to local data
 *
 *  @retval         TOK_OPEN ("<"), TOK_CLOSE (">"), TOK_OPEN_END = ("</"),
 *                  TOK_CLOSE_EMPTY = ("/>"), TOK_EQUAL = ("="), TOK_ID, TOK_EOF
 *
 */
static XML_TOKEN_T trdp_XMLNextToken (
    XML_HANDLE_T *pXML)
{
    int     ch = 0;
    char    *p;

    for (;; )
    {
        /* Skip whitespace */
        while (!feof(pXML->infile) && (ch = fgetc(pXML->infile)) <= ' ') /*lint !e160 Lint objects a GNU warning
                                                                           suppression macro - OK */
        {
            ;
        }

        /* Check for EOF */
        if (feof(pXML->infile)) /*lint !e611 Lint for VxWorks gets lost in macro defintions*/
        {
            return TOK_EOF;
        }

        /* Handle quoted identifiers */
        if (ch == '"')
        {
            p = pXML->tokenValue;
            while (!feof(pXML->infile) && (ch = fgetc(pXML->infile)) != '"') /*lint !e160 Lint objects a GNU warning
                                                                               suppression macro - OK */
            {
                if (p < (pXML->tokenValue + MAX_TOK_LEN - 1))
                {
                    *(p++) = (char) ch;
                }
            }

            *(p++) = 0;
            return TOK_ID;
        }
        else if (ch == '<')
        {
            /* Tag start character */
            ch = fgetc(pXML->infile);    /*lint !e160 Lint objects a GNU warning suppression macro - OK */

            if (ch == '?') /* Skip processing instruction */
            {
                while (!feof(pXML->infile) && (ch = fgetc(pXML->infile))) /*lint !e160 Lint objects a GNU warning
                                                                            suppression macro - OK */
                {
                    if (ch == '?')
                    {
                        if ((ch = fgetc(pXML->infile)) == '>')
                        {
                            break;
                        }
                        else
                        {
                            (void) ungetc(ch, pXML->infile);
                        }
                    }
                }
            }
            else if (ch == '!')
            {
                /* Is it a comment? */
                if (!feof(pXML->infile) && (ch = fgetc(pXML->infile)))
                {
                    if (ch == '-')
                    {
                        if ((ch = fgetc(pXML->infile) == '-'))
                        {
                            int endTagCnt = 0;
                            while (!feof(pXML->infile) && (ch = fgetc(pXML->infile))) /*lint !e160 Lint objects a GNU
                                                                                        warning suppression macro - OK
                                                                                        */
                            {
                                if (ch == '-')
                                {
                                    endTagCnt++;
                                }
                                else if (ch == '>' && endTagCnt == 2)
                                {
                                    endTagCnt = 0;
                                    break;
                                }
                                else
                                {
                                    endTagCnt = 0;
                                }
                            }
                            /* Exit on unexpected end-of-file */
                            if (endTagCnt != 2 && feof(pXML->infile))
                            {
                                pXML->error = TRDP_XML_PARSER_ERR;
                                return TOK_EOF;
                            }
                        }
                    }
                    else
                    {
                        while (!feof(pXML->infile) && (ch = fgetc(pXML->infile)) != '>')
                        {
                            ;
                        }
                    }
                }
                /* Exit on unexpected end-of-file */
                if (feof(pXML->infile))
                {
                    pXML->error = TRDP_XML_PARSER_ERR;
                    return TOK_EOF;
                }
            }
            else if (ch == '/')
            {
                return TOK_OPEN_END;
            }
            else
            {
                (void) ungetc(ch, pXML->infile);
                return TOK_OPEN;
            }
        }
        else if (ch == '/')
        {
            ch = fgetc(pXML->infile); /*lint !e160 Lint objects a GNU warning suppression macro - OK */
            if (ch == '>')
            {
                return TOK_CLOSE_EMPTY;
            }
            else
            {
                (void) ungetc(ch, pXML->infile);
            }
        }
        else if (ch == '>')
        {
            return TOK_CLOSE;
        }
        else if (ch == '=')
        {
            return TOK_EQUAL;
        }
        else
        {
            /* Unquoted identifier */
            p       = pXML->tokenValue;
            *(p++)  = (char) ch;
            while ((!feof(pXML->infile)) &&
                   ((ch = fgetc(pXML->infile)) != '<') /*lint !e160 Lint objects a GNU warning suppression macro - OK */
                   && (ch != '>')
                   && (ch != '=')
                   && (ch != '/')
                   && (ch > ' '))
            {
                if (p < (pXML->tokenValue + MAX_TOK_LEN - 1u))
                {
                    *(p++) = (char) ch;
                }
            }

            *(p++) = 0;

            if ((ch == '<') || (ch == '>') || (ch == '=') || (ch == '/'))
            {
                (void) ungetc(ch, pXML->infile);
            }

            return TOK_ID;
        }
    }
}

/**********************************************************************************************************************/
/** Return next high level XML token.
 *    Any Id is stored in pXML->tokenValue
 *    Other tokens are returned as is
 *
 *  @param[in]      pXML        Pointer to local data
 *
 *  @retval         TOK_START_TAG = TOK_OPEN + TOK_ID
 *                  TOK_END_TAG = TOK_OPEN_END + TOK_ID + TOK_CLOSE
 *
 */
static XML_TOKEN_T trdp_XMLNextTokenHl (
    XML_HANDLE_T *pXML)
{
    XML_TOKEN_T token;

    token = trdp_XMLNextToken(pXML);

    if (token == TOK_OPEN)
    {
        pXML->tagDepth++;
        token = trdp_XMLNextToken(pXML);

        if (token == TOK_ID)
        {
            vos_strncpy(pXML->tokenTag, pXML->tokenValue, MAX_TAG_LEN);
            token = TOK_START_TAG;  /* TOK_OPEN + TOK_ID */
        }
        else
        {
            token = TOK_EOF;  /* Something wrong, < should always be followed by a tag id */
        }
    }
    else if (token == TOK_OPEN_END)
    {
        pXML->tagDepth--;
        token = trdp_XMLNextToken(pXML);

        if (token == TOK_ID)
        {
            vos_strncpy(pXML->tokenTag, pXML->tokenValue, MAX_TAG_LEN);
            token = TOK_END_TAG; /* TOK_OPEN_END + TOK_ID + TOK_CLOSE */
        }
        else
        {
            token = TOK_EOF;  /* Something wrong, </ should always be followed by a tag id + ">"*/
        }
    }
    else if (token == TOK_CLOSE_EMPTY)
    {
        pXML->tagDepth--;
    }
    else if (token == TOK_ID)
    {
        vos_strncpy(pXML->tokenTag, pXML->tokenValue, MAX_TAG_LEN);
        token = TOK_ID;
    }

    return token;
}

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/**********************************************************************************************************************/
/** Opens the XML parsing.
 *
 *  @param[in]      pXML        Pointer to local data
 *  @param[in]      file        Pathname of XML file
 *
 *  @retval         none
 */
TRDP_ERR_T trdp_XMLOpen (
    XML_HANDLE_T    *pXML,
    const char      *file)
{
    if ((pXML->infile = fopen(file, "rb")) == NULL)
    {
        return TRDP_IO_ERR;
    }

    pXML->tagDepth      = 0;
    pXML->tagDepthSeek  = 0;
    pXML->error         = TRDP_NO_ERR;
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Opens the XML parsing from a buffer (string stream).
 *
 *  @param[in]      pXML        Pointer to local data
 *  @param[in]      pBuffer     Pointer to XML stream buffer
 *  @param[in]      bufSize     Size of XML stream buffer
 *
 *  @retval         TRDP_IO_ERR
 */
TRDP_ERR_T trdp_XMLMemOpen (
    XML_HANDLE_T    *pXML,
    const char      *pBuffer,
    size_t          bufSize)
{
#ifdef HAS_FMEMOPEN
#ifdef __APPLE__
    if (__builtin_available(macOS 10.13, *))
#endif
    if ((pXML->infile = fmemopen((void *)pBuffer, bufSize, "rb")) == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "XML stream could not be opened for reading\n");
        return TRDP_IO_ERR;
    }

    pXML->tagDepth      = 0;
    pXML->tagDepthSeek  = 0;
    pXML->error         = TRDP_NO_ERR;
    return TRDP_NO_ERR;
#else
    /* Create and open a temporary file */
    pXML->infile = tmpfile();               /* Note: Needs admin rights under VISTA!    */
    if (pXML->infile != NULL)
    {
        /* Write to temp file */
    	if (fwrite(pBuffer, 1, bufSize, pXML->infile) == bufSize) {
    		/* prepare for reading */
    		trdp_XMLRewind (pXML);
    		return TRDP_NO_ERR;
    	}
    }
    vos_printLogStr(VOS_LOG_ERROR, "Temporary XML file could not be opened to write\n");
    return TRDP_IO_ERR;
#endif
}

/**********************************************************************************************************************/
/** Rewind to start.
 *
 *  @param[in]      pXML        Pointer to local data
 *
 *  @retval         none
 */
void trdp_XMLRewind (
    XML_HANDLE_T *pXML)
{
    if (pXML->infile == NULL)
    {
        pXML->error = TRDP_XML_PARSER_ERR;
    }
    else if (fseek(pXML->infile, 0, SEEK_SET) == -1)
    {
        pXML->error = TRDP_IO_ERR;
    }
    else
    {
        pXML->tagDepth      = 0;
        pXML->tagDepthSeek  = 0;
        pXML->error         = TRDP_NO_ERR;
    }
}

/**********************************************************************************************************************/
/** Closes the XML parsng.
 *
 *  @param[in]      pXML        Pointer to local data
 *
 *  @retval         none
 */
void trdp_XMLClose (
    XML_HANDLE_T *pXML)
{
    fclose(pXML->infile);
}

/**********************************************************************************************************************/
/** Seek next tag on starting depth and return it in provided buffer.
 *  Start tags on deeper depths are ignored.
 *
 *  @param[in]      pXML        Pointer to local data
 *  @param[in,out]  tag         Buffer for found tag
 *  @param[in]      maxlen      Length of buffer
 *
 *  @retval         0           if found
 *                  !=0         if not found
 */
int trdp_XMLSeekStartTagAny (
    XML_HANDLE_T    *pXML,
    char            *tag,
    int             maxlen)
{
    XML_TOKEN_T token;
    int         ret = 99;

    while (ret == 99)
    {
        token = trdp_XMLNextTokenHl(pXML);

        if (token == TOK_EOF)
        {
            ret = -1;            /* End of file, interrupt */
        }
        else if (pXML->tagDepth < (pXML->tagDepthSeek - 1))
        {
            ret = -2;            /* No more tokens on this depth, interrupt */
        }
        else if ((pXML->tagDepth == pXML->tagDepthSeek) && (token == TOK_START_TAG))
        {
            /* We are on the correct depth and have found a start tag */
            vos_strncpy(tag, pXML->tokenTag, (UINT32) maxlen);
            ret = 0;
        }
        /* else ignore */
    }

    return ret;
}

/**********************************************************************************************************************/
/** Seek a specific tag
 *
 *  @param[in]      pXML        Pointer to local data
 *  @param[in]      tag         Tag to be found
 *
 *  @retval         0           if found
 *                  !=0         if not found
 */
int trdp_XMLSeekStartTag (
    XML_HANDLE_T    *pXML,
    const char      *tag)
{
    int     ret;
    char    buf[MAX_TAG_LEN + 1];
    do
    {
        ret = trdp_XMLSeekStartTagAny(pXML, buf, sizeof(buf));
    }
    while ((ret == 0) && (strcmp(buf, tag) != 0));

    return ret;
}

/**********************************************************************************************************************/
/** Count a specific tag
 *
 *  @param[in]      pXML        Pointer to local data
 *  @param[in]      tag         Tag to count
 *
 *  @retval         0           if found
 *                  !=0         if not found
 */
int trdp_XMLCountStartTag (
    XML_HANDLE_T    *pXML,
    const char      *tag)
{
    int             ret;
    char            buf[MAX_TAG_LEN + 1u];
    int             count = 0;

    XML_HANDLE_T    safe        = *pXML;
    off_t           last_read   = ftell(pXML->infile);

    do
    {
        ret = trdp_XMLSeekStartTagAny(pXML, buf, sizeof(buf));
        if (ret == 0 && strcmp(buf, tag) == 0)
        {
            count++;
        }
    }
    while (ret == 0);

    *pXML = safe;
    fseek(pXML->infile, (long) last_read, SEEK_SET);
    return count;
}

/**********************************************************************************************************************/
/** Enter level in XML file
 *
 *  @param[in]      pXML        Pointer to local data
 *
 *  @retval         none
 */
void trdp_XMLEnter (
    XML_HANDLE_T *pXML)
{
    pXML->tagDepthSeek++;
}

/**********************************************************************************************************************/
/** Leave level in XML file
 *
 *  @param[in]      pXML        Pointer to local data
 *
 *  @retval         none
 */
void trdp_XMLLeave (
    XML_HANDLE_T *pXML)
{
    pXML->tagDepthSeek--;
}

/**********************************************************************************************************************/
/** Get value of next attribute, as string and if possible as integer
 *
 *  @param[in]      pXML        Pointer to local data
 *  @param[in]      attribute   Pointer to attribute
 *  @param[out]     pValueInt   Pointer to resulting integer value
 *  @param[out]     value       Pointer to resulting string value
 *
 *  @retval         TOK_ATTRIBUTE   if found
 *                  token           if not found
 */
XML_TOKEN_T trdp_XMLGetAttribute (
    XML_HANDLE_T    *pXML,
    CHAR8           *attribute,
    UINT32          *pValueInt,
    CHAR8           *value)
{
    XML_TOKEN_T token;

    token = trdp_XMLNextTokenHl(pXML);

    if (token == TOK_ID)
    {
        vos_strncpy(attribute, pXML->tokenValue, MAX_TOK_LEN - 1u);
        token = trdp_XMLNextTokenHl(pXML);

        if (token == TOK_EQUAL)
        {
            token = trdp_XMLNextTokenHl(pXML);

            if (token == TOK_ID)
            {
                unsigned long   uIntValue;
                vos_strncpy(value, pXML->tokenValue, MAX_TOK_LEN - 1u);
                VOS_SET_ERRNO(0);
                uIntValue  = (UINT32) strtoul(value, NULL, 10); /* we expect unsigned values mostly */
                if ((value[0] == '-') || (errno == EINVAL) || (errno == ERANGE))    /* #415 */
                {
                    *pValueInt  = 0u;       /* Return zero if out of range or negative */
                }
                else
                {
                    *pValueInt = (UINT32) uIntValue;
                }
                token       = TOK_ATTRIBUTE;
            }
        }
    }

    return token;
}
