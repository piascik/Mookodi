/* ngat_astro_private.h
** $Header$
*/

/* This file should only be included internally to ngat_astro_* modules.
** It provides the external declarations of the error flag/string, so modules
** can set errors up.
*/
#ifndef NGAT_ASTRO_PRIVATE_H
#define NGAT_ASTRO_PRIVATE_H

/* external cariabless */
extern int Astro_Error_Number;
extern char Astro_Error_String[];

#endif
