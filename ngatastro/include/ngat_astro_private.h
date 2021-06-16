/* ngat_astro_private.h
** $Header$
*/

#ifndef NGAT_ASTRO_PRIVATE_H
#define NGAT_ASTRO_PRIVATE_H
/** 
 * @file
 * @brief ngat_astro_private.h contains declarations of variables shared across modules in the NGATAstro library, but
 * not to be used externally.
 * This file should only be included internally to ngat_astro_* modules.
 * It provides the external declarations of the error flag/string, so modules can set errors up.
 */

/* external cariabless */
extern int Astro_Error_Number;
extern char Astro_Error_String[];

#endif
