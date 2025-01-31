/***************************************************************************
 *   Copyright (c) 2008  Jeff Mitchell <mitchell@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef POPUPDROPPER_EXPORT_H
#define POPUPDROPPER_EXPORT_H

#include <QtGlobal>

#ifdef Q_OS_WINDOWS
# ifdef MAKE_POPUPDROPPER_LIB
   /* We are building this library */
#  define POPUPDROPPER_EXPORT Q_DECL_EXPORT
# else
   /* We are using this library */
#  define POPUPDROPPER_EXPORT Q_DECL_IMPORT
# endif // MAKE_POPUPDROPPER_LIB
#else
# define POPUPDROPPER_EXPORT __attribute__((visibility("default")))
#endif 

#endif
