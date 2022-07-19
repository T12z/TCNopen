/******************************************************************************/
/**
 * @file            trdp_env.c
 *
 * @brief           Definition of the TRDP constants and specific calculations
 *
 * @details
 *
 * @note            Project: TRDP SPY
 *
 * @author          Florian Weispfenning, Bombardier Transportation
 * @author          Thorsten Schulz, Universität Rostock
 *
 * @copyright       This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * @copyright       Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 * @copyright       Copyright Universität Rostock, 2019 (substantial changes leading to GLib-only version and update to v2.0, Wirshark 3.)
 *
 * $Id: $
 *
 */

/*******************************************************************************
 * INCLUDES
 */
#include "trdp_env.h"

/******************************************************************************
 *   Locals
 */

/*
* The FCS-32 generator polynomial:
*  x**0 + x**1 + x**2 + x**4 + x**5 + x**7 + x**8 + x**10 + x**11 + x**12 + x**16 + x**22 + x**23 + x**26 + x**32.
*/
static const guint32 fcstab[256] =
{
      0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
      0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
      0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
      0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
      0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
      0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
      0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
      0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
      0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
      0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
      0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
      0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
      0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
      0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
      0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
      0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
      0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
      0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
      0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
      0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
      0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
      0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
      0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
      0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
      0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
      0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
      0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
      0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
      0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
      0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
      0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
      0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
      0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
      0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
      0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
      0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
      0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
      0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
      0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
      0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
      0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
      0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
      0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
      0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
      0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
      0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
      0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
      0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
      0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
      0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
      0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
      0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
      0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
      0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
      0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
      0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
      0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
      0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
      0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
      0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
      0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
      0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
      0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
      0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

guint32 trdp_fcs32(const guint8 buf[], guint32 len, guint32 fcs)
{
    guint32 i;

    for (i=0; i < len; i++)
    {
        fcs = (fcs >> 8)^fcstab[(fcs ^ buf[i]) & 0xff];
    }
    return ~fcs;
}

/*
* The SC-32 generator polynomial
*/
static const guint32 sctab[256] =
{
      0x00000000U, 0xF4ACFB13U, 0x1DF50D35U, 0xE959F626U,
      0x3BEA1A6AU, 0xCF46E179U, 0x261F175FU, 0xD2B3EC4CU,
      0x77D434D4U, 0x8378CFC7U, 0x6A2139E1U, 0x9E8DC2F2U,
      0x4C3E2EBEU, 0xB892D5ADU, 0x51CB238BU, 0xA567D898U,
      0xEFA869A8U, 0x1B0492BBU, 0xF25D649DU, 0x06F19F8EU,
      0xD44273C2U, 0x20EE88D1U, 0xC9B77EF7U, 0x3D1B85E4U,
      0x987C5D7CU, 0x6CD0A66FU, 0x85895049U, 0x7125AB5AU,
      0xA3964716U, 0x573ABC05U, 0xBE634A23U, 0x4ACFB130U,
      0x2BFC2843U, 0xDF50D350U, 0x36092576U, 0xC2A5DE65U,
      0x10163229U, 0xE4BAC93AU, 0x0DE33F1CU, 0xF94FC40FU,
      0x5C281C97U, 0xA884E784U, 0x41DD11A2U, 0xB571EAB1U,
      0x67C206FDU, 0x936EFDEEU, 0x7A370BC8U, 0x8E9BF0DBU,
      0xC45441EBU, 0x30F8BAF8U, 0xD9A14CDEU, 0x2D0DB7CDU,
      0xFFBE5B81U, 0x0B12A092U, 0xE24B56B4U, 0x16E7ADA7U,
      0xB380753FU, 0x472C8E2CU, 0xAE75780AU, 0x5AD98319U,
      0x886A6F55U, 0x7CC69446U, 0x959F6260U, 0x61339973U,
      0x57F85086U, 0xA354AB95U, 0x4A0D5DB3U, 0xBEA1A6A0U,
      0x6C124AECU, 0x98BEB1FFU, 0x71E747D9U, 0x854BBCCAU,
      0x202C6452U, 0xD4809F41U, 0x3DD96967U, 0xC9759274U,
      0x1BC67E38U, 0xEF6A852BU, 0x0633730DU, 0xF29F881EU,
      0xB850392EU, 0x4CFCC23DU, 0xA5A5341BU, 0x5109CF08U,
      0x83BA2344U, 0x7716D857U, 0x9E4F2E71U, 0x6AE3D562U,
      0xCF840DFAU, 0x3B28F6E9U, 0xD27100CFU, 0x26DDFBDCU,
      0xF46E1790U, 0x00C2EC83U, 0xE99B1AA5U, 0x1D37E1B6U,
      0x7C0478C5U, 0x88A883D6U, 0x61F175F0U, 0x955D8EE3U,
      0x47EE62AFU, 0xB34299BCU, 0x5A1B6F9AU, 0xAEB79489U,
      0x0BD04C11U, 0xFF7CB702U, 0x16254124U, 0xE289BA37U,
      0x303A567BU, 0xC496AD68U, 0x2DCF5B4EU, 0xD963A05DU,
      0x93AC116DU, 0x6700EA7EU, 0x8E591C58U, 0x7AF5E74BU,
      0xA8460B07U, 0x5CEAF014U, 0xB5B30632U, 0x411FFD21U,
      0xE47825B9U, 0x10D4DEAAU, 0xF98D288CU, 0x0D21D39FU,
      0xDF923FD3U, 0x2B3EC4C0U, 0xC26732E6U, 0x36CBC9F5U,
      0xAFF0A10CU, 0x5B5C5A1FU, 0xB205AC39U, 0x46A9572AU,
      0x941ABB66U, 0x60B64075U, 0x89EFB653U, 0x7D434D40U,
      0xD82495D8U, 0x2C886ECBU, 0xC5D198EDU, 0x317D63FEU,
      0xE3CE8FB2U, 0x176274A1U, 0xFE3B8287U, 0x0A977994U,
      0x4058C8A4U, 0xB4F433B7U, 0x5DADC591U, 0xA9013E82U,
      0x7BB2D2CEU, 0x8F1E29DDU, 0x6647DFFBU, 0x92EB24E8U,
      0x378CFC70U, 0xC3200763U, 0x2A79F145U, 0xDED50A56U,
      0x0C66E61AU, 0xF8CA1D09U, 0x1193EB2FU, 0xE53F103CU,
      0x840C894FU, 0x70A0725CU, 0x99F9847AU, 0x6D557F69U,
      0xBFE69325U, 0x4B4A6836U, 0xA2139E10U, 0x56BF6503U,
      0xF3D8BD9BU, 0x07744688U, 0xEE2DB0AEU, 0x1A814BBDU,
      0xC832A7F1U, 0x3C9E5CE2U, 0xD5C7AAC4U, 0x216B51D7U,
      0x6BA4E0E7U, 0x9F081BF4U, 0x7651EDD2U, 0x82FD16C1U,
      0x504EFA8DU, 0xA4E2019EU, 0x4DBBF7B8U, 0xB9170CABU,
      0x1C70D433U, 0xE8DC2F20U, 0x0185D906U, 0xF5292215U,
      0x279ACE59U, 0xD336354AU, 0x3A6FC36CU, 0xCEC3387FU,
      0xF808F18AU, 0x0CA40A99U, 0xE5FDFCBFU, 0x115107ACU,
      0xC3E2EBE0U, 0x374E10F3U, 0xDE17E6D5U, 0x2ABB1DC6U,
      0x8FDCC55EU, 0x7B703E4DU, 0x9229C86BU, 0x66853378U,
      0xB436DF34U, 0x409A2427U, 0xA9C3D201U, 0x5D6F2912U,
      0x17A09822U, 0xE30C6331U, 0x0A559517U, 0xFEF96E04U,
      0x2C4A8248U, 0xD8E6795BU, 0x31BF8F7DU, 0xC513746EU,
      0x6074ACF6U, 0x94D857E5U, 0x7D81A1C3U, 0x892D5AD0U,
      0x5B9EB69CU, 0xAF324D8FU, 0x466BBBA9U, 0xB2C740BAU,
      0xD3F4D9C9U, 0x275822DAU, 0xCE01D4FCU, 0x3AAD2FEFU,
      0xE81EC3A3U, 0x1CB238B0U, 0xF5EBCE96U, 0x01473585U,
      0xA420ED1DU, 0x508C160EU, 0xB9D5E028U, 0x4D791B3BU,
      0x9FCAF777U, 0x6B660C64U, 0x823FFA42U, 0x76930151U,
      0x3C5CB061U, 0xC8F04B72U, 0x21A9BD54U, 0xD5054647U,
      0x07B6AA0BU, 0xF31A5118U, 0x1A43A73EU, 0xEEEF5C2DU,
      0x4B8884B5U, 0xBF247FA6U, 0x567D8980U, 0xA2D17293U,
      0x70629EDFU, 0x84CE65CCU, 0x6D9793EAU, 0x993B68F9U
};

guint32 trdp_sc32(const guint8 buf[], guint32 len, guint32 sc)
{
    guint32 i;

    for (i=0; i < len; i++)
    {
        sc = sctab[((guint32)(sc >> 24) ^ buf[i]) & 0xff]^(sc << 8);
    }
    return sc;
}


gint32 trdp_dissect_width(guint32 type)
{
	switch(type)
		{
        case TRDP_BITSET8:    //BITSET8     1
        case TRDP_CHAR8:      //CHAR8		2	char, can be used also as UTF8
        case TRDP_INT8:       //INT8		4	Signed integer, 8 bit
        case TRDP_UINT8:      //UINT8		8	Unsigned integer, 8 bit
			return 1;
        case TRDP_UTF16:      //UTF16		3	Unicode UTF-16 character
        case TRDP_INT16:      //INT16		5	Signed integer, 16 bit
        case TRDP_UINT16:     //UINT16		9	Unsigned integer, 16 bit
			return 2;
        case TRDP_INT32:      //INT32		6	Signed integer, 32 bit
        case TRDP_UINT32:     //UINT32		10	Unsigned integer, 32 bit
        case TRDP_REAL32:     //REAL32		12	Floating point real, 32 bit
        case TRDP_TIMEDATE32: //TIMEDATE32	14	32 bit UNIX time
        case TRDP_SC32:       //SC32		17	SC-32, 32 bit
			return 4;
        case TRDP_INT64:      //INT64		7	Signed integer, 64 bit
        case TRDP_UINT64:     //UINT64		11	Unsigned integer, 64 bit
        case TRDP_REAL64:     //REAL64		13	Floating point real, 64 bit
        case TRDP_TIMEDATE64: //TIMEDATE64	16	32 bit seconds and 32 bit microseconds
			return 8;
        case TRDP_TIMEDATE48: //TIMEDATE48	15	48 bit TCN time (32 bit seconds and 16 bit ticks)
			return 6;
		default:
			return -1;
		}
}
