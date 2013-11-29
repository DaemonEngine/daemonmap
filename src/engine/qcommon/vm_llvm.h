/*
===========================================================================
Copyright (C) 2011-2012 Unvanquished Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_LLVM
void *VM_LoadLLVM( vm_t *vm, intptr_t (*systemcalls)(intptr_t, ...) );
void VM_UnloadLLVM( void *llvmModuleProvider );
#endif

#ifdef __cplusplus
}
#endif
