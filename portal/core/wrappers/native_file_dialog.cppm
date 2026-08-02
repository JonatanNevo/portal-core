//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <nfd.h>

export module native_file_dialog;

export
{
    using ::nfdu8char_t;
    using ::nfdnchar_t;
    using ::nfdpathset_t;
    using ::nfdpathsetenum_t;
    using ::nfdfiltersize_t;

    using ::nfdresult_t;
    using ::NFD_ERROR;
    using ::NFD_OKAY;
    using ::NFD_CANCEL;

    using ::nfdu8filteritem_t;
    using ::nfdnfilteritem_t;

    using ::nfdwindowhandle_t;
    using ::NFD_WINDOW_HANDLE_TYPE_UNSET;
    using ::NFD_WINDOW_HANDLE_TYPE_WINDOWS;
    using ::NFD_WINDOW_HANDLE_TYPE_COCOA;
    using ::NFD_WINDOW_HANDLE_TYPE_X11;

    using ::nfdversion_t;

    using ::nfdopendialogu8args_t;
    using ::nfdopendialognargs_t;
    using ::nfdsavedialogu8args_t;
    using ::nfdsavedialognargs_t;
    using ::nfdpickfolderu8args_t;
    using ::nfdpickfoldernargs_t;
    using ::nfdpathsetsize_t;

    using ::NFD_FreePathN;
    using ::NFD_FreePathU8;

    using ::NFD_Init;
    using ::NFD_Quit;

    using ::NFD_OpenDialogN;
    using ::NFD_OpenDialogU8;
    using ::NFD_OpenDialogN_With_Impl;
    using ::NFD_OpenDialogN_With;
    using ::NFD_OpenDialogU8_With_Impl;
    using ::NFD_OpenDialogU8_With;

    using ::NFD_OpenDialogMultipleN;
    using ::NFD_OpenDialogMultipleU8;
    using ::NFD_OpenDialogMultipleN_With_Impl;
    using ::NFD_OpenDialogMultipleN_With;
    using ::NFD_OpenDialogMultipleU8_With_Impl;
    using ::NFD_OpenDialogMultipleU8_With;

    using ::NFD_SaveDialogN;
    using ::NFD_SaveDialogU8;
    using ::NFD_SaveDialogN_With_Impl;
    using ::NFD_SaveDialogN_With;
    using ::NFD_SaveDialogU8_With_Impl;
    using ::NFD_SaveDialogU8_With;

    using ::NFD_PickFolderN;
    using ::NFD_PickFolderU8;
    using ::NFD_PickFolderN_With_Impl;
    using ::NFD_PickFolderN_With;
    using ::NFD_PickFolderU8_With_Impl;
    using ::NFD_PickFolderU8_With;

    using ::NFD_PickFolderMultipleN;
    using ::NFD_PickFolderMultipleU8;
    using ::NFD_PickFolderMultipleN_With_Impl;
    using ::NFD_PickFolderMultipleN_With;
    using ::NFD_PickFolderMultipleU8_With_Impl;
    using ::NFD_PickFolderMultipleU8_With;

    using ::NFD_GetError;
    using ::NFD_ClearError;

    using ::NFD_PathSet_GetCount;
    using ::NFD_PathSet_GetPathN;
    using ::NFD_PathSet_GetPathU8;
    using ::NFD_PathSet_FreePathN;
    using ::NFD_PathSet_FreePathU8;
    using ::NFD_PathSet_GetEnum;
    using ::NFD_PathSet_FreeEnum;
    using ::NFD_PathSet_EnumNextN;
    using ::NFD_PathSet_EnumNextU8;
    using ::NFD_PathSet_Free;
}